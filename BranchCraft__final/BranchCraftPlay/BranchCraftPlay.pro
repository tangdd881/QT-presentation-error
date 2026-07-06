QT += core gui widgets multimedia

CONFIG += c++17

SOURCES += \
    main.cpp \
    PlayerMainWindow.cpp \
    PlayCanvas.cpp \
    PlayerStoryGraph.cpp

HEADERS += \
    PlayCanvas.h \
    PlayerMainWindow.h \
    PlayerStoryGraph.h
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target