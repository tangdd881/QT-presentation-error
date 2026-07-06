#ifndef WELCOMEWIDGET_H
#define WELCOMEWIDGET_H

#include <QWidget>
#include <QListWidget>

class WelcomeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WelcomeWidget(QWidget *parent = nullptr);

    void updateRecentProjects(const QStringList &recentPaths);  // 刷新近期工程列表

signals:
    void newProjectRequested();
    void openProjectRequested();
    void demoProjectRequested();                      // 功能演示
    void recentProjectRequested(const QString &path); // 打开近期工程

private:
    QListWidget *m_recentList = nullptr;
};

#endif // WELCOMEWIDGET_H