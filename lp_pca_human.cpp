#include "lp_pca_human.h"

#ifdef Q_OS_WIN
#undef WIN32
#include "opennurbs.h"
#define WIN32
#elif defined Q_OS_LINUX
#include "opennurbs.h"
#endif

#include "Eigen/Core"
#include "eiquadprog/eiquadprog-fast.hpp"
#include "eiquadprog/eiquadprog.hpp"

#include "lp_openmesh.h"
#include "lp_renderercam.h"

#include "renderer/lp_glselector.h"

#include <QAction>
#include <QSlider>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLExtraFunctions>
#include <QLabel>
#include <QMatrix4x4>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>


struct LP_PCA_Human::member {
    std::vector<double> defaultParametersM = {90.6,86.7,81.2,95.2,38.5,77.1,37.7,174.0};
    ON_Mesh mesh;
    uint nVs, nFs;
    std::vector<uint> fvid;
    std::vector<uint> evid;

    std::vector<std::vector<QVector3D>> featureCurves;
    std::vector<QVector3D> featurePoints;
    LP_RendererCam cam;
    std::vector<double> parameters;
    std::vector<std::vector<double>> r, pc, edgeRatios, cvxHull;
    std::vector<double> e, mA;
    std::vector<uint> fpt, f;
    std::vector<std::vector<uint>> edgeNodes;


    void updateReference();
    void rationalize();
    bool checkConvexHull(const std::vector<std::vector<double>>& cvxHull,
                         const std::vector<double> &params, double tolerance);
    void setParameters(const std::vector<double> &in);
};

LP_PCA_Human::~LP_PCA_Human()
{
    emit glContextRequest([this](){
        delete mProgram;
        mProgram = nullptr;

        delete mProgramFeatures;
        mProgramFeatures = nullptr;
    });
    Q_ASSERT(!mProgram);
    Q_ASSERT(!mProgramFeatures);
}

bool LP_PCA_Human::eventFilter(QObject *watched, QEvent *event)
{
    if ( QEvent::MouseButtonRelease == event->type()){
        auto e = static_cast<QMouseEvent*>(event);

        if ( e->button() == Qt::LeftButton ){
            if (!mObject.lock()){
                auto &&objs = g_GLSelector->SelectInWorld("Shade",e->pos());
                for ( const auto &obj : objs ){
                    auto o = obj.lock();
                    if ( o && LP_OpenMeshImpl::mTypeName == o->TypeName()){
                        mLabel->setText(o->Uuid().toString());
                        mObject = obj;
                        break;
                    }
                }
            }
        }else if ( e->button() == Qt::RightButton ){
            //g_GLSelector->Clear();
            mObject = LP_Objectw();
            mLabel->setText("Select A Mesh");

            emit glUpdateRequest();
        }
    }
    return QObject::eventFilter(watched, event);
}

QWidget *LP_PCA_Human::DockUi()
{
    mWidget = std::make_shared<QWidget>();
    auto widget = mWidget.get();
    QGridLayout *layout = new QGridLayout;

//    mLabel = new QLabel("Select a mesh");
    QPushButton *pb_impport_reference = new QPushButton("Import Reference");   //Export Loop button
    QPushButton *pb_rationallize = new QPushButton("Rationalize");   //Rationalize button

    std::vector<QLabel*> labels(8);
    for ( int i=0; i<8; ++i ){
        labels[i] = new QLabel;
    }

    QSlider *slider_bust = new QSlider(nullptr);
    slider_bust->setOrientation(Qt::Horizontal);
    slider_bust->setRange(790, 1130);
    slider_bust->setValue(900);

    QSlider *slider_ubust = new QSlider(nullptr);
    slider_ubust->setOrientation(Qt::Horizontal);
    slider_ubust->setRange(700, 1010);
    slider_ubust->setValue(867);

    QSlider *slider_waist = new QSlider(nullptr);
    slider_waist->setOrientation(Qt::Horizontal);
    slider_waist->setRange(520, 1130);
    slider_waist->setValue(812);

    QSlider *slider_hip = new QSlider(nullptr);
    slider_hip->setOrientation(Qt::Horizontal);
    slider_hip->setRange(790, 1130);
    slider_hip->setValue(952);

    QSlider *slider_neck = new QSlider(nullptr);
    slider_neck->setOrientation(Qt::Horizontal);
    slider_neck->setRange(290, 450);
    slider_neck->setValue(385);

    QSlider *slider_leg = new QSlider(nullptr);
    slider_leg->setOrientation(Qt::Horizontal);
    slider_leg->setRange(650, 950);
    slider_leg->setValue(771);

    QSlider *slider_shoulder = new QSlider(nullptr);
    slider_shoulder->setOrientation(Qt::Horizontal);
    slider_shoulder->setRange(290, 600);
    slider_shoulder->setValue(377);

    QSlider *slider_height = new QSlider(nullptr);
    slider_height->setOrientation(Qt::Horizontal);
    slider_height->setRange(1450, 2010);
    slider_height->setValue(1740);

//    layout->addWidget(mLabel);
    layout->addWidget(pb_impport_reference,0,0,1,3);
    layout->addWidget(new QLabel("Bust"),1,0,1,1);
    layout->addWidget(slider_bust,1,1,1,1);
    layout->addWidget(new QLabel("Under Bust"),2,0,1,1);
    layout->addWidget(slider_ubust,2,1,1,1);
    layout->addWidget(new QLabel("Waist"),3,0,1,1);
    layout->addWidget(slider_waist,3,1,1,1);
    layout->addWidget(new QLabel("Hip"),4,0,1,1);
    layout->addWidget(slider_hip,4,1,1,1);
    layout->addWidget(new QLabel("Neck Girth"),5,0,1,1);
    layout->addWidget(slider_neck,5,1,1,1);
    layout->addWidget(new QLabel("Inside Leg"),6,0,1,1);
    layout->addWidget(slider_leg,6,1,1,1);
    layout->addWidget(new QLabel("Shoulder"),7,0,1,1);
    layout->addWidget(slider_shoulder,7,1,1,1);
    layout->addWidget(new QLabel("Height"),8,0,1,1);
    layout->addWidget(slider_height,8,1,1,1);

    for ( int i=1; i<=8; ++i ){
        layout->addWidget(labels[i-1],i,2,1,1);
    }

    layout->addWidget(pb_rationallize,9,0,1,3);

    widget->setLayout(layout);

    std::function<void(QSlider*,int)> updateSlider = [this](QSlider *slider,int id){
        slider->setValue(mMember->parameters[id]*10);
    };
    std::function<void(const int&,int,QLabel*)> updateParams = [this](const int& value,int id, QLabel *label){
        mMember->parameters[id] = 0.1*value;
        label->setNum(0.1*value);
        mMember->updateReference();
        emit glUpdateRequest();
    };
    connect(this, &LP_PCA_Human::updateSliders, std::bind(updateSlider, slider_bust, 0));
    connect(slider_bust, &QSlider::valueChanged, std::bind(updateParams, std::placeholders::_1, 0, labels[0]));

    connect(this, &LP_PCA_Human::updateSliders, std::bind(updateSlider, slider_ubust, 1));
    connect(slider_ubust, &QSlider::valueChanged, std::bind(updateParams, std::placeholders::_1, 1, labels[1]));

    connect(this, &LP_PCA_Human::updateSliders, std::bind(updateSlider, slider_waist, 2));
    connect(slider_waist, &QSlider::valueChanged, std::bind(updateParams, std::placeholders::_1, 2, labels[2]));

    connect(this, &LP_PCA_Human::updateSliders, std::bind(updateSlider, slider_hip, 3));
    connect(slider_hip, &QSlider::valueChanged, std::bind(updateParams, std::placeholders::_1, 3, labels[3]));

    connect(this, &LP_PCA_Human::updateSliders, std::bind(updateSlider, slider_neck, 4));
    connect(slider_neck, &QSlider::valueChanged, std::bind(updateParams, std::placeholders::_1, 4, labels[4]));

    connect(this, &LP_PCA_Human::updateSliders, std::bind(updateSlider, slider_leg, 5));
    connect(slider_leg, &QSlider::valueChanged, std::bind(updateParams, std::placeholders::_1, 5, labels[5]));

    connect(this, &LP_PCA_Human::updateSliders, std::bind(updateSlider, slider_shoulder, 6));
    connect(slider_shoulder, &QSlider::valueChanged, std::bind(updateParams, std::placeholders::_1, 6, labels[6]));

    connect(this, &LP_PCA_Human::updateSliders, std::bind(updateSlider, slider_height, 7));
    connect(slider_height, &QSlider::valueChanged, std::bind(updateParams, std::placeholders::_1, 7, labels[7]));


    connect(pb_impport_reference,&QPushButton::clicked,[this](){
//        auto filename = QFileDialog::getOpenFileName(nullptr, "Import Reference", QDir::currentPath(),
//                                                     "Json (*.json)");
//        if ( filename.isEmpty()){
//            return;
//        }
        QString filename("male.json");
        QFile file(filename);
        if ( !file.open(QIODevice::ReadOnly)){
            qDebug()  << file.errorString();
            return;
        }
        QJsonDocument json = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (json.isEmpty()){
            return;
        }
        auto &&obj = json.object();
        qDebug() << obj.keys();
        mMember->nVs = obj["numV"].toInt();
        mMember->nFs = obj["numF"].toInt();

        auto r_ = obj["R"].toArray(),
             e_ = obj["e"].toArray(),
             pc_ = obj["pc"].toArray(),
             mA_ = obj["mA"].toArray(),
             f_  = obj["F"].toArray(),
             cvxHull_ = obj["cvxHull"].toArray(),
             edgeNodes_ = obj["edgeNodes"].toArray(),
             edgeRatios_ = obj["edgeRatios"].toArray(),
             fpt_ = obj["fpt"].toArray();

        std::vector<std::vector<double>> &r = mMember->r,
                                         &pc = mMember->pc,
                                         &edgeRatios = mMember->edgeRatios,
                                         &cvxHull = mMember->cvxHull;
        std::vector<double> &e = mMember->e,
                            &mA = mMember->mA;
        std::vector<uint> &fpt = mMember->fpt,
                          &f = mMember->f;
        std::vector<std::vector<uint>> &edgeNodes = mMember->edgeNodes;


        std::function<void(const QJsonArray&,std::vector<std::vector<double>>&)> retrieve_f =
                [](const QJsonArray& in,std::vector<std::vector<double>>& out){
            for ( auto &&in_ : in){
                auto _r = in_.toArray();
                std::vector<double> tmp(_r.size());
                auto n = _r.size();
                for ( int i=0; i<n; ++i ){
                    tmp[i] = _r[i].toDouble();
                }
                out.emplace_back(std::move(tmp));
            }
        };

        r.clear();
        pc.clear();
        edgeRatios.clear();
        cvxHull.clear();

        retrieve_f(r_, r);
        retrieve_f(pc_, pc);//28080x20
        retrieve_f(edgeRatios_, edgeRatios);
        retrieve_f(cvxHull_, cvxHull);

        auto n = e_.size();
        e.resize(n);
        for ( int i=0; i<n; ++i ){
            e[i] = e_[i].toDouble();
        }
        n = mA_.size();
        mA.resize(n);
        for ( int i=0; i<n; ++i ){
            mA[i] = mA_[i].toDouble();
        }

        edgeNodes.clear();
        for ( auto &&in_ : edgeNodes_){
            auto _r = in_.toArray();
            std::vector<uint> tmp(_r.size());
            auto n = _r.size();
            for ( int i=0; i<n; ++i ){
                tmp[i] = _r[i].toInt();
            }
            edgeNodes.emplace_back(std::move(tmp));
        }

        fpt.clear();
        for ( auto &&in_ : fpt_){
            fpt.emplace_back(std::move(in_.toInt()));
        }

        f.clear();
        for ( auto &&in_ : f_){
            f.emplace_back(std::move(in_.toInt()));
        }
        mMember->mesh.Destroy();
        mMember->updateReference();
        emit glUpdateRequest();

    });

    connect(pb_rationallize, &QPushButton::clicked, [this](){
        if ( !mMember->mesh.IsValid()){
            return;
        }
        mMember->rationalize();
        mMember->updateReference();
        emit glUpdateRequest();
        emit updateSliders();
    });

    return widget;
}

bool LP_PCA_Human::Run()
{
    mMember = std::make_shared<member>();
    mMember->parameters = mMember->defaultParametersM;   //Male default

    emit glUpdateRequest();

    eiquadprog::solvers::EiquadprogFast qp;
    qp.reset(3,0,3);

    Eigen::MatrixXd Q(3,3);
    Q.setIdentity();

    Eigen::VectorXd C(3);
    C(0) = 0.0;
    C(1) = -5.0;
    C(2) = 0.0;

    Eigen::MatrixXd Aeq(0,3);
    Eigen::VectorXd Beq(0);

    Eigen::MatrixXd Aineq(3,3);
    Eigen::VectorXd Bineq(3);

    Aineq(0,0) = -4.0; Aineq(0,1) = -3.0; Aineq(0,2) =  0.0;
    Aineq(1,0) =  2.0; Aineq(1,1) =  1.0; Aineq(1,2) =  0.0;
    Aineq(2,0) =  0.0; Aineq(2,1) = -2.0; Aineq(2,2) =  1.0;

    Bineq(0) = 8.0;
    Bineq(1) = -2.0;
    Bineq(2) = 0.0;

    Eigen::VectorXd x(3);
    eiquadprog::solvers::EiquadprogFast_status status = qp.solve_quadprog(Q, C, Aeq, Beq, Aineq, Bineq, x);


    return false;
}

void LP_PCA_Human::FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    if (!mInitialized){
        initializeGL();
        mMember->cam = cam;
        return;
    }

    QMatrix4x4 view = cam->ViewMatrix(),
               proj = cam->ProjectionMatrix();


    auto f = ctx->extraFunctions();
    fbo->bind();

    mProgram->bind();
    mProgram->setUniformValue("m4_view", view);
    mProgram->setUniformValue("m4_mvp", proj*view);
    mProgram->setUniformValue("m3_normal", view.normalMatrix());
    mProgram->setUniformValue("v3_color", QVector3D(0.4,0.4,0.4));

    mProgram->enableAttributeArray("a_pos");
    mProgram->enableAttributeArray("a_norm");

    mProgram->setAttributeArray("a_pos", &mMember->mesh.m_V.First()->x,3);
    mProgram->setAttributeArray("a_norm", &mMember->mesh.m_N.First()->x,3);

    f->glDrawElements(GL_TRIANGLES, mMember->fvid.size(), GL_UNSIGNED_INT,
                      mMember->fvid.data());

    //constexpr auto sqrtEps = std::numeric_limits<char>::epsilon();
    //f->glDepthRangef(-sqrtEps, 1.0f - sqrtEps);


    f->glDepthFunc(GL_LEQUAL);
    mProgram->setUniformValue("m4_mvp", proj*view);

    mProgram->setUniformValue("v3_color", QVector3D(0.2,0.2,0.2));
    f->glDrawElements(GL_LINES, mMember->evid.size(), GL_UNSIGNED_INT,
                      mMember->evid.data());

    mProgram->disableAttributeArray("a_pos");
    mProgram->disableAttributeArray("a_norm");

    mProgram->release();    //Release the mesh program

    mProgramFeatures->bind(); //Bind the feature curve program
    mProgramFeatures->enableAttributeArray("a_pos");

    QMatrix4x4 tmp;
    auto ratio = 0.01f/(cam->Roll()-cam->Target()).length();

    tmp.translate(QVector3D(0.0f,0.0f,-ratio));
    mProgramFeatures->setUniformValue("m4_mvp", tmp*proj*view);
    mProgramFeatures->setUniformValue("v4_color", QVector4D(0.8, 0.8, 0.2, 0.6));

    //f->glDepthRangef(-2.0f*sqrtEps, 1.0f - 2.0f*sqrtEps);


    f->glLineWidth(5.f);
    for ( auto &fc : mMember->featureCurves ){
        mProgramFeatures->setAttributeArray("a_pos", fc.data());
        f->glDrawArrays(GL_LINE_STRIP, 0, fc.size());
    }

    f->glEnable(GL_PROGRAM_POINT_SIZE);     //Enable point-size controlled by shader
    mProgramFeatures->setAttributeArray("a_pos", mMember->featurePoints.data());
    f->glDrawArrays(GL_POINTS, 0, mMember->featurePoints.size());

    mProgramFeatures->disableAttributeArray("a_pos");
    mProgramFeatures->release();

    f->glDepthRangef(0.0f, 1.0f);
    f->glLineWidth(1.f);
    fbo->release();
}

void LP_PCA_Human::PainterDraw(QWidget *glW)
{

}

QString LP_PCA_Human::MenuName()
{
    return tr("menuPlugins");
}

QAction *LP_PCA_Human::Trigger()
{
    if ( !mAction ){
        mAction = new QAction("PCA Human");
    }
    return mAction;
}

void LP_PCA_Human::initializeGL()
{
    std::string vsh, fsh;
    vsh =
            "attribute vec3 a_pos;\n"
            "attribute vec3 a_norm;\n"
            "uniform mat4 m4_view;\n"
            "uniform mat4 m4_mvp;\n"
            "uniform mat3 m3_normal;\n"
            "varying vec3 normal;\n"
            "varying vec3 pos;\n"
            "void main(){\n"
            "   pos = vec3( m4_view * vec4(a_pos, 1.0));\n"
            "   normal = m3_normal * a_norm;\n"
            "   gl_Position = m4_mvp*vec4(a_pos, 1.0);\n"
            "}";
    fsh =
            "varying vec3 normal;\n"
            "varying vec3 pos;\n"
            "uniform vec3 v3_color;\n"
            "void main(){\n"
            "   vec3 lightPos = vec3(0.0, 1000.0, 0.0);\n"
            "   vec3 viewDir = normalize( - pos);\n"
            "   vec3 lightDir = normalize(lightPos - pos);\n"
            "   vec3 H = normalize(viewDir + lightDir);\n"
            "   vec3 N = normalize(normal);\n"
            "   vec3 ambi = v3_color;\n"
            "   float Kd = max(dot(H, N), 0.0);\n"
            "   vec3 diff = Kd * vec3(0.2, 0.2, 0.2);\n"
            "   vec3 color = ambi + diff;\n"
            "   float Ks = pow( Kd, 80.0 );\n"
            "   vec3 spec = Ks * vec3(0.5, 0.5, 0.5);\n"
            "   color += spec;\n"
            "   gl_FragColor = vec4(color,1.0);\n"
            "}";

    auto prog = new QOpenGLShaderProgram;
    prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh.c_str());
    prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh.data());
    if (!prog->create() || !prog->link()){
        qDebug() << prog->log();
        return;
    }

    mProgram = prog;

    std::string vsh2 =
            "attribute vec3 a_pos;\n"
            "uniform mat4 m4_mvp;\n"
            "void main(){\n"
            "   gl_Position = m4_mvp * vec4(a_pos, 1.0);\n"
            "   gl_PointSize = 10.0;\n"
            "}";
    std::string fsh2 =
            "uniform vec4 v4_color;\n"
            "void main(){\n"
            "   gl_FragColor = v4_color;\n"
            "}";

    prog = new QOpenGLShaderProgram;
    prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh2.c_str());
    prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh2.data());
    if (!prog->create() || !prog->link()){
        qDebug() << prog->log();
        return;
    }
    mProgramFeatures =  prog;
    mInitialized = true;
}

void LP_PCA_Human::member::updateReference()
{
    int nParas = 7;
    std::vector<double> para(nParas);
    for ( int i=0; i<nParas; ++i ){
        para[i] = parameters[i]/parameters.back();
    }

    auto &m = mesh;
    if ( !m.IsValid()){
        //Reserve space
        m.m_V.Reserve(nVs);
        m.m_V.SetCount(nVs);

        fvid.clear();
        evid.clear();

        m.m_F.Reserve(nFs);
        m.m_F.SetCount(nFs);

        for ( int i=0; i<int(nFs); ++i ){
            auto pF = m.m_F.At(i);
            const auto id = 3*i;
            pF->vi[0] = f.at(id);
            pF->vi[1] = f.at(id+1);
            pF->vi[2] = pF->vi[3] = f.at(id+2);

            fvid.emplace_back(pF->vi[0]);
            fvid.emplace_back(pF->vi[1]);
            fvid.emplace_back(pF->vi[2]);

            evid.emplace_back(pF->vi[0]);
            evid.emplace_back(pF->vi[1]);

            evid.emplace_back(pF->vi[1]);
            evid.emplace_back(pF->vi[2]);

            evid.emplace_back(pF->vi[2]);
            evid.emplace_back(pF->vi[0]);
        }
    }

    std::vector<double> R1(r.size()); //20x1
    auto nRs = R1.size();
    //Dot product
    for ( size_t i=0; i<nRs; ++i ){
        auto &r1_ = R1[i];
        r1_=0.0;
        for ( size_t j=0; j<para.size(); ++j ){
            r1_ += r[i][j] * para[j];
        }
    }
    //Vector sum
    for ( size_t i=0; i<nRs; ++i ){
        R1[i] += e[i];
    }

    Q_ASSERT(size_t(nVs*3)==pc.size());
    double _inv = parameters[7];// * 0.01;
    for ( int i=0; i < int(nVs); ++i ){
        const int idx = 3*i,
                idy = idx+1,
                idz = idy+1;
        double x = 0.0,
               y = 0.0,
               z = 0.0;

        auto &pc_x = pc.at(idx);
        auto &pc_y = pc.at(idy);
        auto &pc_z = pc.at(idz);
        for ( size_t j=0; j < nRs; ++j ){//Dot product
            x += pc_x[j] * R1[j];
            y += pc_y[j] * R1[j];
            z += pc_z[j] * R1[j];
        }
        auto &v = m.m_V[i];
        v.Set(x + mA[idx], y + mA[idy], z + mA[idz]);//Added
        v *= _inv;
    }

    m.ComputeVertexNormals();

//    ON_TextLog log;
//    qDebug() << m.IsValid(&log);

    //Add feature curves
    const auto nFCs = edgeRatios.size();
    auto &FCs = featureCurves;
    FCs.resize(nFCs);
    for ( int i=0; i<int(nFCs); ++i ){
        const auto &eR_ = edgeRatios.at(i);
        const auto  &eN_ = edgeNodes.at(i);
        const auto nPts = eR_.size();
        FCs[i].resize(nPts);
        for ( int j=0; j<int(nPts); ++j ){
            auto &&id0 = eN_.at(2*j),
                 &&id1 = eN_.at(2*j+1);
            auto &&ratio = eR_.at(j);
            const auto &p0 = m.m_V[id0],
                       &p1 = m.m_V[id1];
            auto &&p = p0 * ( 1.0-ratio) + p1 * ratio;
            FCs[i][j].setX(p.x);
            FCs[i][j].setY(p.y);
            FCs[i][j].setZ(p.z);
        }
        FCs[i].emplace_back(FCs[i].front());
    }


    //Add feature points
    const auto nFPs = fpt.size();
    auto &FPs = featurePoints;
    FPs.resize(nFPs);
    for ( size_t i=0; i<nFPs; ++i ){
        auto &&id0 = fpt.at(i);
        auto pF = m.m_V.At(id0);
        FPs[i].setX(pF->x);
        FPs[i].setY(pF->y);
        FPs[i].setZ(pF->z);
    }

    auto bbox = m.BoundingBox();
    auto bbmax = bbox.Max(),
         bbmin = bbox.Min();
    auto diag = (bbmax - bbmin).Length();
    if ( std::isinf(diag)){
        return;
    }
    auto center = 0.5*(bbmax + bbmin);

    cam->setDiagonal(diag);
    cam->SetTarget(QVector3D(center.x, center.y, center.z));
    cam->RefreshCamera();
}

void LP_PCA_Human::member::rationalize()
{
    auto nDim = parameters.size() - 1;
    auto nConstraints = cvxHull.size();

    eiquadprog::solvers::EiquadprogFast qp;

    qp.reset(nDim, 0, nConstraints);    //

    Eigen::MatrixXd Q(nDim, nDim);
    //Q.setZero();
    Q.setIdentity();
    //Q *= 2.0;

    Eigen::VectorXd C(nDim);
    for ( int i=0; i<int(nDim); ++i ){
        C(i) = parameters[i]/parameters.back();
    }

    Eigen::MatrixXd Aeq(0, nDim);
    Eigen::VectorXd Beq(0);

    Eigen::MatrixXd Aineq(nConstraints, nDim);
    Aineq.setZero();
    for ( int i=0; i<int(nConstraints); ++i ){
        for ( int j=0; j<int(nDim); ++j ){
            Aineq(i,j) = cvxHull[i][j];
        }
    }

    Eigen::VectorXd Bineq(nConstraints);
    for ( int i=0; i<int(nConstraints); ++i ){
        Bineq(i) = -cvxHull[i][nDim];
    }

    Eigen::VectorXd x(nDim);
    Eigen::VectorXi activeSet(nDim);
    size_t activeSetSize;
    x.setZero();

    eiquadprog::solvers::EiquadprogFast_status expected = eiquadprog::solvers::EIQUADPROG_FAST_OPTIMAL;

    eiquadprog::solvers::EiquadprogFast_status status = qp.solve_quadprog(Q, C, Aeq, Beq, Aineq, Bineq, x);
//    double out = eiquadprog::solvers::solve_quadprog(Q, C, Aeq, Beq,
//                                                     Aineq.transpose(), Bineq,
//                                                     x, activeSet, activeSetSize);

    std::vector<double> test(nDim);
    x *= -1.0;  //Why ?
    memcpy(test.data(), x.data(), nDim*sizeof(*x.data()));
    if ( checkConvexHull(cvxHull, test, 1e-3)){
        qDebug() << "Irrational Inputs!!! Set to default";
        setParameters(defaultParametersM);

        return;
    }
    std::vector<double> params = parameters;
    const auto &t = parameters.back();
    for ( int i=0; i<int(nDim); ++i ){
        params[i] = t * x(i);
    }

    setParameters(params);
    //Q_ASSERT(std::abs(qp.getObjValue() - val) < 1e-6);

    //Q_ASSERT(x.isApprox(solution));
}

bool LP_PCA_Human::member::checkConvexHull(const std::vector<std::vector<double> > &cvxHull,
                                              const std::vector<double> &params,
                                              double tolerance)
{
    std::vector<double> params_ = parameters;
    const auto nParams = params_.size();
    for (size_t i=0; i<nParams; ++i){
        params_[i] = params[i];
    }
    params_.back() = 1.0;
    for ( const auto &cvx : cvxHull ){
        double tmp = 0.0;

        for ( size_t i=0; i<nParams; ++i ){
            tmp += cvx[i] * params_[i];
        }
        if ( tmp > tolerance ){
            return true;
        }
    }
    return false;
}

void LP_PCA_Human::member::setParameters(const std::vector<double> &in)
{
    parameters = in;
}
