#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qslider.h"
#include <QMainWindow>

class MaterialWidget;
class PropertyPanel;
class SceneCanvas;
class WelcomeWidget;
class QAction;
class QMenuBar;
class QToolBar;
class QSplitter;
class QStackedWidget;
class StoryGraph;
class StoryGraphScene;
class StoryGraphView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 工程管理
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveAsProject();
    void onClearProject();
    void onExportProject();
    void onProjectOpened(const QString &storyFilePath);
    void onStoryDataChanged();
    void onProjectChanged();

    // 素材管理
    void onImportMedia();
    void onRefreshMaterialList();
    void onMaterialRenameRequested(const QString &oldname, const QString &newname);
    void onMaterialDeleteRequested(const QString &relPath);
    void onMaterialReplaceRequested(const QString &oldRelPath, const QString &newFilePath);
    void onPreviewMaterial(const QString &fileName);

    // 剧情节点
    void onAddChildNode();
    void onDeleteSelectedNode();
    void onRenameSelectedNode();

    // 视图控制（预览）
    void onPreview();

    //剧情树视图
    void onZoomSliderChanged(int value);
    void onFitView();
    void onGraphViewZoomChanged(qreal zoom);
    void centerViewOnRootNode();

    //历史工程路径记录
    void onDemoProject();                 // 演示工程
    void onOpenRecentProject(const QString &projectPath);   // 打开近期工程
private:
    void setupMenuBar();
    void setupToolBar();
    void setupCentralArea();
    void setEditorChromeVisible(bool visible);
    void saveStoryToJson();
    void loadStoryFromJson(const QString &filePath);
    void openProjectFile(const QString &istFilePath);   // 公共打开逻辑

    StoryGraph *m_storyGraph;
    StoryGraphScene *m_graphScene;
    StoryGraphView *m_graphView;
    WelcomeWidget *m_welcomeWidget;
    MaterialWidget *m_materialWidget;
    PropertyPanel *m_propertyPanel;
    SceneCanvas *m_sceneCanvas;
    QStackedWidget *m_centralStack;
    QSplitter *m_mainSplitter;
    QSlider *m_zoomSlider;
    QAction *m_fitViewAct;

    // 菜单动作
    QAction *m_newAct;
    QAction *m_openAct;
    QAction *m_saveAct;
    QAction *m_saveAsAct;
    QAction *m_clearAct;
    QAction *m_exportAct;
    QAction *m_exitAct;

    QAction *m_importMediaAct;
    QAction *m_refreshMaterialAct;

    QAction *m_addChildNodeAct;
    QAction *m_deleteNodeAct;
    QAction *m_renameNodeAct;

    QAction *m_previewAct;
};

#endif // MAINWINDOW_H
