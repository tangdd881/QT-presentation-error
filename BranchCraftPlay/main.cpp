#include "PlayerMainWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("互动剧情播放器");
    a.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("互动剧情播放器 - 播放由编辑器导出的互动视频工程");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("projectPath", "工程文件夹路径（包含story.json和resources目录）");

    parser.process(a);

    QString projectPath;
    QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        projectPath = args.first();
    } else {
        // 默认使用当前程序所在目录
        projectPath = QCoreApplication::applicationDirPath();
    }

    QDir dir(projectPath);
    if (!dir.exists()) {
        qWarning() << "工程路径不存在:" << projectPath;
        return -1;
    }

    PlayerMainWindow w(projectPath);
    w.show();
    return a.exec();
}