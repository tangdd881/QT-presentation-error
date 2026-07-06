#ifndef MATERIALWIDGET_H
#define MATERIALWIDGET_H

#include <QWidget>

class QListWidgetItem;
class QListWidget;
class QPushButton;

class MaterialWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MaterialWidget(QWidget *parent = nullptr);
    void updateMaterialList(const QStringList &fileNames);
    QString getSelectedMaterialPath() const;
    QStringList getAllMaterialPaths() const;
    QStringList getMaterialPathsBySuffix(const QStringList &suffixes) const;

signals:
    void importRequested();
    void refreshRequested();
    void materialRenameRequested(const QString &oldName, const QString &newName);
    void materialDeleteRequested(const QString &relPath);
    void materialReplaceRequested(const QString &oldRelPath, const QString &newFilePath);
    void previewRequested(const QString &fileName);


private slots:
    void onImportClicked();
    void onRefreshClicked();
    void onItemDoubleClicked(QListWidgetItem *item);
    void onContextMenu(const QPoint &pos);
    void onRename();
    void onDelete();
    void onReplace();

private:
    void setupUI();

    QListWidget *m_listWidget;
    QPushButton *m_importBtn;
    QPushButton *m_refreshBtn;
    QStringList m_materialPaths;
};

#endif // MATERIALWIDGET_H