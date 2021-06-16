#include "lp_humanfeature.h"


#include "Eigen/Core"
#include "eiquadprog/eiquadprog-fast.hpp"
#include "eiquadprog/eiquadprog.hpp"

#include "extern/opennurbs/opennurbs.h"

#include "lp_openmesh.h"
#include "lp_renderercam.h"

#include "renderer/lp_glselector.h"

#include <QPainter>
#include <QComboBox>
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

struct LP_HumanFeature::member {

    bool convert2ON_Mesh(LP_OpenMesh opMesh);
    bool processFeaturePoint(QObject *watched, QEvent *event, LP_HumanFeature *widget);

    std::vector<QVector3D> get3DFeaturePoints();

    ON_Mesh mesh;
    std::vector<FeaturePoint> featurePoints;
    LP_RendererCam cam;
};

LP_HumanFeature::~LP_HumanFeature()
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

bool LP_HumanFeature::eventFilter(QObject *watched, QEvent *event)
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
                        mMember->convert2ON_Mesh(std::static_pointer_cast<LP_OpenMeshImpl>(obj.lock()));
                        break;
                    }
                }
            }
        }else if ( e->button() == Qt::RightButton ){
            //g_GLSelector->Clear();
            mObject = LP_Objectw();
            mMember->mesh.Destroy();
            mLabel->setText("Select A Mesh");

            emit glUpdateRequest();
        }
    }
    return QObject::eventFilter(watched, event);
}

QWidget *LP_HumanFeature::DockUi()
{
    mWidget = std::make_shared<QWidget>();
    auto widget = mWidget.get();
    QVBoxLayout *layout = new QVBoxLayout;

    mLabel = new QLabel("Select a mesh");
    QStringList types={"Points", "Girths"};
    mCB_FeatureType = new QComboBox(widget);   //Type
    mCB_FeatureType->addItems(types);

    QPushButton *pb_export_features = new QPushButton("Export Features");
    layout->addWidget(mLabel);
    layout->addWidget(mCB_FeatureType);
    layout->addWidget(pb_export_features);

    layout->addStretch();

    widget->setLayout(layout);

    connect(pb_export_features,&QPushButton::clicked,[this](){
        auto filename = QFileDialog::getSaveFileName(nullptr, "Export", QDir::currentPath(),
                                                     "Json (*.json)");
        if ( filename.isEmpty()){
            return;
        }
        QFile file(filename);
        if ( !file.open(QIODevice::WriteOnly)){
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

        emit glUpdateRequest();

    });

    return widget;
}

bool LP_HumanFeature::Run()
{
    mMember = std::make_shared<member>();

    FeaturePoint fpt;
    fpt.mName = "Test";
    fpt.mParams = {1,2,3,1.0,0.0};
    mMember->featurePoints.emplace_back(std::move(fpt));  //Hard-code for debug
    emit glUpdateRequest();
    return false;
}

void LP_HumanFeature::FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
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

    f->glDrawElements(GL_TRIANGLES, mMember->mesh.FaceCount()*3, GL_UNSIGNED_INT,
                      0);

    //constexpr auto sqrtEps = std::numeric_limits<char>::epsilon();
    //f->glDepthRangef(-sqrtEps, 1.0f - sqrtEps);


    f->glDepthFunc(GL_LEQUAL);
    mProgram->setUniformValue("m4_mvp", proj*view);

    mProgram->setUniformValue("v3_color", QVector3D(0.2,0.2,0.2));
//    f->glDrawElements(GL_LINES, mMember->evid.size(), GL_UNSIGNED_INT,
//                      mMember->evid.data());

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


//    f->glLineWidth(5.f);
//    for ( auto &fc : mMember->featureCurves ){
//        mProgramFeatures->setAttributeArray("a_pos", fc.data());
//        f->glDrawArrays(GL_LINE_STRIP, 0, fc.size());
//    }

    f->glEnable(GL_PROGRAM_POINT_SIZE);     //Enable point-size controlled by shader

    auto &&fpts = mMember->get3DFeaturePoints();
    mProgramFeatures->setAttributeArray("a_pos", fpts.data());
    f->glDrawArrays(GL_POINTS, 0, fpts.size());

    mProgramFeatures->disableAttributeArray("a_pos");
    mProgramFeatures->release();

    f->glDepthRangef(0.0f, 1.0f);
    f->glLineWidth(1.f);
    fbo->release();
}

void LP_HumanFeature::PainterDraw(QWidget *glW)
{
    if ( "window_Normal" == glW->objectName() || !mMember->cam){
        return;
    }
    auto cam = mMember->cam;
    auto view = cam->ViewMatrix(),
         proj = cam->ProjectionMatrix(),
         vp   = cam->ViewportMatrix();

    view = vp * proj * view;
    auto &&h = cam->ResolutionY();

    QPainter painter(glW);
    int fontSize(13);
    QFont font("Arial", fontSize);
    QFontMetrics fmetric(font);
    QFont orgFont = painter.font();
    painter.setPen(qRgb(100,255,100));

    painter.setFont(font);

    auto &&pts = mMember->get3DFeaturePoints();
    int i=0;
    for ( auto &p : pts ){
        auto v = view * p;
        painter.drawText(QPointF(v.x(), h-v.y()), QString("%1").arg(mMember->featurePoints[i].mName.c_str()));
        ++i;
    }
}

QString LP_HumanFeature::MenuName()
{
    return tr("menuPlugins");
}

QAction *LP_HumanFeature::Trigger()
{
    if ( !mAction ){
        mAction = new QAction("Human Features Marking");
    }
    return mAction;
}

void LP_HumanFeature::initializeGL()
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

bool LP_HumanFeature::member::convert2ON_Mesh(LP_OpenMesh opMesh)
{
    auto m = opMesh->Mesh();
    const auto &&nVs = m->n_vertices();
    auto pptr = m->points();
    //auto nptr = m->vertex_normals();
    mesh.m_V.Reserve(nVs);
    mesh.m_V.SetCount(nVs);
    for (int i=0; i<int(nVs); ++i, ++pptr/*, ++nptr*/ ){
        mesh.m_V.At(i)->Set((*pptr)[0],(*pptr)[1],(*pptr)[2]);
    }
    return true;
}

bool LP_HumanFeature::member::processFeaturePoint(QObject *watched, QEvent *event, LP_HumanFeature *widget)
{

}

std::vector<QVector3D> LP_HumanFeature::member::get3DFeaturePoints()
{
    const auto &nVs = mesh.VertexCount();
    if ( 0 >= nVs ){
        return std::vector<QVector3D>();
    }
    std::vector<QVector3D> pts;
    for ( auto &fp : featurePoints ){
        auto &params = fp.mParams;
        const int &i = std::get<0>(params),
                  &j = std::get<1>(params),
                  &k = std::get<2>(params);
        const double &u = std::get<3>(params),
                     &v = std::get<4>(params);
        const double w = 1.0-u-v;
        if ( i >= nVs || j >= nVs || k >= nVs ){
            continue;   //Did not check 1=u+v+w
        }
        auto _p = u * mesh.m_V[i] + v * mesh.m_V[j] + w * mesh.m_V[k];
        pts.emplace_back(QVector3D(_p.x, _p.y, _p.z));
    }
    return pts;
}
