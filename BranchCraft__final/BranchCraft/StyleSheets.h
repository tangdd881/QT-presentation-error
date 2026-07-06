#ifndef STYLESHEETS_H
#define STYLESHEETS_H

#include <QString>

namespace StyleSheets {

// 欢迎页样式（完全复用 Gemini 给出的最终版本）
inline const QString WELCOME_WIDGET = R"(
    #WelcomeWidget { background: #f4f6f8; }
    QFrame#WelcomePanel { background: white; border: 1px solid #d8dde3; border-radius: 12px; }
    QLabel#TitleLabel { font-size: 30px; font-weight: 700; color: #20242a; }
    QLabel#SubtitleLabel { font-size: 14px; color: #5f6b7a; }
    QPushButton { min-height: 42px; padding: 0 18px; font-size: 15px; border-radius: 6px; }
    QPushButton#PrimaryButton { background: #2563eb; color: white; border: none; }
    QPushButton#PrimaryButton:hover { background: #1d4ed8; }
    QPushButton#SecondaryButton { background: #ffffff; color: #20242a; border: 1px solid #c7ced8; }
    QPushButton#SecondaryButton:hover { background: #eef2f7; }

    QListWidget#RecentList {
        margin: 2px; border: 1px solid #c7ced8; border-radius: 8px; background: #ffffff;
        font-size: 13px; outline: none; padding: 6px;
    }
    QListWidget#RecentList::viewport {
        background: transparent;
    }
    QListWidget#RecentList::item {
        padding: 8px 12px;
        border-radius: 6px;
        border: 1px solid #e5e9ef;
    }
    QListWidget#RecentList::item:hover {
        background: #eef2f7;
        border: 1px solid #d8dde3;
    }
    QListWidget#RecentList::item:selected {
        background: #2563eb;
        color: white;
        border: 1px solid #2563eb;
    }
)";



// 后续可添加其他界面的样式，例如主窗口、属性面板等
// inline const QString MAIN_WINDOW = "...";

inline const QString MAIN_WINDOW = R"(
    /* 主窗口背景 */
    MainWindow {
        background: #f4f6f8;
    }

    /* 菜单栏 */
    QMenuBar {
        background: #ffffff;
        border-bottom: 1px solid #d8dde3;
        padding: 4px 0px;
    }
    QMenuBar::item {
        background: transparent;
        padding: 6px 12px;
        margin: 0px 2px;
        border-radius: 4px;
    }
    QMenuBar::item:selected {
        background: #eef2f7;
    }
    QMenuBar::item:pressed {
        background: #e2e8f0;
    }

    /* 菜单 */
    QMenu {
        background: #ffffff;
        border: 1px solid #d8dde3;
        border-radius: 6px;
        padding: 4px;
    }
    QMenu::item {
        padding: 6px 24px 6px 20px;
        border-radius: 4px;
    }
    QMenu::item:selected {
        background: #2563eb;
        color: white;
    }
    QMenu::separator {
        height: 1px;
        background: #e5e9ef;
        margin: 4px 8px;
    }

    /* 工具栏 */
    QToolBar {
        background: #ffffff;
        border-bottom: 1px solid #d8dde3;
        spacing: 4px;
        padding: 4px 8px;
    }
    QToolBar QToolButton {
        background: transparent;
        border: none;
        border-radius: 6px;
        padding: 6px 10px;
        color: #20242a;
    }
    QToolBar QToolButton:hover {
        background: #eef2f7;
    }
    QToolBar QToolButton:pressed {
        background: #e2e8f0;
    }

    /* 状态栏 */
    QStatusBar {
        background: #ffffff;
        border-top: 1px solid #d8dde3;
        color: #5f6b7a;
        padding: 4px 8px;
    }

    /* 分割器手柄 */
    QSplitter::handle {
        background: #d8dde3;
        width: 2px;
        height: 2px;
    }
    QSplitter::handle:horizontal {
        width: 2px;
    }
    QSplitter::handle:vertical {
        height: 2px;
    }
)";

} // namespace StyleSheets



#endif // STYLESHEETS_H