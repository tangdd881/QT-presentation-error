#include "ProjectManager.h"
#include "qjsonarray.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QSaveFile>
#include <QUuid>
#include <QSettings>
#include <QCoreApplication>

ProjectManager::ProjectManager()
{
    loadRecentProjects();
}


ProjectManager& ProjectManager::instance()
{
    static ProjectManager inst;
    return inst;
}

bool ProjectManager::createNewProject(const QString &name, const QString &path)
{
    // 1. 创建工程文件夹
    QDir baseDir(path);
    if (!baseDir.exists() && !baseDir.mkpath(".")) {
        qWarning() << "Failed to create base path:" << path;
        return false;
    }

    QString projectFolder = baseDir.filePath(name);
    if (QDir(projectFolder).exists()) {
        qWarning() << "Project folder already exists:" << projectFolder;
        return false;
    }
    if (!baseDir.mkdir(name)) {
        qWarning() << "Failed to create project folder:" << projectFolder;
        return false;
    }

    // 2. 创建子目录 resources/
    QDir projDir(projectFolder);
    if (!projDir.mkdir("resources")) {
        qWarning() << "Failed to create resources folder";
        return false;
    }

    // 3. 保存工程元数据 project.ist
    QJsonObject meta;
    meta["name"] = name;
    meta["version"] = "1.0";
    QFile metaFile(projDir.filePath("project.ist"));
    if (!metaFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot write project.ist";
        return false;
    }
    metaFile.write(QJsonDocument(meta).toJson());
    metaFile.close();

    // 4. 创建空的 story.json（后续编辑剧情时会填充）
    QFile storyFile(projDir.filePath("story.json"));
    if (!storyFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot create story.json";
        return false;
    }

    // 构建一个包含开始节点的 JSON
    // 构建一个包含开始节点的 JSON
    QJsonObject storyObj;
    QJsonArray nodesArray;
    QJsonObject startNode;
    QString startId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    startNode["id"] = startId;
    startNode["type"] = "normal";
    startNode["name"] = "开始";
    startNode["audio"] = "";
    startNode["parentId"] = "";
    startNode["scenePosX"] = 0;
    startNode["scenePosY"] = 0;

    // 创建默认幻灯片（包含音频字段）
    QJsonArray slidesArray;
    QJsonObject defaultSlide;
    defaultSlide["background"] = "";
    defaultSlide["text"] = "";
    defaultSlide["duration"] = 0.0;
    defaultSlide["textRectX"] = 1920/2 - 7500;
    defaultSlide["textRectY"] = 1080 - 450;
    defaultSlide["textRectW"] = 1500;
    defaultSlide["textRectH"] = 400;
    defaultSlide["textFontSize"] = 20;
    defaultSlide["textBgImage"] = "";
    defaultSlide["textColor"] = "#ffffff";
    defaultSlide["characters"] = QJsonArray();
    defaultSlide["layerOrder"] = QJsonArray();
    // 新增音频字段默认值
    defaultSlide["bgmAction"] = 0;        // BgmAction::Continue
    defaultSlide["bgmPath"] = "";
    defaultSlide["slideAudioPath"] = "";
    slidesArray.append(defaultSlide);
    startNode["slides"] = slidesArray;

    nodesArray.append(startNode);
    storyObj["nodes"] = nodesArray;
    storyObj["rootId"] = startId;

    storyFile.write(QJsonDocument(storyObj).toJson());
    storyFile.close();

    // 5. 记录当前工程
    m_projectPath = projectFolder;
    m_projectName = name;

    addToRecentProjects(projDir.filePath("project.ist"));

    emit projectChanged();
    return true;
}

bool ProjectManager::openProject(const QString &projectFile)
{
    // projectFile 应为 project.ist 的完整路径
    QFileInfo info(projectFile);
    if (!info.exists() || !info.isFile()) {
        qWarning() << "Project file does not exist:" << projectFile;
        return false;
    }

    // 读取元数据
    QFile file(projectFile);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse project.ist:" << err.errorString();
        return false;
    }
    QJsonObject meta = doc.object();
    QString projName = meta["name"].toString("Untitled");
    QString projPath = info.absolutePath();   // 工程文件夹路径

    // 检查 story.json 是否存在
    QString storyPath = projPath + "/story.json";
    if (!QFile::exists(storyPath)) {
        qWarning() << "story.json missing in project folder";
        return false;
    }

    m_projectPath = projPath;
    m_projectName = projName;

    addToRecentProjects(projectFile);

    emit projectChanged();
    return true;
}

bool ProjectManager::saveProject()
{
    if (!isProjectOpen()) return false;
    // 先保存元数据（比如名称可能被修改，但当前我们没让用户改，所以简单重写）
    if (!saveProjectMetadata()) return false;
    // 保存剧情数据（实际剧情数据需要从 StoryGraph 获取，这里暂时留空，由外部调用）
    // 剧情保存由 MainWindow 在需要时调用 saveStoryData() 或单独处理
    return true;
}

bool ProjectManager::saveAsProject(const QString &newPath)
{
    // 简化实现：复制整个工程文件夹到新位置（注意处理资源文件复制）
    Q_UNUSED(newPath);
    qWarning() << "saveAsProject not implemented yet";
    return false;
}

bool ProjectManager::closeProject()
{
    if (!isProjectOpen()) return false;
    m_projectPath.clear();
    m_projectName.clear();
    emit projectChanged();
    return true;
}

bool ProjectManager::saveProjectMetadata()
{
    if (!isProjectOpen()) return false;
    QJsonObject meta;
    meta["name"] = m_projectName;
    meta["version"] = "1.0";
    QFile metaFile(m_projectPath + "/project.ist");
    if (!metaFile.open(QIODevice::WriteOnly)) return false;
    metaFile.write(QJsonDocument(meta).toJson());
    metaFile.close();
    return true;
}

bool ProjectManager::saveStoryData()
{
    // 实际剧情数据由 MainWindow 中的 StoryGraph 提供
    // 这个函数会被外部调用，传入剧情 JSON
    return false;
}

void ProjectManager::addToRecentProjects(const QString &projectPath)
{
    if (projectPath.isEmpty()) return;
    m_recentProjects.removeAll(projectPath);
    m_recentProjects.prepend(projectPath);
    while (m_recentProjects.size() > MAX_RECENT) m_recentProjects.removeLast();
    saveRecentProjects();
    emit recentProjectsChanged();
}

QStringList ProjectManager::getRecentProjects() const
{
    return m_recentProjects;
}

void ProjectManager::saveRecentProjects()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("recentProjects", m_recentProjects);
}

void ProjectManager::loadRecentProjects()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    m_recentProjects = settings.value("recentProjects").toStringList();
}