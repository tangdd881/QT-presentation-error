#include "MaterialWidget.h"
#include "PreviewDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>

MaterialWidget::MaterialWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(150);
    setupUI();
}

void MaterialWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *title = new QLabel(tr("素材库"), this);
    title->setStyleSheet("font-weight: bold;");
    mainLayout->addWidget(title);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &MaterialWidget::onItemDoubleClicked);
    connect(m_listWidget, &QListWidget::customContextMenuRequested, this, &MaterialWidget::onContextMenu);
    mainLayout->addWidget(m_listWidget);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_importBtn = new QPushButton(tr("导入素材"), this);
    m_refreshBtn = new QPushButton(tr("刷新"), this);
    connect(m_importBtn, &QPushButton::clicked, this, &MaterialWidget::onImportClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MaterialWidget::onRefreshClicked);
    btnLayout->addWidget(m_importBtn);
    btnLayout->addWidget(m_refreshBtn);
    mainLayout->addLayout(btnLayout);
}

void MaterialWidget::onImportClicked()
{
    emit importRequested();
    // 实际导入由主窗口处理，这里仅发送信号
}

void MaterialWidget::onRefreshClicked()
{
    emit refreshRequested();
}

void MaterialWidget::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    QString fileName = item->text();
    // 需要从工程路径解析绝对路径
    // 这里需要通过信号或者获取工程路径，简单起见发射一个信号给 MainWindow 处理
    emit previewRequested(fileName);
}

void MaterialWidget::updateMaterialList(const QStringList &fileNames)
{
    m_listWidget->clear();
    m_materialPaths = fileNames;
    for (const QString &name : fileNames) {
        // 只显示文件名，不显示路径前缀
        QFileInfo info(name);
        m_listWidget->addItem(info.fileName());
    }
}

QString MaterialWidget::getSelectedMaterialPath() const
{
    int row = m_listWidget->currentRow();
    if (row >= 0 && row < m_materialPaths.size()) {
        return m_materialPaths.at(row);
    }
    return QString();
}


void MaterialWidget::onContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_listWidget->itemAt(pos);
    if (!item) return;
    QMenu menu;
    menu.addAction(tr("重命名"), this, &MaterialWidget::onRename);
    menu.addAction(tr("删除"), this, &MaterialWidget::onDelete);
    menu.addAction(tr("替换"), this, &MaterialWidget::onReplace);
    menu.exec(m_listWidget->mapToGlobal(pos));
}

void MaterialWidget::onRename()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item) return;
    QString oldName = item->text();
    bool ok;
    QString newName = QInputDialog::getText(this, tr("重命名素材"),
                                            tr("新名称:"), QLineEdit::Normal, oldName, &ok);
    if (ok && !newName.isEmpty() && newName != oldName) {
        emit materialRenameRequested(oldName, newName);
    }
}

void MaterialWidget::onDelete()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item) return;
    int ret = QMessageBox::question(this, tr("删除"), tr("确定删除素材？此操作会从工程中移除文件。"),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        QString relPath = getSelectedMaterialPath();
        if (!relPath.isEmpty()) {
            // 发送删除请求到 MainWindow 执行实际文件删除
            emit materialDeleteRequested(relPath);
        }
    }
}

void MaterialWidget::onReplace()
{
    // 类似导入，但替换选中的文件，需先删除旧文件再复制新文件
    QString oldRelPath = getSelectedMaterialPath();
    if (oldRelPath.isEmpty()) return;
    QString newFile = QFileDialog::getOpenFileName(this, tr("选择新素材"),
                                                   QString(), tr("媒体文件 (*.mp4 *.jpg *.png *.mp3 *.wav)"));
    if (!newFile.isEmpty()) {
        emit materialReplaceRequested(oldRelPath, newFile);
    }
}

QStringList MaterialWidget::getAllMaterialPaths() const
{
    return m_materialPaths;
}

QStringList MaterialWidget::getMaterialPathsBySuffix(const QStringList &suffixes) const
{
    QStringList result;
    for (const QString &path : m_materialPaths) {
        QFileInfo info(path);
        if (suffixes.contains(info.suffix(), Qt::CaseInsensitive)) {
            result.append(path);
        }
    }
    return result;
}