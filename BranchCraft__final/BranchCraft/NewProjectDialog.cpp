#include "NewProjectDialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include "ProjectManager.h"
#include <QMessageBox>

NewProjectDialog::NewProjectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("新建工程"));
    setMinimumWidth(450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 工程名称
    QHBoxLayout *nameLayout = new QHBoxLayout;
    QLabel *nameLabel = new QLabel(tr("工程名称："), this);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("我的互动视频"));
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_nameEdit);
    mainLayout->addLayout(nameLayout);

    // 工程路径
    QHBoxLayout *pathLayout = new QHBoxLayout;
    QLabel *pathLabel = new QLabel(tr("保存位置："), this);
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setReadOnly(true);
    m_browseBtn = new QPushButton(tr("浏览..."), this);
    connect(m_browseBtn, &QPushButton::clicked, this, &NewProjectDialog::onBrowse);
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(m_browseBtn);
    mainLayout->addLayout(pathLayout);

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_okBtn = new QPushButton(tr("确定"), this);
    m_cancelBtn = new QPushButton(tr("取消"), this);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addStretch();
    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);
}

void NewProjectDialog::accept()
{
    QString name = m_nameEdit->text().trimmed();
    QString path = m_pathEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("工程名称不能为空"));
        return;
    }
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请选择保存位置"));
        return;
    }

    if (ProjectManager::instance().createNewProject(name, path)) {
        QDialog::accept();   // 创建成功，关闭对话框
    } else {
        QMessageBox::critical(this, tr("失败"), tr("创建工程失败，可能文件夹已存在或权限不足"));
    }
}

void NewProjectDialog::onBrowse()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择工程保存目录"));
    if (!dir.isEmpty()) {
        m_pathEdit->setText(dir);
    }
}

QString NewProjectDialog::projectName() const
{
    return m_nameEdit->text();
}

QString NewProjectDialog::projectPath() const
{
    return m_pathEdit->text();
}