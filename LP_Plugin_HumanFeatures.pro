QT += gui widgets

TEMPLATE = lib
DEFINES += LP_PLUGIN_HUMANFEATURE_LIBRARY _USE_MATH_DEFINES
DEFINES += EIGEN_HAS_CONSTEXPR EIGEN_MAX_CPP_VER=17

CONFIG += c++17

QMAKE_POST_LINK=$(MAKE) install

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    extern/eiquadprog-1.2.3/src/eiquadprog-fast.cpp \
    extern/eiquadprog-1.2.3/src/eiquadprog.cpp \
    lp_humanfeature.cpp \
    lp_pca_human.cpp

HEADERS += \
    LP_Plugin_HumanFeature_global.h \
    MeshPlaneIntersect.hpp \
    extern/eiquadprog.hpp \
    lp_humanfeature.h \
    lp_pca_human.h

# Default rules for deployment.
target.path = $$OUT_PWD/../App/plugins/$$TARGET

!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Model/release/ -lModel
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Model/debug/ -lModel
else:unix:!macx: LIBS += -L$$OUT_PWD/../Model/ -lModel

INCLUDEPATH += $$PWD/../Model
DEPENDPATH += $$PWD/../Model

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Functional/release/ -lFunctional
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Functional/debug/ -lFunctional
else:unix:!macx: LIBS += -L$$OUT_PWD/../Functional/ -lFunctional

INCLUDEPATH += $$PWD/../Functional
DEPENDPATH += $$PWD/../Functional

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCored
else:unix:!macx: LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCore

INCLUDEPATH += $$PWD/../../OpenMesh/include
DEPENDPATH += $$PWD/../../OpenMesh/include

DISTFILES +=

INCLUDEPATH += $$PWD/extern/eigen-3.4-rc1/install/include/eigen3
DEPENDPATH += $$PWD/extern/eigen-3.4-rc1/install/include/eigen3

INCLUDEPATH += $$PWD/extern/eiquadprog-1.2.3/include
DEPENDPATH += $$PWD/extern/eiquadprog-1.2.3/include

INCLUDEPATH += $$PWD/extern/embree3/include
DEPENDPATH += $$PWD/extern/embree3/include

win32: LIBS += -L$$PWD/extern/embree3/lib/ -lembree3

INCLUDEPATH += $$PWD/../../opennurbs/include
DEPENDPATH += $$PWD/../../opennurbs/include

win32: LIBS += -L$$PWD/../../opennurbs/lib/ -lopennurbs_public
else:unix:!macx: LIBS += -L$$PWD/../../opennurbs/lib/ -lopennurbs_public

win32: {
    DEFINES += OPENNURBS_IMPORTS NOMINMAX
}
else:unix:!macx: {
    DEFINES += OPENNURBS_IMPORTS
}
