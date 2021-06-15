#ifndef LP_HUMANFEATURE_H
#define LP_HUMANFEATURE_H

#include "plugin/lp_actionplugin.h"
#include <QFutureWatcher>
#include <QReadWriteLock>
#include <QThreadPool>
#include <QVector3D>

/*Define the name, version of the plugin
*/
#define LP_HumanFeature_Plugin_iid "cpii.rp5.SmartFashion.LP_HumanFeature/0.1"

class LP_ObjectImpl;
class QLabel;
class QOpenGLShaderProgram;

#include "LP_Plugin_HumanFeature_global.h"

class LP_PLUGIN_HUMANFEATURE_EXPORT LP_HumanFeature : public LP_ActionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LP_HumanFeature_Plugin_iid)
    Q_INTERFACES(LP_ActionPlugin)
public:
    virtual ~LP_HumanFeature();

    // QObject interface
public:
    bool eventFilter(QObject *watched, QEvent *event) override;

    // LP_Functional interface
public:
    QWidget *DockUi() override;
    bool Run() override;

public slots:
    void FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;
    void PainterDraw(QWidget *glW) override;

    // LP_ActionPlugin interface
public:
    QString MenuName() override;
    QAction *Trigger() override;

protected:
    void initializeGL();

private:
    QOpenGLShaderProgram *mProgram = nullptr, *mProgramFeatures = nullptr;
    std::shared_ptr<QWidget> mWidget;
    std::weak_ptr<LP_ObjectImpl> mObject;
    QLabel *mLabel = nullptr;
    bool mInitialized = false;

    struct member;
    std::shared_ptr<member> mMember;

signals:
    void updateSliders();
};

#endif // LP_HUMANFEATURE_H
