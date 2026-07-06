#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QObject>
#include <QString>

class ProjectManager : public QObject
{
    Q_OBJECT
public:
    static ProjectManager& instance();

    // 工程操作
    bool createNewProject(const QString &name, const QString &path);
    bool openProject(const QString &projectFile);
    bool saveProject();                    // 保存当前工程（保存 story.json + project.ist）
    bool saveAsProject(const QString &newPath);
    bool closeProject();                   // 关闭当前工程，清空内存

    // 获取信息
    QString currentProjectPath() const { return m_projectPath; }
    QString currentProjectName() const { return m_projectName; }
    bool isProjectOpen() const { return !m_projectPath.isEmpty(); }

    // 剧情文件路径
    QString storyFilePath() const { return m_projectPath + "/story.json"; }

    //最近工程管理
    void addToRecentProjects(const QString &projectPath);
    QStringList getRecentProjects() const;
    static constexpr int MAX_RECENT = 10;

signals:
    void projectChanged();  // 工程发生改变（新建/打开/关闭/保存）时发出

    void recentProjectsChanged();   // 可选，用于通知界面刷新



private:
    ProjectManager();
    bool saveProjectMetadata();   // 保存 project.ist
    bool saveStoryData();         // 保存剧情树到 story.json（剧情数据由外部提供，目前留空）

    QString m_projectPath;
    QString m_projectName;

    void saveRecentProjects();
    void loadRecentProjects();
    QStringList m_recentProjects;
};

#endif // PROJECTMANAGER_H