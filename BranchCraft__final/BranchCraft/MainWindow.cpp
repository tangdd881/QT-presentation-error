#include "MainWindow.h"
#include "MaterialWidget.h"
#include "PropertyPanel.h"
#include "SceneCanvas.h"
#include "NewProjectDialog.h"
#include "ProjectManager.h"
#include "PreviewDialog.h"
#include "StoryGraph.h"
#include "StoryGraphScene.h"
#include "StoryGraphView.h"
#include "WelcomeWidget.h"
#include "NodeGraphicsItem.h"
#include "qlabel.h"
#include "qlistwidget.h"
#include "qspinbox.h"
#include "qtimer.h"
#include <QStackedWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QSplitter>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QSaveFile>
#include <QTreeView>
#include <QUuid>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QOverload>
#include "StyleSheets.h"

// 递归复制目录
static bool copyDirectory(const QString &srcPath, const QString &dstPath)
{
    QDir srcDir(srcPath);
    if (!srcDir.exists())
        return false;

    QDir dstDir;
    if (!dstDir.mkpath(dstPath))
        return false;

    // 复制文件
    for (const QString &fileName : srcDir.entryList(QDir::Files)) {
        QString srcFile = srcPath + "/" + fileName;
        QString dstFile = dstPath + "/" + fileName;
        if (!QFile::copy(srcFile, dstFile))
            return false;
    }

    // 递归复制子目录
    for (const QString &subDirName : srcDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!copyDirectory(srcPath + "/" + subDirName, dstPath + "/" + subDirName))
            return false;
    }
    return true;
}

//历史工程声明槽函数

void MainWindow::onDemoProject()
{
    QString demoPath = QCoreApplication::applicationDirPath() + "/demo/demo.ist";
    if (!QFile::exists(demoPath)) {
        QMessageBox::warning(this, tr("错误"), tr("演示工程不存在: %1").arg(demoPath));
        return;
    }
    openProjectFile(demoPath);
}

void MainWindow::onOpenRecentProject(const QString &projectPath)
{
    openProjectFile(projectPath);
}

// 抽取公共打开逻辑
void MainWindow::openProjectFile(const QString &istFilePath)
{
    if (ProjectManager::instance().openProject(istFilePath)) {
        setEditorChromeVisible(true);
        if (m_centralStack && m_mainSplitter) {
            m_centralStack->setCurrentWidget(m_mainSplitter);
        }
        statusBar()->showMessage(tr("已打开工程: ") + ProjectManager::instance().currentProjectName());
        loadStoryFromJson(ProjectManager::instance().storyFilePath());
        if (m_graphScene) {
            m_graphScene->refreshFromGraph();
        }
        ProjectManager::instance().addToRecentProjects(istFilePath);
    } else {
        QMessageBox::critical(this, tr("错误"), tr("打开工程失败"));
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("互动剧情视频编辑器"));
    resize(1280, 800);

    // 先创建核心数据对象
    m_storyGraph = new StoryGraph(this);
    connect(m_storyGraph, &StoryGraph::dataChanged, this, &MainWindow::onStoryDataChanged);
    connect(&ProjectManager::instance(), &ProjectManager::projectChanged, this, &MainWindow::onProjectChanged);

    // 构建界面（这里会创建 m_graphScene, m_welcomeWidget 等）
    setupMenuBar();
    setupToolBar();
    setupCentralArea();   // 所有 UI 控件在此创建
    setEditorChromeVisible(false);

    setStyleSheet(StyleSheets::MAIN_WINDOW);

    // 素材库信号连接（控件已存在）
    connect(m_materialWidget, &MaterialWidget::importRequested, this, &MainWindow::onImportMedia);
    connect(m_materialWidget, &MaterialWidget::refreshRequested, this, &MainWindow::onRefreshMaterialList);
    connect(m_materialWidget, &MaterialWidget::materialRenameRequested, this, &MainWindow::onMaterialRenameRequested);
    connect(m_materialWidget, &MaterialWidget::materialDeleteRequested, this, &MainWindow::onMaterialDeleteRequested);
    connect(m_materialWidget, &MaterialWidget::materialReplaceRequested, this, &MainWindow::onMaterialReplaceRequested);
    connect(m_materialWidget, &MaterialWidget::previewRequested, this, &MainWindow::onPreviewMaterial);

    // 属性面板相关信号
    connect(m_propertyPanel, &PropertyPanel::graphDataChanged, m_graphScene, QOverload<>::of(&StoryGraphScene::refreshFromGraph));
    connect(m_propertyPanel, &PropertyPanel::graphDataChanged, this, [this](){
        if (m_sceneCanvas) {
            m_sceneCanvas->updateFromNode();
            m_sceneCanvas->update();
        }
    });
    connect(m_propertyPanel, &PropertyPanel::dataChanged, this, [this](const QString &){
        if (m_sceneCanvas) {
            m_sceneCanvas->refreshDisplay();
        }
    });

    // 数据变化刷新场景（m_graphScene 已存在）
    connect(m_storyGraph, &StoryGraph::dataChanged, m_graphScene, QOverload<>::of(&StoryGraphScene::refreshFromGraph));

    // 欢迎页近期工程初始化（m_welcomeWidget 已存在）
    m_welcomeWidget->updateRecentProjects(ProjectManager::instance().getRecentProjects());
    connect(&ProjectManager::instance(), &ProjectManager::recentProjectsChanged, this, [this](){
        if (m_welcomeWidget) {
            m_welcomeWidget->updateRecentProjects(ProjectManager::instance().getRecentProjects());
        }
    });

    statusBar()->showMessage(tr("就绪"));
}

MainWindow::~MainWindow() {}

//设置欢应界面，将控件隐藏

void MainWindow::setEditorChromeVisible(bool visible)
{
    if (menuBar()) {
        menuBar()->setVisible(visible);
    }
    if (statusBar()) {
        statusBar()->setVisible(visible);
    }
    const auto toolBars = findChildren<QToolBar*>();
    for (QToolBar *toolBar : toolBars) {
        toolBar->setVisible(visible);
    }
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    m_newAct = fileMenu->addAction(tr("新建工程(&N)"), this, &MainWindow::onNewProject);
    m_newAct->setShortcut(QKeySequence::New);
    m_openAct = fileMenu->addAction(tr("打开工程(&O)"), this, &MainWindow::onOpenProject);
    m_openAct->setShortcut(QKeySequence::Open);
    m_saveAct = fileMenu->addAction(tr("保存工程(&S)"), this, &MainWindow::onSaveProject);
    m_saveAct->setShortcut(QKeySequence::Save);
    m_saveAsAct = fileMenu->addAction(tr("另存为工程(&A)"), this, &MainWindow::onSaveAsProject);
    fileMenu->addSeparator();
    m_clearAct = fileMenu->addAction(tr("清空工程内容(&C)"), this, &MainWindow::onClearProject);
    fileMenu->addSeparator();
    m_exportAct = fileMenu->addAction(tr("一键导出(&E)"), this, &MainWindow::onExportProject);
    fileMenu->addSeparator();
    m_exitAct = fileMenu->addAction(tr("退出(&X)"), this, &QWidget::close);
    m_exitAct->setShortcut(QKeySequence::Quit);

    QMenu *editMenu = menuBar()->addMenu(tr("编辑(&E)"));
    m_importMediaAct = editMenu->addAction(tr("导入素材(&I)"), this, &MainWindow::onImportMedia);
    m_refreshMaterialAct = editMenu->addAction(tr("刷新素材列表(&R)"), this, &MainWindow::onRefreshMaterialList);

    QMenu *nodeMenu = menuBar()->addMenu(tr("节点(&N)"));
    m_addChildNodeAct = nodeMenu->addAction(tr("添加子节点"), this, &MainWindow::onAddChildNode);
    m_deleteNodeAct = nodeMenu->addAction(tr("删除选中节点(&Delete)"), this, &MainWindow::onDeleteSelectedNode);
    m_deleteNodeAct->setShortcut(QKeySequence::Delete);
    m_renameNodeAct = nodeMenu->addAction(tr("重命名节点(&F2)"), this, &MainWindow::onRenameSelectedNode);
    m_renameNodeAct->setShortcut(Qt::Key_F2);

    QMenu *viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    m_previewAct = viewMenu->addAction(tr("预览放映(&F5)"), this, &MainWindow::onPreview);
    m_previewAct->setShortcut(Qt::Key_F5);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar(tr("主要工具"));
    toolBar->addAction(m_newAct);
    toolBar->addAction(m_openAct);
    toolBar->addAction(m_saveAct);
    toolBar->addSeparator();
    toolBar->addAction(m_importMediaAct);
    toolBar->addSeparator();
    toolBar->addAction(m_addChildNodeAct);
    toolBar->addAction(m_deleteNodeAct);
    toolBar->addSeparator();
    toolBar->addAction(m_previewAct);
    toolBar->addAction(m_exportAct);
    QToolBar *viewToolBar = addToolBar(tr("视图"));

    // 适应窗口按钮
    m_fitViewAct = new QAction(tr("适应窗口"), this);
    m_fitViewAct->setShortcut(Qt::Key_Space);
    connect(m_fitViewAct, &QAction::triggered, this, &MainWindow::onFitView);
    viewToolBar->addAction(m_fitViewAct);

    viewToolBar->addSeparator();

    QLabel *zoomLabel = new QLabel(tr("缩放:"));
    viewToolBar->addWidget(zoomLabel);

    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(10, 500);   // 10% ~ 500%
    m_zoomSlider->setValue(100);
    m_zoomSlider->setFixedWidth(120);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &MainWindow::onZoomSliderChanged);
    viewToolBar->addWidget(m_zoomSlider);
}

void MainWindow::setupCentralArea()
{
    m_centralStack = new QStackedWidget(this);
    m_welcomeWidget = new WelcomeWidget(this);
    m_centralStack->addWidget(m_welcomeWidget);

    connect(m_welcomeWidget, &WelcomeWidget::newProjectRequested, this, &MainWindow::onNewProject);
    connect(m_welcomeWidget, &WelcomeWidget::openProjectRequested, this, &MainWindow::onOpenProject);

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);

    // 素材库 Widget
    m_materialWidget = new MaterialWidget(this);
    m_materialWidget->setMinimumWidth(150);
    m_materialWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_mainSplitter->addWidget(m_materialWidget);

    // 垂直分割器：剧情树视图 + 场景画布
    QSplitter *verticalSplitter = new QSplitter(Qt::Vertical, this);

    // 创建场景和视图
    m_graphScene = new StoryGraphScene(m_storyGraph, this);
    m_graphView = new StoryGraphView(m_graphScene, this);
    m_graphView->setMinimumHeight(100);
    verticalSplitter->addWidget(m_graphView);

    m_sceneCanvas = new SceneCanvas(m_storyGraph, this);
    m_sceneCanvas->setMinimumHeight(100);
    verticalSplitter->addWidget(m_sceneCanvas);

    verticalSplitter->setStretchFactor(0, 1);   // 剧情树视图占1份
    verticalSplitter->setStretchFactor(1, 2);   // 画布占2份
    verticalSplitter->setSizes({300, 400});     // 初始大小

    m_mainSplitter->addWidget(verticalSplitter);

    // 属性面板
    m_propertyPanel = new PropertyPanel(m_storyGraph, m_materialWidget, this);
    m_propertyPanel->setMinimumWidth(200);
    m_propertyPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_mainSplitter->addWidget(m_propertyPanel);

    // 拉伸因子
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setStretchFactor(2, 0);
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setSizes({200, 600, 300});

    m_centralStack->addWidget(m_mainSplitter);
    setCentralWidget(m_centralStack);
    m_centralStack->setCurrentWidget(m_welcomeWidget);

    // 属性面板的幻灯片切换信号连接画布
    connect(m_propertyPanel, &PropertyPanel::slideChanged, m_sceneCanvas, &SceneCanvas::setCurrentSlideIndex);

    // 当剧情图数据改变时，刷新属性面板中当前节点的显示（仅刷新矩形等轻量级数据）
    connect(m_storyGraph, &StoryGraph::dataChanged, this, [this](){
        if (m_propertyPanel) {
            QString curId = m_graphScene->selectedNodeId();
            if (!curId.isEmpty()) {
                m_propertyPanel->refreshCurrentNode();  // 轻量刷新
            }
        }
    });

    // 画布拖拽文本框后，更新属性面板的矩形数值（专用信号）
    connect(m_sceneCanvas, &SceneCanvas::textRectChanged, m_propertyPanel, &PropertyPanel::updateTextRect);
    // 画布拖拽角色名字矩形后，更新属性面板
    connect(m_sceneCanvas, &SceneCanvas::characterNameRectChanged, m_propertyPanel, &PropertyPanel::updateCharacterNameRect);
    // 画布拖拽角色图片或选项后，可通过 nodeDataChanged 触发整体刷新（但我们已经用 dataChanged 处理，可省略，或者保留但调用 refreshCurrentNode）
    connect(m_sceneCanvas, &SceneCanvas::nodeDataChanged, m_propertyPanel, &PropertyPanel::onNodeDataChanged);
    // 当剧情图节点被选中时，更新属性面板和场景画布
    connect(m_graphScene, &StoryGraphScene::nodeSelected,
            m_propertyPanel, &PropertyPanel::setCurrentNode,
            Qt::QueuedConnection);

    connect(m_graphScene, &StoryGraphScene::nodeSelected,
            m_sceneCanvas, &SceneCanvas::setCurrentNode,
            Qt::QueuedConnection);
    // 连接剧情树场景的“链接到已有节点”请求
    connect(m_graphScene, &StoryGraphScene::linkToExistingNodeRequested, this, [this](const QString &branchNodeId){
        m_propertyPanel->setCurrentNode(branchNodeId);
        // 延迟调用，确保属性面板的 loading 状态已清除
        QTimer::singleShot(1, m_propertyPanel, &PropertyPanel::onLinkToExistingNode);
    });
    connect(m_graphView, &StoryGraphView::zoomChanged, this, &MainWindow::onGraphViewZoomChanged);

    connect(m_welcomeWidget, &WelcomeWidget::demoProjectRequested, this, &MainWindow::onDemoProject);
    connect(m_welcomeWidget, &WelcomeWidget::recentProjectRequested, this, &MainWindow::onOpenRecentProject);

}
//-----------------------剧情树视图----------------------------
void MainWindow::onZoomSliderChanged(int value)
{
    if (!m_graphView) return;
    qreal zoom = value / 100.0;
    m_graphView->setZoomLevel(zoom);
}

void MainWindow::onFitView()
{
    if (!m_graphView) return;
    m_graphView->fitToView();
    // 同步滑块值
    qreal zoom = m_graphView->zoomLevel();
    int value = qRound(zoom * 100);
    if (value != m_zoomSlider->value()) {
        m_zoomSlider->blockSignals(true);
        m_zoomSlider->setValue(value);
        m_zoomSlider->blockSignals(false);
    }
}

void MainWindow::onGraphViewZoomChanged(qreal zoom)
{
    int value = qRound(zoom * 100);
    if (value != m_zoomSlider->value()) {
        m_zoomSlider->blockSignals(true);
        m_zoomSlider->setValue(value);
        m_zoomSlider->blockSignals(false);
    }
}

// ---------------------- 工程管理（原有实现，保持不变）----------------
void MainWindow::onNewProject()
{
    //添加操作其他项目前保存询问机制，避免未保存旧工程
    if (ProjectManager::instance().isProjectOpen()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("保存当前工程"),
            tr("当前有打开的工程，是否保存后再打开新工程？"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );

        if (reply == QMessageBox::Yes) {
            onSaveProject();
        } else if (reply == QMessageBox::Cancel) {
            return;  // 用户取消打开操作
        }
        // No 的情况：继续执行，不保存
    }
    NewProjectDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        if (ProjectManager::instance().isProjectOpen()) {
            statusBar()->showMessage(tr("当前工程: ") + ProjectManager::instance().currentProjectName());
            // 从工程文件夹的 story.json 加载数据（因为 createNewProject 已经写入了默认节点）
            loadStoryFromJson(ProjectManager::instance().storyFilePath());
            if (m_centralStack && m_mainSplitter) {
                m_centralStack->setCurrentWidget(m_mainSplitter);
            }
            setEditorChromeVisible(true);
            QTimer::singleShot(50, this, &MainWindow::centerViewOnRootNode);
            // 强制刷新图形视图
            if (m_graphScene) {
                m_graphScene->refreshFromGraph();
            }
            // 在 loadStoryFromJson 之后
            ProjectManager::instance().addToRecentProjects(
                ProjectManager::instance().currentProjectPath() + "/project.ist"
                );
        } else {
            QMessageBox::critical(this, tr("错误"), tr("创建工程失败"));
        }
    }
    if (m_sceneCanvas) {
        m_sceneCanvas->setProjectPath(ProjectManager::instance().currentProjectPath());
    }
}

void MainWindow::onOpenProject()
{
    //添加操作其他项目前保存询问机制，避免未保存旧工程
    if (ProjectManager::instance().isProjectOpen()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("保存当前工程"),
            tr("当前有打开的工程，是否保存后再打开新工程？"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );

        if (reply == QMessageBox::Yes) {
            onSaveProject();
        } else if (reply == QMessageBox::Cancel) {
            return;  // 用户取消打开操作
        }
        // No 的情况：继续执行，不保存
    }
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开工程"), QString(), tr("互动剧情工程 (*.ist)"));
    if (!fileName.isEmpty()) {
        if (ProjectManager::instance().openProject(fileName)) {
            setEditorChromeVisible(true);
            if (m_centralStack && m_mainSplitter) {
                m_centralStack->setCurrentWidget(m_mainSplitter);
            }
            statusBar()->showMessage(tr("已打开工程: ") + ProjectManager::instance().currentProjectName());
            loadStoryFromJson(ProjectManager::instance().storyFilePath());
            // 手动刷新图形视图
            if (m_graphScene) {
                m_graphScene->refreshFromGraph();
            }
            ProjectManager::instance().addToRecentProjects(fileName);
        } else {
            QMessageBox::critical(this, tr("错误"), tr("打开工程失败"));
        }
    }
    if (m_sceneCanvas) {
        m_sceneCanvas->setProjectPath(ProjectManager::instance().currentProjectPath());
    }
}
void MainWindow::onSaveProject()
{
    if (!ProjectManager::instance().isProjectOpen()) {
        QMessageBox::warning(this, tr("警告"), tr("没有打开的工程"));
        return;
    }
    saveStoryToJson();
    if (ProjectManager::instance().saveProject()) {
        statusBar()->showMessage(tr("工程已保存"));
    } else {
        QMessageBox::critical(this, tr("错误"), tr("保存失败"));
    }
}

void MainWindow::onSaveAsProject()
{
    QString newPath = QFileDialog::getSaveFileName(this, tr("另存为工程"), QString(), tr("互动剧情工程 (*.ist)"));
    if (!newPath.isEmpty()) {
        if (ProjectManager::instance().saveAsProject(newPath)) {
            statusBar()->showMessage(tr("工程已另存为: ") + newPath);
        } else {
            QMessageBox::critical(this, tr("错误"), tr("另存失败"));
        }
    }
}

void MainWindow::onClearProject()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("清空工程"), tr("确定要清空当前所有工程内容吗？此操作不可撤销。"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_storyGraph->clear();
        statusBar()->showMessage(tr("工程已清空"));
    }
}

void MainWindow::onExportProject()
{
    if (!ProjectManager::instance().isProjectOpen()) {
        QMessageBox::warning(this, tr("警告"), tr("请先新建或打开工程"));
        return;
    }

    // 选择导出根目录
    QString exportRoot = QFileDialog::getExistingDirectory(this, tr("选择导出根目录"));
    if (exportRoot.isEmpty())
        return;

    QString projectName = ProjectManager::instance().currentProjectName();
    QString projectSrcPath = ProjectManager::instance().currentProjectPath();

    // 在导出根目录下创建以工程名命名的文件夹
    QString exportDir = exportRoot + "/" + projectName;
    QDir dir(exportDir);   // 注意：现在 dir 代表 exportDir

    if (dir.exists()) {
        QMessageBox::StandardButton ret = QMessageBox::question(
            this, tr("覆盖确认"),
            tr("导出目录已存在，是否覆盖？"),
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes)
            return;
        // 删除旧目录 - 正确用法：调用无参 removeRecursively()
        if (!dir.removeRecursively()) {
            QMessageBox::critical(this, tr("错误"), tr("无法删除已有目录"));
            return;
        }
    }
    if (!dir.mkpath(exportDir)) {   // 重新创建
        QMessageBox::critical(this, tr("错误"), tr("创建导出目录失败"));
        return;
    }

    // 1. 复制播放器目录（整个 player 文件夹）
    QString playerSrc = QCoreApplication::applicationDirPath() + "/player";
    QString playerDst = exportDir + "/player";
    if (!QFile::exists(playerSrc)) {
        QMessageBox::critical(this, tr("错误"), tr("找不到播放器目录:\n%1").arg(playerSrc));
        return;
    }
    if (!copyDirectory(playerSrc, playerDst)) {
        QMessageBox::critical(this, tr("错误"), tr("复制播放器失败"));
        return;
    }

    // 2. 复制工程数据（story.json 和 resources）
    QString storySrc = projectSrcPath + "/story.json";
    QString storyDst = exportDir + "/story.json";
    if (!QFile::copy(storySrc, storyDst)) {
        QMessageBox::critical(this, tr("错误"), tr("复制 story.json 失败"));
        return;
    }

    QString resourcesSrc = projectSrcPath + "/resources";
    QString resourcesDst = exportDir + "/resources";
    if (QDir(resourcesSrc).exists()) {
        if (!copyDirectory(resourcesSrc, resourcesDst)) {
            QMessageBox::critical(this, tr("错误"), tr("复制 resources 文件夹失败"));
            return;
        }
    } else {
        // 如果没有 resources 目录，创建一个空的
        QDir().mkpath(resourcesDst);
    }

    // 可选：在导出根目录下创建一个启动批处理文件
    QString batPath = exportDir + "/启动播放器.bat";
    QFile bat(batPath);
    if (bat.open(QIODevice::WriteOnly | QIODevice::Text)) {
        bat.write("start player\\BranchCraftPlay.exe .\n");
        bat.close();
    }

    QMessageBox::information(this, tr("导出成功"),
                             tr("工程已导出到:\n%1\n可以直接运行“启动播放器.bat”开始播放。").arg(exportDir));
}
void MainWindow::onProjectOpened(const QString &storyFilePath)
{
    loadStoryFromJson(storyFilePath);
}

void MainWindow::loadStoryFromJson(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot read story.json:" << filePath;
        return;
    }
    QByteArray data = file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "story.json parse error:" << err.errorString();
        return;
    }
    m_storyGraph->fromJson(doc.object());
    statusBar()->showMessage(tr("剧情加载完成，节点数: %1").arg(m_storyGraph->getAllNodes().size()));
    if (m_sceneCanvas) {
        m_sceneCanvas->setProjectPath(ProjectManager::instance().currentProjectPath());
    }
    QTimer::singleShot(50, this, &MainWindow::centerViewOnRootNode);
}

void MainWindow::saveStoryToJson()
{
    if (!ProjectManager::instance().isProjectOpen()) return;
    QString storyPath = ProjectManager::instance().storyFilePath();
    QSaveFile file(storyPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot write story.json";
        return;
    }
    QJsonObject storyObj = m_storyGraph->toJson();
    file.write(QJsonDocument(storyObj).toJson());
    if (!file.commit()) {
        qWarning() << "Failed to commit story.json";
    }
}

void MainWindow::onStoryDataChanged()
{
    setWindowTitle(tr("互动剧情编辑器 - %1*").arg(ProjectManager::instance().currentProjectName()));
}

void MainWindow::onProjectChanged()
{
    if (ProjectManager::instance().isProjectOpen()) {
        setWindowTitle(tr("互动剧情编辑器 - %1").arg(ProjectManager::instance().currentProjectName()));
    } else {
        setWindowTitle(tr("互动剧情编辑器 - 无工程"));
    }
    m_storyGraph->clear();
    onRefreshMaterialList();
    if (ProjectManager::instance().isProjectOpen()) {
        m_sceneCanvas->setProjectPath(ProjectManager::instance().currentProjectPath());
    } else {
        m_sceneCanvas->setProjectPath(QString());
        if (m_centralStack && m_welcomeWidget) {
            m_centralStack->setCurrentWidget(m_welcomeWidget);
        }
        setEditorChromeVisible(false);
    }
}

// ---------------------- 素材管理 ---------------------
void MainWindow::onImportMedia()
{
    if (!ProjectManager::instance().isProjectOpen()) {
        QMessageBox::warning(this, tr("警告"), tr("请先新建或打开工程"));
        return;
    }
    QStringList files = QFileDialog::getOpenFileNames(this, tr("导入媒体素材"), QString(), tr("媒体文件 (*.mp4 *.jpg *.png *.mp3 *.wav)"));
    if (files.isEmpty()) return;
    QString resourcesDir = ProjectManager::instance().currentProjectPath() + "/resources";
    QDir dir(resourcesDir);
    if (!dir.exists()) dir.mkpath(".");
    QStringList addedRelPaths;
    for (const QString &src : files) {
        QFileInfo info(src);
        QString destFileName = info.fileName();
        QString dest = resourcesDir + "/" + destFileName;
        int counter = 1;
        while (QFile::exists(dest)) {
            QFileInfo base(info.baseName());
            destFileName = QString("%1_%2.%3").arg(base.baseName()).arg(counter).arg(info.suffix());
            dest = resourcesDir + "/" + destFileName;
            counter++;
        }
        if (QFile::copy(src, dest)) {
            addedRelPaths.append("resources/" + destFileName);
        } else {
            qWarning() << "Failed to copy" << src << "to" << dest;
        }
    }
    if (!addedRelPaths.isEmpty()) {
        onRefreshMaterialList();
        statusBar()->showMessage(tr("已导入 %1 个素材").arg(addedRelPaths.size()));
    }
}

void MainWindow::onRefreshMaterialList()
{
    if (!ProjectManager::instance().isProjectOpen()) return;
    QString resourcesDir = ProjectManager::instance().currentProjectPath() + "/resources";
    QDir dir(resourcesDir);
    QStringList entries = dir.entryList(QDir::Files);
    QStringList relPaths;
    for (const QString &file : entries) {
        relPaths.append("resources/" + file);
    }
    m_materialWidget->updateMaterialList(relPaths);
}

void MainWindow::onMaterialRenameRequested(const QString &oldName, const QString &newName)
{
    if (!ProjectManager::instance().isProjectOpen()) return;
    QString resourcesDir = ProjectManager::instance().currentProjectPath() + "/resources";
    QString oldPath = resourcesDir + "/" + oldName;
    QString newPath = resourcesDir + "/" + newName;
    if (QFile::rename(oldPath, newPath)) {
        onRefreshMaterialList();
        statusBar()->showMessage(tr("已重命名为: %1").arg(newName));
    } else {
        QMessageBox::warning(this, tr("错误"), tr("重命名失败"));
    }
}

void MainWindow::onMaterialDeleteRequested(const QString &relPath)
{
    if (!ProjectManager::instance().isProjectOpen()) return;
    QString fullPath = ProjectManager::instance().currentProjectPath() + "/" + relPath;
    if (QFile::remove(fullPath)) {
        statusBar()->showMessage(tr("已删除: %1").arg(relPath));
        onRefreshMaterialList();
    } else {
        QMessageBox::critical(this, tr("错误"), tr("删除失败"));
    }
}

void MainWindow::onMaterialReplaceRequested(const QString &oldRelPath, const QString &newFilePath)
{
    if (!ProjectManager::instance().isProjectOpen()) return;
    QString fullOldPath = ProjectManager::instance().currentProjectPath() + "/" + oldRelPath;
    if (!QFile::remove(fullOldPath)) {
        QMessageBox::critical(this, tr("错误"), tr("无法删除原文件"));
        return;
    }
    if (QFile::copy(newFilePath, fullOldPath)) {
        statusBar()->showMessage(tr("已替换: %1").arg(oldRelPath));
        onRefreshMaterialList();
    } else {
        QMessageBox::critical(this, tr("错误"), tr("替换失败"));
    }
}

void MainWindow::onPreviewMaterial(const QString &fileName)
{
    if (!ProjectManager::instance().isProjectOpen()) return;
    QString fullPath = ProjectManager::instance().currentProjectPath() + "/resources/" + fileName;
    if (QFile::exists(fullPath)) {
        PreviewDialog dlg(fullPath, this);
        dlg.exec();
    } else {
        QMessageBox::warning(this, tr("错误"), tr("文件不存在"));
    }
}

void MainWindow::onAddChildNode()
{
    QString curId = m_graphScene->selectedNodeId();

    if (!curId.isEmpty()) {
        // 有选中节点 -> 选择添加的子节点类型
        NodeGraphicsItem *parentItem = m_graphScene->findNodeItem(curId);
        if (!parentItem) return;

        QMenu menu;
        QAction *actNormal = menu.addAction(tr("普通子节点"));
        QAction *actChoice = menu.addAction(tr("分支子节点"));
        QAction *selected = menu.exec(QCursor::pos());
        if (!selected) return;

        bool isChoice = (selected == actChoice);
        m_graphScene->onAddChildNode(parentItem, isChoice);
    } else {
        // 没有选中节点
        if (m_storyGraph->getRootId().isEmpty()) {
            // 没有任何节点，创建根节点，让用户选择根节点类型
            QMenu menu;
            QAction *actNormal = menu.addAction(tr("普通根节点"));
            QAction *actChoice = menu.addAction(tr("分支根节点"));
            QAction *selected = menu.exec(QCursor::pos());
            if (!selected) return;

            NodeData newNode;
            newNode.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            newNode.type = (selected == actChoice) ? NodeType::Choice : NodeType::Normal;
            newNode.name = (selected == actChoice) ? tr("根节点（分支）") : tr("根节点");
            // 添加默认幻灯片（普通节点需要）-- 由 StoryGraph::addNode 自动处理
            m_storyGraph->addNode(newNode, QString());
            // 刷新视图
            m_graphScene->refreshFromGraph();
            // 居中显示根节点
            QTimer::singleShot(50, this, &MainWindow::centerViewOnRootNode);
        } else {
            QMessageBox::information(this, tr("提示"), tr("请先选中一个节点，再添加子节点。"));
        }
    }
}

void MainWindow::onDeleteSelectedNode()
{
    QString curId = m_graphScene->selectedNodeId();
    if (curId.isEmpty()) return;
    NodeGraphicsItem *item = m_graphScene->findNodeItem(curId);
    if (item)
        m_graphScene->onDeleteNode(item);
}

void MainWindow::onRenameSelectedNode()
{
    QString curId = m_graphScene->selectedNodeId();
    if (curId.isEmpty()) return;
    NodeGraphicsItem *item = m_graphScene->findNodeItem(curId);
    if (item)
        m_graphScene->onRenameNode(item);
}
void MainWindow::onPreview()
{
    if (!ProjectManager::instance().isProjectOpen()) {
        QMessageBox::warning(this, tr("警告"), tr("请先新建或打开工程"));
        return;
    }

    // 播放器 exe 路径（假定放在编辑器 exe 所在目录下的 player 子文件夹）
    QString playerPath = QCoreApplication::applicationDirPath() + "/player/BranchCraftPlay.exe";
    if (!QFile::exists(playerPath)) {
        QMessageBox::critical(this, tr("错误"),
                              tr("找不到播放器，请确保播放器文件位于:\n%1").arg(playerPath));
        return;
    }

    QString projectPath = ProjectManager::instance().currentProjectPath();
    QStringList args;
    args << projectPath;

    // 启动播放器，不等待结束
    if (!QProcess::startDetached(playerPath, args)) {
        QMessageBox::critical(this, tr("错误"), tr("无法启动播放器进程"));
    }
}
void PropertyPanel::refreshCurrentNode()
{
    if (m_loading) return;
    NodeData fresh = m_graph->getNode(m_currentNode.id);
    if (fresh.id.isEmpty()) return;

    m_loading = true;
    // 保留当前幻灯片索引
    int oldIndex = m_currentSlideIndex;
    m_currentNode = fresh;

    // 修正索引边界
    if (m_currentSlideIndex >= m_currentNode.slides.size())
        m_currentSlideIndex = m_currentNode.slides.isEmpty() ? 0 : m_currentNode.slides.size() - 1;
    if (m_currentSlideIndex < 0) m_currentSlideIndex = 0;

    // 只刷新 UI 中可能被外部修改的部分（轻量级）
    refreshCurrentSlideUI();
    updateNodeNameUI();

    m_loading = false;
}
void PropertyPanel::updateCharacterNameRect(int charIndex, const QRectF &rect)
{
    if (m_loading) return;
    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= m_currentNode.slides.size()) return;
    if (charIndex < 0 || charIndex >= m_currentNode.slides[m_currentSlideIndex].characters.size()) return;

    // 更新工作副本中的名字矩形
    auto &ch = m_currentNode.slides[m_currentSlideIndex].characters[charIndex];
    ch.nameRect = rect;

    // 如果当前选中的角色正是这个，同步更新 spinbox
    if (m_characterList->currentRow() == charIndex) {
        QSignalBlocker bx(m_charNameXSpin), by(m_charNameYSpin), bw(m_charNameWSpin), bh(m_charNameHSpin);
        m_charNameXSpin->setValue(rect.x());
        m_charNameYSpin->setValue(rect.y());
        m_charNameWSpin->setValue(rect.width());
        m_charNameHSpin->setValue(rect.height());
    }

    // 注意：不保存到 graph，因为调用者（SceneCanvas）已经保存过了
}

void MainWindow::centerViewOnRootNode()
{
    if (!m_graphView || !m_graphScene) return;

    QString rootId = m_graphScene->getRootNodeId();
    if (rootId.isEmpty()) {
        // 没有根节点，尝试适应整个场景
        m_graphView->fitToView();
        return;
    }

    NodeGraphicsItem *rootItem = m_graphScene->findNodeItem(rootId);
    if (rootItem) {
        QPointF itemCenter = rootItem->scenePos() + rootItem->boundingRect().center();
        m_graphView->centerOn(itemCenter);
    } else {
        // 节点图形项不存在（可能尚未创建），延迟再试一次
        QTimer::singleShot(100, this, &MainWindow::centerViewOnRootNode);
    }
}
