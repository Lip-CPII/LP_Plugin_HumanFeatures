#ifndef LP_PCA_HUMAN_H
#define LP_PCA_HUMAN_H

#include "lp_humanfeature.h"

class LP_PCA_Human : public LP_HumanFeature
{
    Q_OBJECT
public:
    virtual ~LP_PCA_Human();

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

#endif // LP_PCA_HUMAN_H
