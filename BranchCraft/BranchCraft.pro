QT       += core gui multimedia multimediawidgets widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# 定义应用程序名称
TARGET = InteractiveStoryEditor
TEMPLATE = app

SOURCES += \
    NodeGraphicsItem.cpp \
    PreviewDialog.cpp \
    SceneCanvas.cpp \
    StoryGraph.cpp \
    StoryGraphScene.cpp \
    StoryGraphView.cpp \
    WelcomeWidget.cpp \
    main.cpp \
    MainWindow.cpp \
    NewProjectDialog.cpp \
    MaterialWidget.cpp \
    PropertyPanel.cpp \
    ProjectManager.cpp

HEADERS += \
    MainWindow.h \
    MaterialWidget.h \
    NewProjectDialog.h \
    MaterialWidget.h \
    NodeGraphicsItem.h \
    PreviewDialog.h \
    ProjectManager.h \
    PropertyPanel.h \
    SceneCanvas.h \
    StoryGraph.h \
    StoryGraphScene.h \
    StoryGraphView.h \
    StyleSheets.h \
    WelcomeWidget.h

RESOURCES += \
    resources.qrc   # 可选，用于存放图标等

# 默认规则
win64:RC_ICONS += app.ico
