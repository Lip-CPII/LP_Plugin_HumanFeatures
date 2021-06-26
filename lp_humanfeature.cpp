#include "lp_humanfeature.h"

#include "xmmintrin.h"
#include "pmmintrin.h"


#include "MeshPlaneIntersect.hpp"

#include "embree3/rtcore.h"
#include "embree3/rtcore_common.h"
#include "embree3/rtcore_ray.h"

#include "extern/opennurbs/opennurbs.h"

#include "lp_openmesh.h"
#include "lp_renderercam.h"
#include <iostream>

#include "renderer/lp_glselector.h"

#include <QInputDialog>
#include <QStandardItemModel>
#include <QTreeView>
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

template<> std::vector<MeshPlaneIntersect<float,int>::Mesh::EdgePath> MeshPlaneIntersect<float,int>::Mesh::o_edgePaths = std::vector<EdgePath>();


const QStringList gShimaFeaturesList = {
   "T001",
   "T002",
   "T003",
   "T026",  /*T006 Equivalent*/
   "T011",
   "T028",
   "T022",
   "T063",
   "T016",
   "T006",  /*T026 Equivalent*/
   "T023",
   "T014"
};



struct LP_HumanFeature::member {
    RTCRayHit rayCast(const QVector3D &rayOrg, const QVector3D &rayDir);

    bool convert2ON_Mesh(LP_OpenMesh opMesh);

    bool pickFeaturePoint(QPoint &&pickPos);
    bool pickFeatureGirth();
    bool projFeatureCurve(const std::vector<QPoint> &projline, const QString &name);
    QVector3D evaluationFeaturePoint(const ON_Mesh &m, const FeaturePoint &fp);
    std::vector<QVector3D> evaluationFeatureGirth(const ON_Mesh &m, const FeatureGirth &fg);
    void initializeEmbree3();

    void importFeatures(const QString &filename);
    void exportFeatures(const QString &filename);

    double getShimaMeasurement(const QString &shimaFt );
    void exportSizeChart(const QString &filename);
    double pointName2Measurements(QStringList composite, std::string type);

    double measurement_T001();
    double measurement_T002();
    double measurement_T003();
    double measurement_T026();
    double measurement_T011();
    double measurement_T028();
    double measurement_T022();
    double measurement_T063();
    double measurement_T016();
    double measurement_T006();
    double measurement_T023();
    double measurement_T014();

    RTCDevice rtDevice;
    RTCScene rtScene;
    RTCGeometry rtGeometry;
    unsigned int rtGeomID;
    std::vector<uint> fids;

    QMap<QString, std::function<double()>> shimaFeatures;

    std::shared_ptr<FeaturePoint> pickPoint;
    std::vector<std::shared_ptr<FeaturePoint>> pickCurve;
    std::shared_ptr<FeatureGirth> pickGirth; //Temporary feature girth
    std::vector<QPoint> projectLine;

    std::vector<QVector3D> get3DFeaturePoints();
    std::vector<std::vector<QVector3D>> get3DFeatureCurves();
    std::vector<std::vector<QVector3D>> get3DFeatureGirths();

    //=================Singa===================//
    double getCurveLength(const FeatureCurve &curve);
    double getP2CLength(const FeaturePoint &point,const FeatureCurve &curve);
    double getP2PLength(const FeaturePoint &pointA,const FeaturePoint &pointB);
    //=================June 25===============//
<<<<<<< HEAD

=======
>>>>>>> 53eb3fd0cd860adc6400ce1964391478baf64c59
    ON_Mesh mesh;
    std::vector<FeaturePoint> featurePoints;
    std::vector<FeatureCurve> featureCurves;
    std::vector<FeatureGirth> featureGirths;
    LP_RendererCam cam;

    std::function<void()> updateFPsList;
    std::function<void()> updateFCsList;
    std::function<void()> updateFGsList;
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
                        emit glUpdateRequest();
                        break;
                    }
                }
                return true;
            }else{
                mMember->pickFeaturePoint(e->pos());
                if ("Girths" == mCB_FeatureType->currentText()){
                    mMember->pickFeatureGirth();
                }
//                std::cout<<"girth test"<<std::endl;
                emit glUpdateRequest();
                return true;
            }
        }else if ( e->button() == Qt::RightButton ){
            //g_GLSelector->Clear();
            mObject = LP_Objectw();
            mMember->mesh.Destroy();
            mMember->projectLine.clear();
            mMember->pickCurve.clear();
            mMember->pickPoint.reset();
            mMember->pickGirth.reset();
            mLabel->setText("Select A Mesh");

            emit glUpdateRequest();
        }else if (e->button() == Qt::MiddleButton ){
            mShowLabels = true;
            emit glUpdateRequest();
        }
    } else if ( QEvent::MouseMove == event->type()) {
        auto e = static_cast<QMouseEvent*>(event);
        if ( e->buttons() == Qt::MiddleButton ){
            mShowLabels = false;
        }
    } else if ( QEvent::MouseButtonPress == event->type()) {
        if (mObject.lock()){
            auto e = static_cast<QMouseEvent*>(event);
            if ( e->button() == Qt::LeftButton ) {
                if ( "Curves" == mCB_FeatureType->currentText()){
                    if ( 2 == mMember->projectLine.size()){
                        mMember->projectLine.back() = e->pos();
                    } else {
                        mMember->projectLine.emplace_back(e->pos());
                    }
                }
            }
        }
    } else if ( QEvent::KeyPress == event->type()){
        auto e = static_cast<QKeyEvent*>(event);
        switch (e->key()) {
            case Qt::Key_Return:
            case Qt::Key_Space:
            case Qt::Key_Enter:
            if ( "Points" == mCB_FeatureType->currentText()) {
                auto name = QInputDialog::getText(0, "Inp ut", "Feature Name");  //Ask for featture name
                if ( name.isEmpty()) {
                    break;
                }
                mMember->pickPoint->mName = name.toStdString();
                mMember->featurePoints.emplace_back( *mMember->pickPoint.get());
                mMember->pickPoint.reset(); //Reset the pick Point
                mMember->updateFPsList();   //Update the listview
                emit glUpdateRequest();
            } else if ( "Curves" == mCB_FeatureType->currentText()) {
                auto name = QInputDialog::getText(0, "Input", "Feature Name");  //Ask for featture name
                if ( name.isEmpty()) {
                    break;
                }
                mMember->projFeatureCurve(mMember->projectLine, name);
                mMember->updateFCsList();   //Update the listview
                mMember->projectLine.clear();
                emit glUpdateRequest();
            } else if ( "Girths" == mCB_FeatureType->currentText()){
                //Feature Girths
                auto name = QInputDialog::getText(0, "Input", "Feature Name");  //Ask for featture name
                if ( name.isEmpty()) {
                    break;
                }
                mMember->pickGirth->mName = name.toStdString();
                mMember->featureGirths.emplace_back( *mMember->pickGirth.get());
                mMember->pickGirth.reset(); //Reset the pick Point
                mMember->updateFGsList();   //Update the listview
                emit glUpdateRequest();

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
    QStringList types={"Points", "Curves", "Girths", "All"};
    mCB_FeatureType = new QComboBox(widget);   //Type
    mCB_FeatureType->addItems(types);
    mCB_FeatureType->setCurrentIndex(3);

    auto pFPGroupBox = new QGroupBox("Feature Points");
    auto fpLayout = new QVBoxLayout;
    auto Treeview_pt = new QTreeView;

    fpLayout->addWidget(Treeview_pt);
    pFPGroupBox->setLayout(fpLayout);

    auto pFCGroupBox = new QGroupBox("Feature Curves");
    auto fcLayout = new QVBoxLayout;
    auto Treeview_curv = new QTreeView;

    fcLayout->addWidget(Treeview_curv);
    pFCGroupBox->setLayout(fcLayout);

    auto pFGGroupBox = new QGroupBox("Feature Girths");
    auto fgLayout = new QVBoxLayout;
    auto Treeview_girth = new QTreeView;

    fgLayout->addWidget(Treeview_girth);
    pFGGroupBox->setLayout(fgLayout);

    QPushButton *pb_export_sizechart = new QPushButton("Export SizeChart");

    QPushButton *pb_import_features = new QPushButton("Import Features");
    QPushButton *pb_export_features = new QPushButton("Export Features");
    layout->addWidget(mLabel);
    layout->addWidget(pb_import_features);
    layout->addWidget(mCB_FeatureType);
    layout->addWidget(pFPGroupBox);
    layout->addWidget(pFCGroupBox);
    layout->addWidget(pFGGroupBox);
    layout->addWidget(pb_export_features);
    layout->addWidget(pb_export_sizechart);

    layout->addStretch();

    widget->setLayout(layout);

    connect(pb_export_sizechart, &QPushButton::clicked, [this]() mutable {
        auto filename = QFileDialog::getSaveFileName(nullptr, "Export", QDir::currentPath(),
                                                     "CSV (*.csv)");
        mMember->exportSizeChart(filename);
        emit glUpdateRequest();

    });

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
        mMember->updateFPsList();
        mMember->updateFCsList();
        mMember->updateFGsList();
        emit glUpdateRequest();
    });

    connect(mCB_FeatureType, &QComboBox::currentIndexChanged, [pFPGroupBox](const int &id){
        pFPGroupBox->setHidden( 0 != id );
    });
    connect(mCB_FeatureType, &QComboBox::currentIndexChanged, [pFCGroupBox](const int &id){
        pFCGroupBox->setHidden( 1 != id );
    });
    connect(mCB_FeatureType, &QComboBox::currentIndexChanged, [pFGGroupBox](const int &id){
        pFGGroupBox->setHidden( 2 != id );
    });

    connect(mCB_FeatureType, &QComboBox::currentIndexChanged, [this](){
        emit glUpdateRequest();
    });

    //For feature points update
    mMember->updateFPsList = [this,Treeview_pt](){
        auto model = new QStandardItemModel();
        model->setHorizontalHeaderLabels({"Name","Indices","UV"});

        for ( auto &p : mMember->featurePoints ){

            auto iName = new QStandardItem(p.mName.c_str());
            iName->setData(QVariant::fromValue<void*>(&p)); //Store pointer to FeaturePoint for eitiing from  the treeview
            auto iIndices = new QStandardItem(QString("%1,%2,%3")
                                            .arg(std::get<0>(p.mParams))
                                            .arg(std::get<1>(p.mParams))
                                            .arg(std::get<2>(p.mParams)));
            iIndices->setData(QVariant::fromValue<void*>(&p));
            iIndices->setEditable(false);
            auto iUV = new QStandardItem(QString("%1,%2")
                                            .arg(std::get<3>(p.mParams))
                                            .arg(std::get<4>(p.mParams)));

            iUV->setData(QVariant::fromValue<void*>(&p));
            iUV->setEditable(false);
            model->appendRow({iName,iIndices,iUV});
        }
        delete Treeview_pt->model();
        Treeview_pt->reset();
        Treeview_pt->setModel(model);
        connect(model, &QStandardItemModel::dataChanged,
                [model,this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles){
            Q_UNUSED(bottomRight)
            Q_UNUSED(roles)
            auto item = model->itemFromIndex(topLeft);
            if ( 0 != item->column()){
                return;
            }
            auto pF = static_cast<FeaturePoint*>(item->data().value<void*>());
            pF->mName = item->text().toStdString();
            emit glUpdateRequest();
        });
    };
    //For feature curve's update
    mMember->updateFCsList = [this,Treeview_curv](){
        auto model = new QStandardItemModel();
        model->setHorizontalHeaderLabels({"Name","# Points"});

        for ( auto &c : mMember->featureCurves ){

            auto iName = new QStandardItem(c.mName.c_str());
            iName->setData(QVariant::fromValue<void*>(&c)); //Store pointer to FeaturePoint for eitiing from  the treeview
            auto iPoints = new QStandardItem(QString("#%1").arg(c.mCurve.size()));
            iPoints->setData(QVariant::fromValue<void*>(&c));
            iPoints->setEditable(false);
            model->appendRow({iName,iPoints});
        }
        delete Treeview_curv->model();
        Treeview_curv->reset();
        Treeview_curv->setModel(model);
        connect(model, &QStandardItemModel::dataChanged,
                [model,this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles){
            Q_UNUSED(bottomRight)
            Q_UNUSED(roles)
            auto item = model->itemFromIndex(topLeft);
            if ( 0 != item->column()){
                return;
            }
            auto pF = static_cast<FeatureCurve*>(item->data().value<void*>());
            pF->mName = item->text().toStdString();
            emit glUpdateRequest();
        });
    };

    //For feature girths update
    mMember->updateFGsList = [this,Treeview_girth](){
        auto model = new QStandardItemModel();
        model->setHorizontalHeaderLabels({"Name","Edges","Scales"});

        for ( auto &fg : mMember->featureGirths ){

            auto iName = new QStandardItem(fg.mName.c_str());
            iName->setData(QVariant::fromValue<void*>(&fg)); //Store pointer to FeaturePoint for eitiing from  the treeview
            auto Edges = new QStandardItem(QString("%1,%2")
                                            .arg(std::get<0>(fg.mParams.front()))
                                            .arg(std::get<1>(fg.mParams.front())));
            Edges->setData(QVariant::fromValue<void*>(&fg));
            Edges->setEditable(false);
            auto iscale = new QStandardItem(QString("%1")
                                            .arg(std::get<2>(fg.mParams.front())));

            iscale->setData(QVariant::fromValue<void*>(&fg));
            iscale->setEditable(false);
            model->appendRow({iName,Edges,iscale});
        }
        delete Treeview_girth->model();
        Treeview_girth->reset();
        Treeview_girth->setModel(model);
        connect(model, &QStandardItemModel::dataChanged,
                [model,this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles){
            Q_UNUSED(bottomRight)
            Q_UNUSED(roles)
            auto item = model->itemFromIndex(topLeft);
            if ( 0 != item->column()){
                return;
            }
            auto pF = static_cast<FeaturePoint*>(item->data().value<void*>());
            pF->mName = item->text().toStdString();
            emit glUpdateRequest();
        });
    };

    return widget;
}

bool LP_HumanFeature::Run()
{
    for ( auto &sf : gShimaFeaturesList ){
        if ( mMember->shimaFeatures.contains(sf)){
            qWarning() << "Feature redefined : " << sf;
            continue;
        }
        mMember->shimaFeatures[sf] = nullptr;
    }

    mMember->initializeEmbree3();
    emit glUpdateRequest();
    return false;
}

void LP_HumanFeature::FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    if (!mInitialized || !mObject.lock()){
        initializeGL();
        mMember->cam = cam;
        return;
    }

    QMatrix4x4 view = cam->ViewMatrix(),
               proj = cam->ProjectionMatrix();
    QMatrix4x4 tmp;
    tmp.setToIdentity();
    const auto sqrtEps = std::sqrt(std::numeric_limits<float>::epsilon());
    tmp.translate(QVector3D(0.0f,0.0f,-sqrtEps));

    auto f = ctx->extraFunctions();
    f->glEnable(GL_BLEND);
    f->glEnable(GL_CULL_FACE);
    fbo->bind();

    mProgram->bind();
    mProgram->setUniformValue("m4_view", view);
    mProgram->setUniformValue("m4_mvp", tmp*proj*view);
    mProgram->setUniformValue("m3_normal", view.normalMatrix());
    mProgram->setUniformValue("v3_color", QVector3D(0.4,0.4,0.4));

    mProgram->enableAttributeArray("a_pos");
    //mProgram->enableAttributeArray("a_norm");

    mProgram->setAttributeArray("a_pos", &mMember->mesh.m_V.First()->x,3);
    //mProgram->setAttributeArray("a_norm", &mMember->mesh.m_N.First()->x,3);


    mProgram->setUniformValue("v3_color", QVector3D(0.2,1.0,0.9));

    f->glDrawElements(GL_TRIANGLES, mMember->fids.size(), GL_UNSIGNED_INT,
                      mMember->fids.data());

    f->glDisable(GL_CULL_FACE);

    //f->glDepthRangef(-sqrtEps, 1.0f - sqrtEps);

    f->glDepthFunc(GL_LEQUAL);
    mProgram->setUniformValue("m4_mvp", proj*view);

//    f->glDrawElements(GL_LINES, mMember->evid.size(), GL_UNSIGNED_INT,
//                      mMember->evid.data());

    mProgram->disableAttributeArray("a_pos");
    mProgram->disableAttributeArray("a_norm");

    mProgram->release();    //Release the mesh program

    mProgramFeatures->bind(); //Bind the feature curve program
    mProgramFeatures->enableAttributeArray("a_pos");


//    auto ratio = 0.01f/(cam->Roll()-cam->Target()).length();

//    tmp.translate(QVector3D(0.0f,0.0f,-ratio));
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

    if ( mMember->pickGirth ){  //Draw the temporary pick girth
        auto girth = mMember->evaluationFeatureGirth(mMember->mesh,
                                                  *mMember->pickGirth.get());
        mProgramFeatures->setUniformValue("v4_color", QVector4D(0.8, 0.8, 0.2, 0.6));
        mProgramFeatures->setAttributeArray("a_pos", girth.data());
        f->glDrawArrays(GL_LINE_LOOP, 0, girth.size());
    }


    if ( "Points" == mCB_FeatureType->currentText()
         || "All" == mCB_FeatureType->currentText()) {//Draw Feature Points

        mProgramFeatures->setUniformValue("v4_color", QVector4D(0.2, 0.8, 0.2, 0.6));
        f->glEnable(GL_PROGRAM_POINT_SIZE);     //Enable point-size controlled by shader
        auto &&fpts = mMember->get3DFeaturePoints();
        mProgramFeatures->setAttributeArray("a_pos", fpts.data());
        f->glDrawArrays(GL_POINTS, 0, fpts.size());
    }

    if ( "Curves" == mCB_FeatureType->currentText()
         || "All" == mCB_FeatureType->currentText()) {//Draw Feature Curves

        f->glLineWidth(5.f);
        mProgramFeatures->setUniformValue("v4_color", QVector4D(1.0, 0.6, 0.4, 0.6));
        auto &&fcs = mMember->get3DFeatureCurves();
        for ( auto &fc : fcs ){
            mProgramFeatures->setAttributeArray("a_pos", fc.data());
            f->glDrawArrays(GL_LINE_STRIP, 0, fc.size());
        }
        f->glLineWidth(1.f);
    }

    if ( "Girths" == mCB_FeatureType->currentText()
         || "All" == mCB_FeatureType->currentText()) {//Draw Feature Curves

        f->glLineWidth(5.f);
        mProgramFeatures->setUniformValue("v4_color", QVector4D(1.0, 0.6, 0.4, 0.6));
        auto &&fgs = mMember->get3DFeatureGirths();
        for ( auto &fg : fgs ){
            mProgramFeatures->setAttributeArray("a_pos", fg.data());
            f->glDrawArrays(GL_LINE_LOOP, 0, fg.size());
        }
        f->glLineWidth(1.f);
    }



    mProgramFeatures->disableAttributeArray("a_pos");
    mProgramFeatures->release();

    f->glDepthRangef(0.0f, 1.0f);

    f->glDisable(GL_BLEND);
    fbo->release();
}

void LP_HumanFeature::PainterDraw(QWidget *glW)
{
    if ( "window_Normal" == glW->objectName() || !mMember->cam
         || !mShowLabels){
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

//    const QVector3D cpos = view.inverted()*QVector3D(0.0f,0.0f,0.0f);
    int i=1;

    if ( "Points" == mCB_FeatureType->currentText()
         || "All" == mCB_FeatureType->currentText()){
        auto &&pts = mMember->get3DFeaturePoints();
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
    if ( "Curves" == mCB_FeatureType->currentText()
         || "All" == mCB_FeatureType->currentText()) {
        auto &&curvs = mMember->get3DFeatureCurves();

        painter.setPen(qRgb(200,100,100));
        int j=1;
        for ( auto &c : curvs ){
            auto &&p = c.front();
            auto v = view * p;
            QString fName(mMember->featureCurves[j-1].mName.c_str());
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
            ++i; ++j;
        }
    }

    QPen pen(qRgb(220,255,100));
    pen.setWidth(3);
    painter.setPen(pen);
    painter.drawPolyline(mMember->projectLine.data(), mMember->projectLine.size());
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
            "   gl_FragColor = vec4(color,0.2);\n"
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

RTCRayHit LP_HumanFeature::member::rayCast(const QVector3D &rayOrg, const QVector3D &rayDir)
{
    RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    RTCRayHit rayhit;
    RTCRay &ray = rayhit.ray;
    ray.org_x = rayOrg.x();
    ray.org_y = rayOrg.y();
    ray.org_z = rayOrg.z();

    ray.dir_x = rayDir.x();
    ray.dir_y = rayDir.y();
    ray.dir_z = rayDir.z();

    ray.tnear = 0.001f;
    ray.tfar = std::numeric_limits<float>::infinity();

    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(rtScene,
                  &context,
                  &rayhit);

    return rayhit;
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
    fids.clear();
    fids.resize(nFs*3);
    auto fit = fids.begin();
    for ( const auto &f : m->faces()){
        auto pv =  pF->vi;
        for ( const auto &v : f.vertices()){
            (*triangles++) = v.idx();
            (*pv++) = v.idx();
            *fit++ = v.idx();
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
    int h = cam->ResolutionY();
    QVector3D cpos(0.0f,0.0f,0.0f);
    QVector3D cdir(0.0f,0.0f,-1.0f);
    if ( cam->IsPerspective()){
        cpos = view.inverted()*QVector3D(0.0f,0.0f,0.0f);
        QVector3D _ray(pickPos.x(),                         //Mouse click screen space position
                       cam->ResolutionY() - pickPos.y(),
                       0.0f);
        _ray = (vp * proj * view).inverted() * _ray;        //Inverted back to world space
        cdir = (_ray - cpos).normalized();
    } else {
        cpos = (vp * proj * view).inverted()*QVector3D(pickPos.x(), h-pickPos.y(),0.0f);
        QMatrix4x4 tmp = view.inverted();
        tmp(0,3) = tmp(1,3) = tmp(2,3) = 0.0f;
        cdir = tmp * cdir;
    }

    RTCRayHit &&rayhit = rayCast(cpos, cdir);

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
    pickPoint->mParams = {pF->vi[0],    //w = 1-u-v
                          pF->vi[1],    //u
                          pF->vi[2],    //v
                          rayhit.hit.u, rayhit.hit.v};
    return  true;
}

bool LP_HumanFeature::member::pickFeatureGirth()
{
//    std::vector<QVector3D> fpts = get3DFeaturePoints();
    QVector3D pos = evaluationFeaturePoint(mesh, *pickPoint);

    typedef MeshPlaneIntersect<float, int> Intersector;
    std::vector<Intersector::Vec3D> vertices;
    for(int n =0; n<mesh.VertexCount(); n++)
    {
        auto v = mesh.Vertex(n);
        vertices.push_back({float(v.x), float(v.y), float(v.z)});
        }

    std::vector<Intersector::Face> faces;
    for(int n =0; n<mesh.FaceCount(); n++)
    {
        auto fv = mesh.m_F.At(n)->vi;
        faces.push_back({fv[0], fv[1], fv[2]});
        }

    Intersector::Mesh Imesh(vertices, faces);
    Intersector::Plane plane;

    plane.origin = {pos.x(), pos.y(), pos.z()};
    plane.normal = {0, 1, 0};

    auto result = Imesh.Intersect(plane);


    // pick one girth with maximum size
    unsigned long max = 0;
    int max_idx = 0;
    for(unsigned long n =0; n<result.size(); ++n)
    {
        if(max < result[n].points.size()){
            max_idx = n;
            max = result[n].points.size();
        }
    }

    std::vector<MeshPlaneIntersect<float,int>::Mesh::EdgePath> edgepaths = Intersector::Mesh::o_edgePaths;
    MeshPlaneIntersect<float,int>::Mesh::EdgePath ep = edgepaths[max_idx];
//    std::cout<<"ep size:"<<ep.size()<<std::endl;
    std::vector<float> scales = result[max_idx].factors;

    pickGirth = std::make_shared<FeatureGirth>();
    pickGirth->mName = "G_x";

//    std::cout<<"scale size:"<<scales.size()<<std::endl;
//    std::cout<<"point size:"<< result[max_idx].points.size() <<std::endl;
    for (unsigned long m=0; m<result[max_idx].points.size(); ++m)
    {
//        auto p = result[max_idx].points[m];
//        QVector3D pt;
//        pt.setX(p[0]);
//        pt.setY(p[1]);
//        pt.setZ(p[2]);
//        pts.emplace_back(pt);

        auto e = ep[m+1]; // important!
        std::tuple params = std::make_tuple(e.first, e.second, scales[m]);
        pickGirth->mParams.emplace_back(params);

    }
//    std::cout<<"params"<<std::get<0>(pickGirth->mParams.front())<<std::endl;
    return  true;
}

bool LP_HumanFeature::member::projFeatureCurve(const std::vector<QPoint> &projline, const QString& name)
{
    if ( 2 > projline.size()){
        return false;
    }
    QMatrix4x4 view = cam->ViewMatrix(),
               proj = cam->ProjectionMatrix(),
               vp = cam->ViewportMatrix();

    vp = ( vp ).inverted();    //
    view = proj * view;
    int w = cam->ResolutionX(),
        h = cam->ResolutionY();

    auto pit = projline.begin();
    auto pend = projline.cend();
    auto p = &*pit++;
    ON_Xform on_trans;
    on_trans[0][0] = view(0,0);   on_trans[0][1] = view(0,1);   on_trans[0][2] = view(0,2);   on_trans[0][3] = view(0,3);
    on_trans[1][0] = view(1,0);   on_trans[1][1] = view(1,1);   on_trans[1][2] = view(1,2);   on_trans[1][3] = view(1,3);
    on_trans[2][0] = view(2,0);   on_trans[2][1] = view(2,1);   on_trans[2][2] = view(2,2);   on_trans[2][3] = view(2,3);
    on_trans[3][0] = view(3,0);   on_trans[3][1] = view(3,1);   on_trans[3][2] = view(3,2);   on_trans[3][3] = view(3,3);

    auto pointList = mesh.m_V;
    ON_TransformPointList(3, false, mesh.VertexCount(), 3, &pointList.First()->x, on_trans);
    ON_3dPointListRef pointListRef = ON_3dPointListRef::FromFloatArray(mesh.VertexCount(),
                                                                       3,
                                                                       &pointList.First()->x);
    auto &topo = mesh.Topology();

    QHash<int, std::pair<bool, FeaturePoint>> txEdges;

    //Perofrm work in perspective space
    for (; pit != pend; ++pit){
        auto sp0 = vp * QVector3D(p->x(), h - p->y(), 0.0f),//Project screen point to perspective space
             sp1 = vp * QVector3D(pit->x(), h - pit->y(), 0.0f);

        const ON_Line projSeg(ON_2dPoint(sp0.x(), sp0.y()), ON_2dPoint(sp1.x(), sp1.y()));
        auto e_end = topo.m_tope.Last();
        e_end++;
        for ( auto e = topo.m_tope.First(); e != e_end; ++e ){
            ON_3fPoint p0 = *pointList.At(topo.m_topv.At(e->m_topvi[0])->m_vi[0]),
                       p1 = *pointList.At(topo.m_topv.At(e->m_topvi[1])->m_vi[0]);

            if (( -1.0 > p0.x && -1.0 > p1.x ) || ( 1.0 < p0.x && 1.0 < p1.x )){//Outside viewing frustum
                continue;
            }
            if (( -1.0 > p0.y && -1.0 > p1.y ) || ( 1.0 < p0.y && 1.0 < p1.y )){//Outside viewing frustum
                continue;
            }
            if (( -1.0 > p0.z && -1.0 > p1.z ) || ( 1.0 < p0.z && 1.0 < p1.z )){//Outside viewing frustum
                continue;
            }

            double a, b;
            bool rc = ON_IntersectLineLine(ON_Line(ON_2dPoint(p0.x, p0.y), ON_2dPoint(p1.x, p1.y)),
                                                   projSeg, &a, &b, 1e-6, true);
            if ( rc ){
                //Check front-face
                int fid = -1;
                for ( auto i=0;  i < e->m_topf_count; ++i ){// 2 for 2-manifold
                    auto tmpF = mesh.m_F.At(e->m_topfi[i]);

                    ON_3dVector n;
                    Q_ASSERT(tmpF->ComputeFaceNormal(pointListRef, n));
                    if ( std::numeric_limits<float>::epsilon() <= ON_3dVector::DotProduct(n, ON_3dVector(0.0,0.0,1.0))){
                        fid = e->m_topfi[i];
                        break;
                    }
                }
                if ( 0 > fid ){
                    qWarning() << "Back face";
                    continue;
                }
                const auto teid = int(e - topo.m_tope.First());
                if  ( txEdges.contains(teid) ){
                    Q_ASSERT(false);
                    continue;
                }
                auto pF = mesh.m_F.At(fid);

                auto tf = topo.m_topf.At(e->m_topfi[0]);

                int i;
                for ( i=0; i<3; ++i){    //Assume triangle
                    if ( teid == tf->m_topei[i]){
                        break;
                    }
                }
                if ( 3 <= i ){
                    qCritical() << "Error : Unknown edge";
                    continue;
                }

                auto vi = pF->vi;
                Q_ASSERT(vi);
                double u = 0.0, v = 0.0;

                switch (i) {
                case 0:{
                    v = tf->m_reve[0] ? a : 1.0 - a;
                    break;
                }
                case 1:{
                    u = tf->m_reve[1] ? 1.0 - a : a;
                    break;
                }
                case 2:{
                    u = tf->m_reve[2] ? a : 1.0 - a;
                    v = 1.0 - u;
                    break;
                }
                default:
                    Q_ASSERT(false);
                    break;
                }
                FeaturePoint fp;

                fp.mName = std::to_string(teid);    //Store the edge index
                fp.mParams = {vi[0],vi[1],vi[2],u,v};
                txEdges[teid] = {false, fp};

            }
        }
        p = &*pit;
    }

    auto txIt = txEdges.begin();
    auto txEnd = txEdges.end();


    std::function<void(const int &, const int &,std::vector<FeaturePoint>&)> propagateChain;
    propagateChain =
            [&topo, &txEdges, &propagateChain](const int &eid,
            const int &prevfid,
            std::vector<FeaturePoint> &chain)
    {
        auto te = topo.m_tope.At(eid);
        auto txEnd = txEdges.end();

        for ( int i=0; i<te->m_topf_count; ++i  ){
            if ( prevfid == te->m_topfi[i]){
                continue;
            }
            auto tf = topo.m_topf.At(te->m_topfi[i]);
            for ( int j=0; j<3; ++j ){
                if ( eid == tf->m_topei[j]) {
                    continue;
                }
                auto _txIt  = txEdges.find(tf->m_topei[j]);
                if ( _txIt != txEnd ) {
                    if ( _txIt.value().first){
                        continue;
                    }
                    _txIt.value().first = true;
                    chain.emplace_back(std::move(_txIt.value().second));

                    propagateChain(_txIt.key(), te->m_topfi[i], chain);
                    break;
                }
            }
        }
    };


    view = cam->ViewMatrix();
    proj = cam->ProjectionMatrix();
    vp = cam->ViewportMatrix();

    std::vector<FeaturePoint> projPoints;

    pit = projline.begin();

    for (; pit != pend; ++pit) {
        QVector3D cpos(0.0f,0.0f,0.0f);
        QVector3D cdir(0.0f,0.0f,-1.0f);
        if ( cam->IsPerspective()){
            cpos = view.inverted()*QVector3D(0.0f,0.0f,0.0f);
            QVector3D _ray(pit->x(),                         //Mouse click screen space position
                           h - pit->y(),
                           0.0f);
            _ray = (vp * proj * view).inverted() * _ray;        //Inverted back to world space
            cdir = (_ray - cpos).normalized();
        } else {
            cpos = (vp * proj * view).inverted()*QVector3D(pit->x(), h-pit->y(),0.0f);
            QMatrix4x4 tmp = view.inverted();
            tmp(0,3) = tmp(1,3) = tmp(2,3) = 0.0f;
            cdir = tmp * cdir;
        }

        RTCRayHit &&rayhit = rayCast(cpos, cdir);


        FeaturePoint fp;
        fp.mName = "X";
        if ( RTC_INVALID_GEOMETRY_ID == rayhit.hit.geomID ){
            auto pF = mesh.m_F.At(0);
            fp.mParams = {pF->vi[0],    //w = 1-u-v
                          pF->vi[1],    //u
                          pF->vi[2],    //v
                          rayhit.hit.u, rayhit.hit.v};
        }else{
            auto pF = mesh.m_F.At(rayhit.hit.primID);
            fp.mName = std::to_string(rayhit.hit.primID);
            fp.mParams = {pF->vi[0],    //w = 1-u-v
                          pF->vi[1],    //u
                          pF->vi[2],    //v
                          rayhit.hit.u, rayhit.hit.v};
        }

        projPoints.push_back(std::move(fp));
    }

    for ( ; txIt != txEnd; ++txIt ){
        if ( txIt.value().first){ //The edge is used
            continue;
        }

        auto eid = txIt.key();
        std::vector<FeaturePoint> fc_0, fc_1;
        auto te = topo.m_tope.At(eid);

        (*txIt).first = true;   //Set used
        fc_0.emplace_back(std::move(txIt.value().second));
        propagateChain(eid, te->m_topfi[1], fc_0);
        propagateChain(eid, te->m_topfi[0], fc_1);
        qDebug() << fc_0.size() << " fc2: " << fc_1.size();
        fc_0.insert(fc_0.begin(), fc_1.rbegin(), fc_1.rend());


        //Check starting point
        auto frontPt = fc_0.front(),
             clickPt = projPoints.front();
        const int fid = std::stoi(clickPt.mName);
        eid = std::stoi(frontPt.mName);
        te = topo.m_tope.At(eid);
        if ( fid == te->m_topfi[0] || fid == te->m_topfi[1]){   //Assume 2-manifold
            fc_0.emplace_back(projPoints.back());
            fc_0.insert(fc_0.begin(), projPoints.front());
        } else {
            fc_0.emplace_back(projPoints.front());
            fc_0.insert(fc_0.begin(), projPoints.back());
        }


        FeatureCurve fc;
        fc.mName = name.toStdString();
        fc.mCurve = std::move(fc_0);
        featureCurves.emplace_back(std::move(fc));
    }

    return true;
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
    auto _p = u * mesh.m_V[j] + v * mesh.m_V[k] + w * mesh.m_V[i];
    return QVector3D(_p.x, _p.y, _p.z);
}

std::vector<QVector3D> LP_HumanFeature::member::evaluationFeatureGirth(const ON_Mesh &m, const FeatureGirth &fg)
{
    const auto &nVs = m.VertexCount();
//    if ( 0 >= nVs ){
//        qDebug() << "Invalid Reference Mesh!";
//        return std::vector<QVector3D>();
//    }
    auto &params = fg.mParams;
    std::vector<QVector3D> pts_vec;
    for (unsigned long n = 0; n < fg.mParams.size(); n++){
        const int &i = std::get<0>(params[n]),
                  &j = std::get<1>(params[n]);
        const double &t = std::get<2>(params[n]);
        if ( i >= nVs || j >= nVs  ){
            qDebug() << "Invalid Feature Point!";
            return std::vector<QVector3D>();
        }
        auto _p = mesh.m_V[i] + t * (mesh.m_V[j] - mesh.m_V[i]);
        pts_vec.emplace_back(QVector3D(_p.x, _p.y, _p.z));
    }
    return pts_vec;
}

void LP_HumanFeature::member::initializeEmbree3()
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    rtDevice = rtcNewDevice("verbose=0");
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
    auto jFCs = jDoc["FeatureCurves"].toArray();
    for ( auto &&jFc : jFCs ) {
        FeatureCurve fc;
        auto  &&jObj = jFc.toObject();
        Q_ASSERT( 1 == jObj.size());
        auto jObjIt = jObj.begin();
        fc.mName =  jObjIt.key().toStdString();
        auto &&jFPs = jObjIt.value().toArray();
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
            fc.mCurve.emplace_back(std::move(fp));
        }
        featureCurves.emplace_back(std::move(fc));
    }
    // for feature girths
    auto jFGs = jDoc["FeatureGirths"].toArray();
    for ( auto &&jFg : jFGs ) {
        FeatureGirth fg;
        auto  &&jObj = jFg.toObject();
        Q_ASSERT( 1 == jObj.size());
        auto jObjIt = jObj.begin();
        fg.mName =  jObjIt.key().toStdString();
        auto &&jFPs = jObjIt.value().toArray();
        for ( auto &&jFp : jFPs){
            auto info = jFp.toString().split(",");
            if ( 4 != info.size()){
                qWarning() << "Corrupt Feature Girth : " <<info;
                continue;
            }
            fg.mName = info.at(0).toStdString();
            fg.mParams.emplace_back(std::make_tuple<int,int, float>(info.at(1).toInt(),
                         info.at(2).toInt(),
                         info.at(3).toFloat()
                         ));
        }
        featureGirths.emplace_back(std::move(fg));
    }

}

void LP_HumanFeature::member::exportFeatures(const QString &filename)
{
    if ( filename.isEmpty()){
        return;
    }
    //Prepare the data
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
    //Prepare the data
    QJsonObject jObj_cs;
    QJsonArray jCurvs;
    for ( auto &c : featureCurves ){
        QJsonObject jObj_c;
        QJsonArray jFpts;
        for ( auto &p : c.mCurve ){
            jFpts.append(QString("%6,%1,%2,%3,%4,%5")
                         .arg(std::get<0>(p.mParams))
                         .arg(std::get<1>(p.mParams))
                         .arg(std::get<2>(p.mParams))
                         .arg(std::get<3>(p.mParams))
                         .arg(std::get<4>(p.mParams))
                         .arg(p.mName.data()));
        }
        jObj_c[c.mName.c_str()] = jFpts;
        jCurvs.append(jObj_c);
    }

    jObj["FeatureCurves"] = jCurvs;

    //Prepare the data
    QJsonObject jObj_gs;
    QJsonArray jGirths;
    for ( auto &g : featureGirths ){
        QJsonObject jObj_g;
        QJsonArray jFpts;
        for (unsigned long n = 0; n < g.mParams.size(); n++){
            jFpts.append(QString("%4,%1,%2,%3")
                         .arg(std::get<0>(g.mParams[n]))
                         .arg(std::get<1>(g.mParams[n]))
                         .arg(std::get<2>(g.mParams[n]))
                         .arg(g.mName.data()));
        }

        jObj_g[g.mName.c_str()] = jFpts;
        jGirths.append(jObj_g);
    }

    jObj["FeatureGirths"] = jGirths;

    QJsonDocument jDoc(jObj);

    QFile file(filename);
    if ( !file.open(QIODevice::WriteOnly)){
        qDebug()  << file.errorString();
        return;
    }
    file.write(jDoc.toJson());
    file.close();

}

double LP_HumanFeature::member::getShimaMeasurement(const QString &shimaFt)
{
    auto it = shimaFeatures.find(shimaFt);
    if ( shimaFeatures.end() == it ){
        qWarning() << "Unknown shima feature : " << shimaFt;
        return 0.0;
    }
    return (*it)();
}

void LP_HumanFeature::member::exportSizeChart(const QString &filename)
{
    QFile defaultCSV("SizeChart.csv");
    if ( !defaultCSV.open(QIODevice::ReadOnly)){
        return;
    }
    QString data = defaultCSV.readAll();
    defaultCSV.close();


    QFile file(filename);
    if ( !file.open(QIODevice::WriteOnly)){
        return;
    }
    QRegularExpression tag("(T\\d\\d\\d,)");
    QStringList list;
    QTextStream in(&data);
    QTextStream out(&file);
    while (!in.atEnd()){
        auto &&inLine = in.readLine();
        auto m = tag.match(inLine);
        QString outLine;
        if ( m.hasMatch()){
            list << m.captured();
            auto data_ = inLine.split(",");
            if ( data_[0] == "T001"){
               data_[3] = QString("%1").arg(measurement_T001());
            }
            else if ( data_[0] == "T002"){
               data_[3] = QString("%1").arg(measurement_T002());
            }
            else if ( data_[0] == "T003"){
               data_[3] = QString("%1").arg(measurement_T003());
            }
            else if ( data_[0] == "T026"){
               data_[3] = QString("%1").arg(measurement_T026());
            }
            else if ( data_[0] == "T022"){
               data_[3] = QString("%1").arg(measurement_T022());
            }
            else if ( data_[0] == "T063"){
               data_[3] = QString("%1").arg(measurement_T063());
            }
            else if ( data_[0] == "T011"){
               data_[3] = QString("%1").arg(measurement_T011());
            }
            else if ( data_[0] == "T028"){
               data_[3] = QString("%1").arg(measurement_T028());
            }
            else if ( data_[0] == "T016"){
               data_[3] = QString("%1").arg(measurement_T016());
            }
            else if ( data_[0] == "T006"){
               data_[3] = QString("%1").arg(measurement_T006());
            }
            else if ( data_[0] == "T023"){
               data_[3] = QString("%1").arg(measurement_T023());
            }
            else if ( data_[0] == "T014"){
               data_[3] = QString("%1").arg(measurement_T014());
            }
            //USER
            for (int i = 0; i<data_.size(); i++){
                outLine += data_[i]+",";
            }
            outLine += "\n";
            out << outLine;
        }
        else {out << inLine + "\n";}
    }
//    auto i = tag.globalMatch(data);

//    while (i.hasNext()){
//        auto match = i.next();
//        list << data.mid(match.capturedStart(), 20);
//        qDebug() << data.mid(match.capturedStart(), 20);
//    }

    file.close();
}

double LP_HumanFeature::member::pointName2Measurements(QStringList composite, std::string type)
{
    std::vector<QVector3D> measure_points;
    int next = 0;

    for ( auto &st : composite ) {
        if (type == "points") {
        for ( auto &p : featurePoints ){
            if ( 0 == p.mName.compare(st.toStdString())){
                ++next;
                measure_points.emplace_back(evaluationFeaturePoint(mesh, p));
            }
        }
        }
        else if (type == "curves"){
            for ( auto &c : featureCurves ){
                if ( 0 == c.mName.compare(st.toStdString())){
                    ++next;
                    for (unsigned long i = 0; i< c.mCurve.size(); i++)
                        measure_points.emplace_back(evaluationFeaturePoint(mesh, c.mCurve[i]));
                    break;
                }
            }
        }
        else if (type == "girths"){
            for ( auto &g : featureGirths ){
                if ( 0 == g.mName.compare(st.toStdString())){
                    ++next;
                    measure_points = evaluationFeatureGirth(mesh, g);
                }
            }
         }
                break;
    }

//    if ( next != composite.size()){
//        return 0.0;
//    }
    if ( measure_points.empty()) {
        return 0.0;
    }
    double measurement = 0.0;
    for ( int i = 0; i < measure_points.size()-1; ++i ){
//    for ( int i = 1; i < measure_points.size(); ++i ){
//        measurement += (measure_points[i] - measure_points[i-1]).length();
        measurement += measure_points[i].distanceToPoint(measure_points[i+1]);
    }
    std::cout<<"measurements:"<<measurement<<std::endl;
    return measurement;
}


double LP_HumanFeature::member::measurement_T063()
{
    QStringList composite = {"Shoulder_R","Elbow_R_U","Wist_R_U"};
    return pointName2Measurements(composite, "points");
}
double LP_HumanFeature::member::measurement_T001()
{
    QStringList composite = {"T028"};
    return (measurement_T028()+20);
}
double LP_HumanFeature::member::measurement_T002()
{
    QStringList composite = {"Bust_Girth"};
    return pointName2Measurements(composite, "girths")/2;
}
double LP_HumanFeature::member::measurement_T003()
{
    QStringList composite = {"Shoulder_L","Neck_B","Shoulder_R"};
    return pointName2Measurements(composite, "points");
}
double LP_HumanFeature::member::measurement_T026()
{
    QStringList composite = {"Chest_U"};
    return pointName2Measurements(composite, "curves");
}
double LP_HumanFeature::member::measurement_T011()
{
    QStringList composite = {"Armhole"};
    return pointName2Measurements(composite, "curves");
}
double LP_HumanFeature::member::measurement_T028()
{
    QStringList composite = {"Shoulder_R", "Bust_R", "Helper_01"};
    return pointName2Measurements(composite, "points");
}
double LP_HumanFeature::member::measurement_T022()
{
    QStringList composite = {"Waist_Girth"};
    return pointName2Measurements(composite, "girths")/2;
}
double LP_HumanFeature::member::measurement_T016()
{
    QStringList composite = {"Arm_Width"};
    return pointName2Measurements(composite, "curves");
}
double LP_HumanFeature::member::measurement_T006()
{
    QStringList composite = {"Chest_U_B"};
    return pointName2Measurements(composite, "curves");
}
double LP_HumanFeature::member::measurement_T023()
{
    QStringList composite = {"T023"};
    return pointName2Measurements(composite, "curves");
}
double LP_HumanFeature::member::measurement_T014()
{
    QStringList composite = {"Neck_B", "Neck_R", "Wist_R_U"};
    return pointName2Measurements(composite, "points");
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

std::vector<std::vector<QVector3D> > LP_HumanFeature::member::get3DFeatureCurves()
{
    const auto &nVs = mesh.VertexCount();
    if ( 0 >= nVs ){
        return std::vector<std::vector<QVector3D>>();
    }
    std::vector<std::vector<QVector3D>> curves;
    for ( auto &fc : featureCurves ){

        std::vector<QVector3D> pts;
        for ( auto &fp : fc.mCurve ){
            pts.emplace_back(evaluationFeaturePoint(mesh, fp));
        }
        curves.emplace_back( pts );
    }
    return curves;
}

<<<<<<< HEAD

=======
>>>>>>> 53eb3fd0cd860adc6400ce1964391478baf64c59
std::vector<std::vector<QVector3D>> LP_HumanFeature::member::get3DFeatureGirths()
{
    const auto &nVs = mesh.VertexCount();
    if ( 0 >= nVs ){
        return std::vector<std::vector<QVector3D>>();
    }
    std::vector<std::vector<QVector3D>> girths;
    for ( auto &fg : featureGirths ){
        girths.emplace_back( evaluationFeatureGirth(mesh, fg) );
    }
    return girths;
}

<<<<<<< HEAD

=======
>>>>>>> 53eb3fd0cd860adc6400ce1964391478baf64c59
double LP_HumanFeature::member::getCurveLength(const FeatureCurve &curve)
{
    double distance = 0;
    if(curve.mCurve.size()<=0) return 0;
    std::vector<QVector3D> pts;
    for ( int i =0;i<int(curve.mCurve.size())-1;i++)
    {
        distance+=getP2PLength(curve.mCurve[i],curve.mCurve[i+1]);
    }
    qDebug()<<QString::fromStdString(curve.mName)<<"Distance ="<<distance;
    return distance;
}

double LP_HumanFeature::member::getP2CLength(const FeaturePoint &point, const FeatureCurve &curve)
{
    double distance=9999;
    std::string idx;
    for(auto &pt : curve.mCurve)
    {
        if(getP2PLength(pt,point)<distance)
        {
            distance = getP2PLength(pt,point);
            idx = pt.mName;
        }
    }
    return distance;
}

double LP_HumanFeature::member::getP2PLength(const FeaturePoint &pointA, const FeaturePoint &pointB)
{
    double distance=0;
    QVector3D ptA,ptB;
    ptA = evaluationFeaturePoint(mesh, pointA);
    ptB = evaluationFeaturePoint(mesh, pointB);
    distance = ptA.distanceToPoint(ptB);
    return distance;
}
<<<<<<< HEAD




=======
>>>>>>> 53eb3fd0cd860adc6400ce1964391478baf64c59
