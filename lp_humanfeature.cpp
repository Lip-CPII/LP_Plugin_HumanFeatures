#include "lp_humanfeature.h"

#include "xmmintrin.h"
#include "pmmintrin.h"



#include "embree3/rtcore.h"
#include "embree3/rtcore_common.h"
#include "embree3/rtcore_ray.h"

#include "extern/opennurbs/opennurbs.h"

#include "lp_openmesh.h"
#include "lp_renderercam.h"

#include "renderer/lp_glselector.h"

#include <QInputDialog>
#include <QStandardItemModel>
#include <QListView>
#include <QGroupBox>
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

    bool pickFeaturePoint(QPoint &&pickPos);
    QVector3D evaluationFeaturePoint(const ON_Mesh &m, const FeaturePoint &fp);
    void initializeEmbree3();

    void importFeatures(const QString &filename);
    void exportFeatures(const QString &filename);

    RTCDevice rtDevice;
    RTCScene rtScene;
    RTCGeometry rtGeometry;
    unsigned int rtGeomID;

    std::shared_ptr<FeaturePoint> pickPoint;

    std::vector<QVector3D> get3DFeaturePoints();

    ON_Mesh mesh;
    std::vector<FeaturePoint> featurePoints;
    LP_RendererCam cam;

    std::function<void()> updateFPsList;
    ~member(){
        rtcReleaseScene(rtScene);
        rtcReleaseDevice(rtDevice);
    }
};

LP_HumanFeature::~LP_HumanFeature()
{
    emit glContextRequest([this](){
        delete mProgram;
        mProgram = nullptr;

        delete mProgramFeatures;
        mProgramFeatures = nullptr;
    });

    mMember.reset();

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
            }else{
                mMember->pickFeaturePoint(e->pos());
                emit glUpdateRequest();
            }
        }else if ( e->button() == Qt::RightButton ){
            //g_GLSelector->Clear();
            mObject = LP_Objectw();
            mMember->mesh.Destroy();
            mLabel->setText("Select A Mesh");

            emit glUpdateRequest();
        }
    } else if ( QEvent::KeyPress == event->type()){
        auto e = static_cast<QKeyEvent*>(event);
        switch (e->key()) {
            case Qt::Key_Return:
            case Qt::Key_Space:
            case Qt::Key_Enter:
            if ( "Points" == mCB_FeatureType->currentText()){
                auto name = QInputDialog::getText(0, "Input", "Feature Name");  //Ask for featture name
                if ( name.isEmpty()){
                    break;
                }
                mMember->pickPoint->mName = name.toStdString();
                mMember->featurePoints.emplace_back( *mMember->pickPoint.get());
                mMember->pickPoint.reset(); //Reset the pick Point
                mMember->updateFPsList();   //Update the listview
                emit glUpdateRequest();
            } else {
                //Feature Girth
            }
            break;
        }
    }
    return QObject::eventFilter(watched, event);
}

QWidget *LP_HumanFeature::DockUi()
{
    mMember = std::make_shared<member>();


    mWidget = std::make_shared<QWidget>();
    auto widget = mWidget.get();
    QVBoxLayout *layout = new QVBoxLayout;

    mLabel = new QLabel("Select a mesh");
    QStringList types={"Points", "Girths"};
    mCB_FeatureType = new QComboBox(widget);   //Type
    mCB_FeatureType->addItems(types);

    auto pFPGroupBox = new QGroupBox("Feature Points");
    auto fpLayout = new QVBoxLayout;
    auto listview = new QListView;

    fpLayout->addWidget(listview);
    pFPGroupBox->setLayout(fpLayout);

    QPushButton *pb_import_features = new QPushButton("Import Features");
    QPushButton *pb_export_features = new QPushButton("Export Features");
    layout->addWidget(mLabel);
    layout->addWidget(pb_import_features);
    layout->addWidget(mCB_FeatureType);
    layout->addWidget(pFPGroupBox);
    layout->addWidget(pb_export_features);

    layout->addStretch();

    widget->setLayout(layout);

    connect(pb_export_features,&QPushButton::clicked,[this]() mutable {
        auto filename = QFileDialog::getSaveFileName(nullptr, "Export", QDir::currentPath(),
                                                     "Json (*.json)");
        mMember->exportFeatures(filename);
        emit glUpdateRequest();

    });

    connect(pb_import_features,&QPushButton::clicked,[this]() mutable {
        auto filename = QFileDialog::getOpenFileName(nullptr, "Import", QDir::currentPath(),
                                                     "Json (*.json)");
        mMember->importFeatures(filename);
        emit glUpdateRequest();

    });

    connect(mCB_FeatureType, &QComboBox::currentIndexChanged, [pFPGroupBox](const int &id){
        pFPGroupBox->setHidden( 0 != id );
    });

    mMember->updateFPsList = [this,listview](){
        auto model = new QStandardItemModel;

        for ( auto &p : mMember->featurePoints ){
            QString data  = QString("%1\n  (i, j, k) = (%2, %3, %4)\n  (u, v) = (%5, %6)")
                    .arg(p.mName.data())
                    .arg(std::get<0>(p.mParams))
                    .arg(std::get<1>(p.mParams))
                    .arg(std::get<2>(p.mParams))
                    .arg(std::get<3>(p.mParams))
                    .arg(std::get<4>(p.mParams));
            model->appendRow(new QStandardItem(data));
        }
        delete listview->model();
        listview->reset();
        listview->setModel(model);
    };

    return widget;
}

bool LP_HumanFeature::Run()
{
    mMember->initializeEmbree3();
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

//    f->glDrawElements(GL_TRIANGLES, mMember->mesh.FaceCount()*3, GL_UNSIGNED_INT,
//                      0);

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

    if ( mMember->pickPoint ){  //Draw the temporary pick point
        auto pt = mMember->evaluationFeaturePoint(mMember->mesh,
                                                  *mMember->pickPoint.get());
        std::vector<QVector3D> pts = {pt};
        mProgramFeatures->setUniformValue("v4_color", QVector4D(0.8, 0.8, 0.2, 0.6));
        mProgramFeatures->setAttributeArray("a_pos", pts.data());
        f->glDrawArrays(GL_POINTS, 0, 1);
    }

//    f->glLineWidth(5.f);
//    for ( auto &fc : mMember->featureCurves ){
//        mProgramFeatures->setAttributeArray("a_pos", fc.data());
//        f->glDrawArrays(GL_LINE_STRIP, 0, fc.size());
//    }

    mProgramFeatures->setUniformValue("v4_color", QVector4D(0.2, 0.8, 0.2, 0.6));
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
    auto &&h = cam->ResolutionY(),
         &&w = cam->ResolutionX();

    QPainter painter(glW);
    int fontSize(13);
    QFont font("Arial", fontSize);
    QFontMetrics fmetric(font);
    QFont orgFont = painter.font();
    painter.setPen(qRgb(100,255,100));

    painter.setFont(font);

    auto &&pts = mMember->get3DFeaturePoints();
    int i=1;
    for ( auto &p : pts ){
        auto v = view * p;
        QString fName(mMember->featurePoints[i-1].mName.c_str());
        int padding = 10;
        int x = padding;
        if ( 0 == i % 2 ){
            x = w - fmetric.boundingRect(fName).width() - padding;
            painter.drawLine(QPointF(x, (i-1)*20)+fmetric.boundingRect(fName).bottomLeft(),
                             QPointF(v.x(), h-v.y()));

            painter.drawText(QPointF(x, (i-1)*20), QString("%1").arg(fName));
        } else {
            painter.drawLine(QPointF(x, i*20)+fmetric.boundingRect(fName).bottomRight(),
                             QPointF(v.x(), h-v.y()));

            painter.drawText(QPointF(x, i*20), QString("%1").arg(fName));
        }
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
    auto vertices = (float*)rtcSetNewGeometryBuffer(rtGeometry, RTC_BUFFER_TYPE_VERTEX,0,RTC_FORMAT_FLOAT3,sizeof(ON_3fPoint), nVs);


    mesh.m_V.Reserve(nVs);
    mesh.m_V.SetCount(nVs);
    for (int i=0; i<int(nVs); ++i, ++pptr, ++vertices/*, ++nptr*/ ){
        mesh.m_V.At(i)->Set((*pptr)[0],(*pptr)[1],(*pptr)[2]);
        *vertices = (*pptr)[0];
        *++vertices = (*pptr)[1];
        *++vertices = (*pptr)[2];
    }

    auto const nFs = m->n_faces();
    mesh.m_F.Reserve(nFs);
    mesh.m_F.SetCount(nFs);
    auto  pF = mesh.m_F.First();
    auto triangles = (uint*)rtcSetNewGeometryBuffer(rtGeometry, RTC_BUFFER_TYPE_INDEX,0,RTC_FORMAT_UINT3, 12, nFs);
    for ( const auto &f : m->faces()){
        auto pv =  pF->vi;
        for ( const auto &v : f.vertices()){
            (*triangles++) = v.idx();
            (*pv++) = v.idx();
        }
        pF->vi[3] = pF->vi[2];
        pF++;
    }

    rtcCommitGeometry( rtGeometry );
    rtGeomID = rtcAttachGeometry( rtScene, rtGeometry );
    rtcReleaseGeometry( rtGeometry );

    rtcCommitScene( rtScene );
    return true;
}

bool LP_HumanFeature::member::pickFeaturePoint(QPoint &&pickPos)
{
    auto view = cam->ViewMatrix(),
         proj = cam->ProjectionMatrix(),
         vp = cam->ViewportMatrix();
    QVector3D cpos = view.inverted()*QVector3D(0.0f,0.0f,0.0f);
    QVector3D _ray(pickPos.x(),                         //Mouse click screen space position
                   cam->ResolutionY() - pickPos.y(),
                   0.0f);
    _ray = (vp * proj * view).inverted() * _ray;        //Inverted back to world space

    QVector3D cdir = (_ray - cpos).normalized();


    RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    RTCRayHit rayhit;
    RTCRay &ray = rayhit.ray;
    ray.org_x = cpos.x();
    ray.org_y = cpos.y();
    ray.org_z = cpos.z();

    ray.dir_x = cdir.x();
    ray.dir_y = cdir.y();
    ray.dir_z = cdir.z();

    ray.tnear = 0.01f;
    ray.tfar = std::numeric_limits<float>::infinity();

    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(rtScene,
                  &context,
                  &rayhit);

    pickPoint.reset();
    if ( RTC_INVALID_GEOMETRY_ID == rayhit.hit.geomID ){
        qDebug() << "Hit nothing : " << rayhit.hit.geomID;

        return false;
    }
    qDebug() << QString("Hit : %1 (%2, %3)").arg(rayhit.hit.primID)
                .arg(rayhit.hit.u).arg(rayhit.hit.v);

    auto pF = mesh.m_F.At(rayhit.hit.primID);
    pickPoint = std::make_shared<FeaturePoint>();
    pickPoint->mName = "P_x";
    pickPoint->mParams = {pF->vi[1],    //u
                          pF->vi[2],    //v
                          pF->vi[0],    //w = 1-u-v
                          rayhit.hit.u, rayhit.hit.v};
    return  true;
}

QVector3D LP_HumanFeature::member::evaluationFeaturePoint(const ON_Mesh &m, const FeaturePoint &fp)
{
    const auto &nVs = m.VertexCount();
    if ( 0 >= nVs ){
        qDebug() << "Invalid Reference Mesh!";
        return QVector3D();
    }
    auto &params = fp.mParams;
    const int &i = std::get<0>(params),
              &j = std::get<1>(params),
              &k = std::get<2>(params);
    const double &u = std::get<3>(params),
                 &v = std::get<4>(params);
    const double w = 1.0-u-v;
    if ( i >= nVs || j >= nVs || k >= nVs ){
        qDebug() << "Invalid Feature Point!";
        return QVector3D();   //Did not check 1=u+v+w
    }
    auto _p = u * mesh.m_V[i] + v * mesh.m_V[j] + w * mesh.m_V[k];
    return QVector3D(_p.x, _p.y, _p.z);
}

void LP_HumanFeature::member::initializeEmbree3()
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    rtDevice = rtcNewDevice("verbose=1");
    rtScene = rtcNewScene(rtDevice);
    rtGeometry = rtcNewGeometry(rtDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
}

void LP_HumanFeature::member::importFeatures(const QString &filename)
{

    QFile file(filename);
    if ( !file.open(QIODevice::ReadOnly)){
        qDebug()  << file.errorString();
        return;
    }
    QJsonDocument jDoc =  QJsonDocument::fromJson(file.readAll());
    file.close();
    //For feature points
    auto jFPs = jDoc["FeaturePoints"].toArray();
    for ( auto &&jFp : jFPs){
        auto info = jFp.toString().split(",");
        if ( 6 != info.size()){
            qWarning() << "Corrupt Feature Point : " <<info;
            continue;
        }
        FeaturePoint fp;
        fp.mName = info.at(0).toStdString();
        fp.mParams = {info.at(1).toInt(),
                     info.at(2).toInt(),
                     info.at(3).toInt(),
                     info.at(4).toDouble(),
                     info.at(5).toDouble()};
        featurePoints.emplace_back(std::move(fp));
    }

}

void LP_HumanFeature::member::exportFeatures(const QString &filename)
{
    if ( filename.isEmpty()){
        return;
    }
    //Prepare the daata
    QJsonObject jObj;
    QJsonArray jFpts;
    for ( auto &p : featurePoints ){
        jFpts.append(QString("%6,%1,%2,%3,%4,%5")
                     .arg(std::get<0>(p.mParams))
                     .arg(std::get<1>(p.mParams))
                     .arg(std::get<2>(p.mParams))
                     .arg(std::get<3>(p.mParams))
                     .arg(std::get<4>(p.mParams))
                     .arg(p.mName.data()));
    }

    jObj["FeaturePoints"] = jFpts;
    QJsonDocument jDoc(jObj);

    QFile file(filename);
    if ( !file.open(QIODevice::WriteOnly)){
        qDebug()  << file.errorString();
        return;
    }
    file.write(jDoc.toJson());
    file.close();

}

std::vector<QVector3D> LP_HumanFeature::member::get3DFeaturePoints()
{
    const auto &nVs = mesh.VertexCount();
    if ( 0 >= nVs ){
        return std::vector<QVector3D>();
    }
    std::vector<QVector3D> pts;
    for ( auto &fp : featurePoints ){
        pts.emplace_back(
                    evaluationFeaturePoint(mesh, fp)
                    );
    }
    return pts;
}
