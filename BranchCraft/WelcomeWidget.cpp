#include "WelcomeWidget.h"
#include "StyleSheets.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QListWidget>
#include <QFileInfo>
#include <QDir>

WelcomeWidget::WelcomeWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("WelcomeWidget");

    setStyleSheet(
        StyleSheets::WELCOME_WIDGET
        );

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(48, 48, 48, 48);
    outerLayout->addStretch();

    auto *centerLayout = new QHBoxLayout;
    centerLayout->addStretch();

    auto *panel = new QFrame(this);
    panel->setObjectName("WelcomePanel");
    panel->setMinimumWidth(520);
    panel->setMaximumWidth(680);

    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(42, 38, 42, 38);
    panelLayout->setSpacing(18);

    // 标题
    auto *titleLabel = new QLabel(tr("BranchCraft"), panel);
    titleLabel->setObjectName("TitleLabel");
    panelLayout->addWidget(titleLabel);

    // 副标题
    auto *subtitleLabel = new QLabel(tr("创建、编辑并预览互动分支剧情项目。"), panel);
    subtitleLabel->setObjectName("SubtitleLabel");
    subtitleLabel->setWordWrap(true);
    panelLayout->addWidget(subtitleLabel);

    panelLayout->addSpacing(12);

    // 新建项目
    auto *newProjectButton = new QPushButton(tr("新建项目"), panel);
    newProjectButton->setObjectName("PrimaryButton");
    connect(newProjectButton, &QPushButton::clicked, this, &WelcomeWidget::newProjectRequested);
    panelLayout->addWidget(newProjectButton);

    // 打开已有项目
    auto *openProjectButton = new QPushButton(tr("打开已有项目"), panel);
    openProjectButton->setObjectName("SecondaryButton");
    connect(openProjectButton, &QPushButton::clicked, this, &WelcomeWidget::openProjectRequested);
    panelLayout->addWidget(openProjectButton);

    // 功能演示按钮
    auto *demoButton = new QPushButton(tr("功能演示"), panel);
    demoButton->setObjectName("SecondaryButton");
    connect(demoButton, &QPushButton::clicked, this, &WelcomeWidget::demoProjectRequested);
    panelLayout->addWidget(demoButton);

    panelLayout->addSpacing(8);

    // 近期工程标题和列表
    auto *recentLabel = new QLabel(tr("近期工程"), panel);
    recentLabel->setObjectName("SubtitleLabel");
    panelLayout->addWidget(recentLabel);

    m_recentList = new QListWidget(panel);
    m_recentList->setObjectName("RecentList");
    m_recentList->setMaximumHeight(200);
    m_recentList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_recentList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_recentList->setAttribute(Qt::WA_StyledBackground, true);

    // 【新增修改点】：使用 setSpacing 为各个记录卡片之间增加间距
    m_recentList->setSpacing(6); // 这里的 6 就是各项之间的像素间距，可以根据喜好调整大小

    connect(m_recentList, &QListWidget::itemClicked, [this](QListWidgetItem *item){
        QString path = item->data(Qt::UserRole).toString();
        emit recentProjectRequested(path);
    });
    panelLayout->addWidget(m_recentList);

    // 提示文字
    auto *hintLabel = new QLabel(tr("项目文件为 project.ist，剧情数据保存在 story.json 中。"), panel);
    hintLabel->setObjectName("SubtitleLabel");
    hintLabel->setWordWrap(true);
    panelLayout->addWidget(hintLabel);

    centerLayout->addWidget(panel);
    centerLayout->addStretch();

    outerLayout->addLayout(centerLayout);
    outerLayout->addStretch();
}

void WelcomeWidget::updateRecentProjects(const QStringList &recentPaths)
{
    m_recentList->clear();
    for (const QString &path : recentPaths) {
        QFileInfo info(path);
        QString projectName = info.dir().dirName();  // 获取工程文件夹的名称
        QListWidgetItem *item = new QListWidgetItem(projectName);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        m_recentList->addItem(item);
    }
}