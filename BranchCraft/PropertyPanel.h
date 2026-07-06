#ifndef PROPERTYPANEL_H
#define PROPERTYPANEL_H

#include <QWidget>
#include <QStackedWidget>
#include "StoryGraph.h"
#include "qtabwidget.h"
#include "MaterialWidget.h"

class QLabel;
class QLineEdit;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QListWidget;
class QTextEdit;

class PropertyPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PropertyPanel(StoryGraph *graph, MaterialWidget *materialWidget, QWidget *parent = nullptr);
    ~PropertyPanel() = default;

public slots:
    void setCurrentNode(const QString &nodeId);
    void refreshCurrentNode();
    void updateCharacterNameRect(int charIndex, const QRectF &rect);
    void updateTextRect(const QRectF &rect);
    void onNodeDataChanged(const QString &nodeId);
    void onLinkToExistingNode();

signals:
    void dataChanged(const QString &nodeId);
    void graphDataChanged();
    void slideChanged(int index);

private slots:
    void onNodeNameChanged();
    void onBackgroundImageChanged();
    void onTextChanged();
    void onSlideAudioChanged();
    void onBgmActionChanged();
    void onDurationChanged();
    void onAddCharacter();
    void onRemoveCharacter();
    void onTextRectChanged();
    void onTextFontSizeChanged();
    void onBgmPathChanged();
    void onTextBgImageChanged();
    void onCharacterScaleChanged(double scale);
    void onCharacterSelected(int row);
    void onTextColorChanged();
    void onBrowseCharNameBg();
    void onCharacterNameRectChanged();
    void onCharacterNameChanged();
    void onCharacterNameFontChanged();
    void onCharacterNameColorChanged();
    void onCharacterContextMenu(const QPoint &pos);

    // 分支节点槽函数
    void onAddChoice();                // 添加新选项并创建新节点
          // 新：链接到已有节点
    void onRemoveChoice();
    void onChoiceSelected(int row);
    void onChoiceFontSizeChanged();
    void onChoiceColorChanged();
    void onChoiceSizeChanged();
    void saveCurrentChoice();          // 保存当前编辑的选项

    void onPrevSlide();
    void onNextSlide();
    void onAddSlide();
    void onRemoveSlide();
    void onCopySlide();

private:
    void setupUI();
    void setupTextPage();
    void setupBackgroundPage();
    void setupCharactersPage();
    void setupMediaPage();
    void setupChoicePage();

    void saveCurrentSlideToData();
    void saveCurrentNode();
    void updateUIFromCurrentSlide();
    void refreshCurrentSlideUI();
    void updateNodeNameUI();
    void loadSlideToUI(int index);
    void updateSlideNavigator();
    void updateChoicePageVisibility();
    void enableCharacterDetailControls(bool enable);
    void loadChoiceToUI(int row);
    void refreshTargetNodeCombo();      // 刷新目标节点下拉框内容
    void addChoiceAndRefresh(const NodeData::Choice &newChoice);
    void processPendingRefresh();
    // 在 private 区域添加
    void updateNodeChoice(int choiceIndex, const NodeData::Choice &newChoice); // 轻量更新单个选项
    void refreshChoiceUI();  // 刷新当前选项的显示（不触发保存）

    QString selectMaterialFromLibrary(const QStringList &suffixes, const QString &title);
    QString getCurrentProjectPath() const;

    StoryGraph *m_graph;
    MaterialWidget *m_materialWidget;
    NodeData m_currentNode;
    int m_currentSlideIndex = 0;
    bool m_loading;

    QTabWidget *m_tabWidget;
    QWidget *m_choicePage;

    QLabel *m_slideLabel;
    QPushButton *m_prevSlideBtn, *m_nextSlideBtn;
    QPushButton *m_addSlideBtn, *m_removeSlideBtn, *m_copySlideBtn;

    QLineEdit *m_nodeNameEdit;
    QLineEdit *m_imagePathEdit;
    QPushButton *m_browseImageBtn;
    //音频
    QLineEdit *m_slideAudioEdit = nullptr;
    QComboBox *m_bgmActionCombo = nullptr;
    QLineEdit *m_bgmPathEdit = nullptr;
    QPushButton *m_browseBgmBtn;   // 背景音乐浏览按钮

    QDoubleSpinBox *m_durationSpin;
    QDoubleSpinBox *m_charScaleSpin;
    QDoubleSpinBox *m_textXSpin, *m_textYSpin, *m_textWSpin, *m_textHSpin;
    QSpinBox *m_fontSizeSpin;
    QLineEdit *m_textBgEdit;
    QPushButton *m_textColorBtn;
    QColor m_selectedTextColor;

    QLineEdit *m_charNameEdit;
    QPushButton *m_charNameColorBtn;
    QSpinBox *m_charNameFontSpin;
    QLineEdit *m_charNameBgEdit;
    QPushButton *m_browseCharNameBgBtn;
    QDoubleSpinBox *m_charNameXSpin, *m_charNameYSpin, *m_charNameWSpin, *m_charNameHSpin;

    QTextEdit *m_textEdit;
    QListWidget *m_characterList;
    QPushButton *m_addCharBtn, *m_removeCharBtn;

    QListWidget *m_choiceList;
    QPushButton *m_addChoiceBtn, *m_removeChoiceBtn, *m_linkExistingBtn;   // 新增链接已有节点按钮
    QLineEdit *m_choiceTextEdit;
    QLineEdit *m_choiceImageEdit;
    QPushButton *m_browseChoiceImageBtn;
    QComboBox *m_targetNodeCombo;
    QSpinBox *m_choiceFontSizeSpin;
    QPushButton *m_choiceColorBtn;
    QColor m_selectedChoiceColor;
    QDoubleSpinBox *m_choiceWidthSpin, *m_choiceHeightSpin;

    //防止信号重入变量
    bool m_pendingRefresh;   // 是否有待处理的刷新
    QString m_pendingNodeId;
};

#endif // PROPERTYPANEL_H