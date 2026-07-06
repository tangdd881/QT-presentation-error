#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;

class NewProjectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NewProjectDialog(QWidget *parent = nullptr);
    QString projectName() const;
    QString projectPath() const;
    void accept() override;

private slots:
    void onBrowse();

private:
    QLineEdit *m_nameEdit;
    QLineEdit *m_pathEdit;
    QPushButton *m_browseBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;
};

#endif // NEWPROJECTDIALOG_H