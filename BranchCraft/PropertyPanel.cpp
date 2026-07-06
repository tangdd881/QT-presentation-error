#include "PropertyPanel.h"
#include "ProjectManager.h"
#include "qtimer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QTextEdit>
#include <QInputDialog>
#include <QTabWidget>
#include <QMenu>
#include <QCursor>
#include<QColorDialog>

PropertyPanel::PropertyPanel(StoryGraph *graph, MaterialWidget *materialWidget, QWidget *parent)
    : QWidget(parent), m_graph(graph), m_materialWidget(materialWidget), m_loading(false)
{
    setMinimumWidth(200);
    // 创建节点名称编辑框（放在顶部，所有页面共享）
    m_nodeNameEdit = new QLineEdit(this);
    connect(m_nodeNameEdit, &QLineEdit::textChanged, this, &PropertyPanel::onNodeNameChanged);

    // 创建各个选项卡页面需要的控件
    // 背景图片
    m_imagePathEdit = new QLineEdit(this);
    m_imagePathEdit->setReadOnly(true);
    m_browseImageBtn = new QPushButton(tr("浏览"), this);
    connect(m_browseImageBtn, &QPushButton::clicked, this, &PropertyPanel::onBackgroundImageChanged);

    // 文字内容
    m_textEdit = new QTextEdit(this);
    connect(m_textEdit, &QTextEdit::textChanged, this, &PropertyPanel::onTextChanged);

    // 角色列表
    m_characterList = new QListWidget(this);
    m_characterList->setMaximumHeight(150);
    m_addCharBtn = new QPushButton(tr("添加角色"));
    m_removeCharBtn = new QPushButton(tr("删除角色"));
    connect(m_addCharBtn, &QPushButton::clicked, this, &PropertyPanel::onAddCharacter);
    connect(m_removeCharBtn, &QPushButton::clicked, this, &PropertyPanel::onRemoveCharacter);
    // 分支选项专用控件
    m_choiceList = new QListWidget(this);
    m_choiceList->setMaximumHeight(120);
    m_addChoiceBtn = new QPushButton(tr("新建节点并链接"));
    m_linkExistingBtn = new QPushButton(tr("链接到已有节点"));
    m_removeChoiceBtn = new QPushButton(tr("删除选项"));
    m_choiceTextEdit = new QLineEdit(this);
    m_choiceImageEdit = new QLineEdit(this);
    m_choiceImageEdit->setReadOnly(true);
    m_browseChoiceImageBtn = new QPushButton(tr("浏览"), this);
    m_targetNodeCombo = new QComboBox(this);
    m_choiceFontSizeSpin = new QSpinBox;
    m_choiceColorBtn = new QPushButton;
    connect(m_addChoiceBtn, &QPushButton::clicked, this, &PropertyPanel::onAddChoice);
    connect(m_linkExistingBtn, &QPushButton::clicked, this, &PropertyPanel::onLinkToExistingNode);
    connect(m_removeChoiceBtn, &QPushButton::clicked, this, &PropertyPanel::onRemoveChoice);
    connect(m_choiceList, &QListWidget::currentRowChanged, this, &PropertyPanel::onChoiceSelected);
    connect(m_browseChoiceImageBtn, &QPushButton::clicked, [this]() {
        QString relPath = selectMaterialFromLibrary({"jpg","png","jpeg"}, tr("选择按钮图片"));
        if (!relPath.isEmpty()) m_choiceImageEdit->setText(relPath);
    });

    // 整体布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 节点名称区域（固定顶部）
    QHBoxLayout *nameLayout = new QHBoxLayout;
    nameLayout->addWidget(new QLabel(tr("节点名称:")));
    nameLayout->addWidget(m_nodeNameEdit);
    mainLayout->addLayout(nameLayout);

    //幻灯片导航栏
    QHBoxLayout *navLayout = new QHBoxLayout;
    m_prevSlideBtn = new QPushButton("<");
    m_nextSlideBtn = new QPushButton(">");
    m_slideLabel = new QLabel(tr("幻灯片: 0/0"));
    navLayout->addWidget(m_prevSlideBtn);
    navLayout->addWidget(m_slideLabel);
    navLayout->addWidget(m_nextSlideBtn);
    navLayout->addStretch();
    m_addSlideBtn = new QPushButton(tr("+"));
    m_removeSlideBtn = new QPushButton(tr("-"));
    m_copySlideBtn = new QPushButton(tr("复制"));
    m_addSlideBtn->setToolTip(tr("添加新幻灯片"));
    m_removeSlideBtn->setToolTip(tr("删除当前幻灯片"));
    m_copySlideBtn->setToolTip(tr("复制当前幻灯片"));
    navLayout->addWidget(m_addSlideBtn);
    navLayout->addWidget(m_removeSlideBtn);
    navLayout->addWidget(m_copySlideBtn);
    mainLayout->addLayout(navLayout);

    connect(m_prevSlideBtn, &QPushButton::clicked, this, &PropertyPanel::onPrevSlide);
    connect(m_nextSlideBtn, &QPushButton::clicked, this, &PropertyPanel::onNextSlide);
    connect(m_addSlideBtn, &QPushButton::clicked, this, &PropertyPanel::onAddSlide);
    connect(m_removeSlideBtn, &QPushButton::clicked, this, &PropertyPanel::onRemoveSlide);
    connect(m_copySlideBtn, &QPushButton::clicked, this, &PropertyPanel::onCopySlide);

    // 选项卡
    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    // 创建各个页面（不包含分支选项页，因为它需要动态添加）
    setupTextPage();
    setupBackgroundPage();
    setupCharactersPage();
    setupMediaPage();
    setupChoicePage(); // 创建分支选项页面（但暂不添加到选项卡，根据节点类型显示）

}

void PropertyPanel::setupTextPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);

    // 原有的文字编辑框
    layout->addWidget(new QLabel(tr("文字内容:")));
    layout->addWidget(m_textEdit);

    // --- 新增：文本框位置和大小调节 ---
    QGroupBox *posGroup = new QGroupBox(tr("文本框位置/大小"));
    QFormLayout *posLayout = new QFormLayout(posGroup);
    m_textXSpin = new QDoubleSpinBox; m_textXSpin->setRange(-10000, 10000);
    m_textYSpin = new QDoubleSpinBox; m_textYSpin->setRange(-10000, 10000);
    m_textWSpin = new QDoubleSpinBox; m_textWSpin->setRange(50, 3000);
    m_textHSpin = new QDoubleSpinBox; m_textHSpin->setRange(30, 1000);
    posLayout->addRow(tr("X:"), m_textXSpin);
    posLayout->addRow(tr("Y:"), m_textYSpin);
    posLayout->addRow(tr("宽度:"), m_textWSpin);
    posLayout->addRow(tr("高度:"), m_textHSpin);
    layout->addWidget(posGroup);

    // 文字大小
    QHBoxLayout *fontLayout = new QHBoxLayout;
    fontLayout->addWidget(new QLabel(tr("文字大小:")));
    m_fontSizeSpin = new QSpinBox; m_fontSizeSpin->setRange(8, 100);
    fontLayout->addWidget(m_fontSizeSpin);
    layout->addLayout(fontLayout);

    QHBoxLayout *colorLayout = new QHBoxLayout;
    colorLayout->addWidget(new QLabel(tr("文字颜色:")));
    m_textColorBtn = new QPushButton;
    m_textColorBtn->setFixedSize(40, 20);
    // 设置初始颜色显示
    QPalette pal = m_textColorBtn->palette();
    m_textColorBtn->setPalette(pal);
    connect(m_textColorBtn, &QPushButton::clicked, this, &PropertyPanel::onTextColorChanged);
    colorLayout->addWidget(m_textColorBtn);
    layout->addLayout(colorLayout);

    // 文本框背景图片
    QHBoxLayout *bgLayout = new QHBoxLayout;
    bgLayout->addWidget(new QLabel(tr("文本框背景图:")));
    m_textBgEdit = new QLineEdit;
    QPushButton *browseBgBtn = new QPushButton(tr("浏览"));
    bgLayout->addWidget(m_textBgEdit);
    bgLayout->addWidget(browseBgBtn);
    layout->addLayout(bgLayout);

    layout->addStretch();
    m_tabWidget->addTab(page, tr("文字"));

    // 连接信号
    connect(m_textXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onTextRectChanged);
    connect(m_textYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onTextRectChanged);
    connect(m_textWSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onTextRectChanged);
    connect(m_textHSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onTextRectChanged);
    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PropertyPanel::onTextFontSizeChanged);
    connect(browseBgBtn, &QPushButton::clicked, [this](){
        QString relPath = selectMaterialFromLibrary({"jpg","png","jpeg"}, tr("选择文本框背景图"));
        if (!relPath.isEmpty()) {
            m_textBgEdit->setText(relPath);
            onTextBgImageChanged();
        }

    });
}

void PropertyPanel::setupBackgroundPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);
    QHBoxLayout *imageLayout = new QHBoxLayout;
    imageLayout->addWidget(new QLabel(tr("背景图片:")));
    imageLayout->addWidget(m_imagePathEdit);
    imageLayout->addWidget(m_browseImageBtn);
    layout->addLayout(imageLayout);
    layout->addStretch();
    m_tabWidget->addTab(page, tr("背景"));
}

void PropertyPanel::setupCharactersPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);

    // 角色列表区域（不变）
    layout->addWidget(new QLabel(tr("角色列表:")));
    layout->addWidget(m_characterList);
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(m_addCharBtn);
    btnLayout->addWidget(m_removeCharBtn);
    layout->addLayout(btnLayout);

    // ---------- 名字内容 ----------
    QHBoxLayout *nameLayout = new QHBoxLayout;
    nameLayout->addWidget(new QLabel(tr("角色名字:")));
    m_charNameEdit = new QLineEdit(this);
    m_charNameEdit->setEnabled(false);
    connect(m_charNameEdit, &QLineEdit::textChanged, this, &PropertyPanel::onCharacterNameChanged);
    nameLayout->addWidget(m_charNameEdit);
    layout->addLayout(nameLayout);

    // ---------- 名字样式组 ----------
    QGroupBox *styleGroup = new QGroupBox(tr("名字样式"), this);
    QFormLayout *styleLayout = new QFormLayout(styleGroup);

    // 字体大小
    m_charNameFontSpin = new QSpinBox;
    m_charNameFontSpin->setRange(8, 100);
    m_charNameFontSpin->setEnabled(false);
    connect(m_charNameFontSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PropertyPanel::onCharacterNameFontChanged);
    styleLayout->addRow(tr("字体大小:"), m_charNameFontSpin);

    // 文字颜色
    m_charNameColorBtn = new QPushButton;
    m_charNameColorBtn->setFixedSize(40, 20);
    m_charNameColorBtn->setEnabled(false);
    connect(m_charNameColorBtn, &QPushButton::clicked, this, &PropertyPanel::onCharacterNameColorChanged);
    styleLayout->addRow(tr("文字颜色:"), m_charNameColorBtn);

    // 背景图片
    QHBoxLayout *bgLayout = new QHBoxLayout;
    m_charNameBgEdit = new QLineEdit;
    m_charNameBgEdit->setReadOnly(true);
    m_charNameBgEdit->setEnabled(false);
    m_browseCharNameBgBtn = new QPushButton(tr("浏览"));
    m_browseCharNameBgBtn->setEnabled(false);
    connect(m_browseCharNameBgBtn, &QPushButton::clicked, this, &PropertyPanel::onBrowseCharNameBg);
    bgLayout->addWidget(m_charNameBgEdit);
    bgLayout->addWidget(m_browseCharNameBgBtn);
    styleLayout->addRow(tr("背景图片:"), bgLayout);

    // 名字位置（微调，也可由画布拖拽）
    m_charNameXSpin = new QDoubleSpinBox; m_charNameXSpin->setRange(-10000,10000);
    m_charNameYSpin = new QDoubleSpinBox; m_charNameYSpin->setRange(-10000,10000);
    m_charNameWSpin = new QDoubleSpinBox; m_charNameWSpin->setRange(50,2000);
    m_charNameHSpin = new QDoubleSpinBox; m_charNameHSpin->setRange(20,1000);
    connect(m_charNameXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onCharacterNameRectChanged);
    connect(m_charNameYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onCharacterNameRectChanged);
    connect(m_charNameWSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onCharacterNameRectChanged);
    connect(m_charNameHSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onCharacterNameRectChanged);
    QHBoxLayout *posLayout = new QHBoxLayout;
    posLayout->addWidget(new QLabel(tr("X:")));
    posLayout->addWidget(m_charNameXSpin);
    posLayout->addWidget(new QLabel(tr("Y:")));
    posLayout->addWidget(m_charNameYSpin);
    posLayout->addWidget(new QLabel(tr("宽:")));
    posLayout->addWidget(m_charNameWSpin);
    posLayout->addWidget(new QLabel(tr("高:")));
    posLayout->addWidget(m_charNameHSpin);
    styleLayout->addRow(tr("位置/大小:"), posLayout);

    layout->addWidget(styleGroup);

    // 角色图片缩放（原有）
    QHBoxLayout *scaleLayout = new QHBoxLayout;
    scaleLayout->addWidget(new QLabel(tr("角色图片缩放:")));
    m_charScaleSpin = new QDoubleSpinBox;
    m_charScaleSpin->setRange(0.1, 3.0);
    m_charScaleSpin->setSingleStep(0.1);
    m_charScaleSpin->setValue(1.0);
    m_charScaleSpin->setEnabled(false);
    connect(m_charScaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onCharacterScaleChanged);
    scaleLayout->addWidget(m_charScaleSpin);
    layout->addLayout(scaleLayout);

    layout->addStretch();
    m_tabWidget->addTab(page, tr("角色"));
    connect(m_characterList, &QListWidget::currentRowChanged, this, &PropertyPanel::onCharacterSelected);

    // 右键菜单：更换图片、删除角色
    m_characterList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_characterList, &QListWidget::customContextMenuRequested,
            this, &PropertyPanel::onCharacterContextMenu);
}

void PropertyPanel::setupMediaPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);   // page 使用垂直布局

    // ========= 1. 幻灯片独立音频 =========
    QGroupBox *slideAudioGroup = new QGroupBox(tr("幻灯片独立音频（自动播放）"), page);
    QHBoxLayout *audioLayout = new QHBoxLayout(slideAudioGroup);
    m_slideAudioEdit = new QLineEdit(slideAudioGroup);
    m_slideAudioEdit->setReadOnly(true);
    QPushButton *browseSlideAudioBtn = new QPushButton(tr("浏览"), slideAudioGroup);
    audioLayout->addWidget(m_slideAudioEdit);
    audioLayout->addWidget(browseSlideAudioBtn);
    layout->addWidget(slideAudioGroup);

    // ========= 2. 幻灯片停留时长 =========
    QGroupBox *durationGroup = new QGroupBox(tr("幻灯片停留时长"), page);
    QHBoxLayout *durationLayout = new QHBoxLayout(durationGroup);
    m_durationSpin = new QDoubleSpinBox(durationGroup);
    m_durationSpin->setRange(0, 3600);
    m_durationSpin->setSuffix(tr(" 秒"));
    m_durationSpin->setSpecialValueText(tr("一直显示"));
    durationLayout->addWidget(m_durationSpin);
    layout->addWidget(durationGroup);

    // ========= 3. 背景音乐控制 =========
    QGroupBox *bgmGroup = new QGroupBox(tr("背景音乐控制"), page);
    QFormLayout *bgmLayout = new QFormLayout(bgmGroup);

    m_bgmActionCombo = new QComboBox(bgmGroup);
    m_bgmActionCombo->addItem(tr("继续播放"), static_cast<int>(BgmAction::Continue));
    m_bgmActionCombo->addItem(tr("停止播放"), static_cast<int>(BgmAction::Stop));
    m_bgmActionCombo->addItem(tr("切换/设置音乐"), static_cast<int>(BgmAction::Switch));
    bgmLayout->addRow(tr("动作:"), m_bgmActionCombo);

    QHBoxLayout *switchLayout = new QHBoxLayout;
    m_bgmPathEdit = new QLineEdit(bgmGroup);
    m_bgmPathEdit->setReadOnly(true);
    m_browseBgmBtn = new QPushButton(tr("浏览"), bgmGroup);
    switchLayout->addWidget(m_bgmPathEdit);
    switchLayout->addWidget(m_browseBgmBtn);
    bgmLayout->addRow(tr("新背景音乐:"), switchLayout);

    layout->addWidget(bgmGroup);
    layout->addStretch();          // 将剩余空间推到底部，避免控件松散

    // 将页面添加到选项卡
    m_tabWidget->addTab(page, tr("媒体"));

    // ---------- 信号连接 ----------
    connect(browseSlideAudioBtn, &QPushButton::clicked, [this](){
        QString relPath = selectMaterialFromLibrary({"mp3","wav","ogg"}, tr("选择幻灯片音频"));
        if (!relPath.isEmpty()) {
            m_slideAudioEdit->setText(relPath);
            onSlideAudioChanged();
        }
    });
    connect(m_slideAudioEdit, &QLineEdit::textChanged, this, &PropertyPanel::onSlideAudioChanged);

    connect(m_durationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onDurationChanged);

    connect(m_bgmActionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyPanel::onBgmActionChanged);
    connect(m_browseBgmBtn, &QPushButton::clicked, [this](){
        QString relPath = selectMaterialFromLibrary({"mp3","wav","ogg"}, tr("选择背景音乐"));
        if (!relPath.isEmpty()) {
            m_bgmPathEdit->setText(relPath);
            onBgmPathChanged();
        }
    });
    connect(m_bgmPathEdit, &QLineEdit::textChanged, this, &PropertyPanel::onBgmPathChanged);
}
void PropertyPanel::setupChoicePage()
{
    // 分支选项页面（单独页面，不自动添加到选项卡）
    m_choicePage = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_choicePage);

    QGroupBox *choiceGroup = new QGroupBox(tr("分支选项"), this);
    QVBoxLayout *choiceLayout = new QVBoxLayout(choiceGroup);
    choiceLayout->addWidget(m_choiceList);
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(m_addChoiceBtn);
    btnLayout->addWidget(m_linkExistingBtn);
    btnLayout->addWidget(m_removeChoiceBtn);
    choiceLayout->addLayout(btnLayout);

    QGroupBox *editGroup = new QGroupBox(tr("编辑选中选项"), this);
    QFormLayout *editLayout = new QFormLayout(editGroup);
    editLayout->addRow(tr("按钮文字:"), m_choiceTextEdit);
    // 文字大小
    m_choiceFontSizeSpin = new QSpinBox;
    m_choiceFontSizeSpin->setRange(8, 100);
    editLayout->addRow(tr("选项文字大小:"), m_choiceFontSizeSpin);
    // 文字颜色
    m_choiceColorBtn = new QPushButton;
    m_choiceColorBtn->setFixedSize(40, 20);
    editLayout->addRow(tr("选项文字颜色:"), m_choiceColorBtn);


    // 按钮宽度
    m_choiceWidthSpin = new QDoubleSpinBox;
    m_choiceWidthSpin->setRange(30, 800);
    m_choiceWidthSpin->setSingleStep(10);
    editLayout->addRow(tr("按钮宽度:"), m_choiceWidthSpin);

    // 按钮高度
    m_choiceHeightSpin = new QDoubleSpinBox;
    m_choiceHeightSpin->setRange(20, 400);
    m_choiceHeightSpin->setSingleStep(5);
    editLayout->addRow(tr("按钮高度:"), m_choiceHeightSpin);

    QHBoxLayout *imageEditLayout = new QHBoxLayout;
    imageEditLayout->addWidget(m_choiceImageEdit);
    imageEditLayout->addWidget(m_browseChoiceImageBtn);
    editLayout->addRow(tr("按钮图片:"), imageEditLayout);
    editLayout->addRow(tr("跳转节点:"), m_targetNodeCombo);
    choiceLayout->addWidget(editGroup);

    layout->addWidget(choiceGroup);
    layout->addStretch();

    // 文字编辑框
    connect(m_choiceTextEdit, &QLineEdit::textChanged, this, &PropertyPanel::saveCurrentChoice);
    // 图片路径编辑框（通常由浏览按钮设置，手动修改时也可保存）
    connect(m_choiceImageEdit, &QLineEdit::textChanged, this, &PropertyPanel::saveCurrentChoice);
    // 目标节点下拉框
    connect(m_targetNodeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyPanel::saveCurrentChoice);
    // 字体大小
    connect(m_choiceFontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PropertyPanel::onChoiceFontSizeChanged);
    connect(m_choiceColorBtn, &QPushButton::clicked,
            this, &PropertyPanel::onChoiceColorChanged);
    connect(m_choiceWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onChoiceSizeChanged);
    connect(m_choiceHeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyPanel::onChoiceSizeChanged);

    //初始隐藏
    m_choicePage->hide();
}

void PropertyPanel::setCurrentNode(const QString &nodeId)
{
    if (m_loading) {
        m_pendingNodeId = nodeId;
        QMetaObject::invokeMethod(this, "processPendingRefresh", Qt::QueuedConnection);
        return;
    }
    if (nodeId.isEmpty()) return;

    // 获取节点数据，如果无效则直接返回
    NodeData fresh = m_graph->getNode(nodeId);
    if (fresh.id.isEmpty()) {
        qWarning() << "setCurrentNode: node not found" << nodeId;
        return;
    }

    if (m_currentNode.id == nodeId && !m_loading) {
        refreshCurrentSlideUI();
        return;
    }

    if (!m_currentNode.id.isEmpty()) {
        saveCurrentNode();
    }

    m_loading = true;
    m_currentNode = fresh;

    // 修正幻灯片索引
    if (m_currentNode.slides.isEmpty()) {
        // 如果没有幻灯片，创建一个默认幻灯片（防止后续越界）
        Slide defaultSlide;
        defaultSlide.textRect = QRectF(1920/2 - 400, 1080 - 150, 800, 100);
        defaultSlide.textFontSize = 20;
        m_currentNode.slides.append(defaultSlide);
        m_graph->updateNode(m_currentNode);
    }
    if (m_currentSlideIndex >= m_currentNode.slides.size())
        m_currentSlideIndex = m_currentNode.slides.isEmpty() ? 0 : m_currentNode.slides.size() - 1;
    if (m_currentSlideIndex < 0) m_currentSlideIndex = 0;

    // 更新所有 UI
    updateUIFromCurrentSlide();   // 统一更新所有页面（不再单独调用多个函数）
    updateSlideNavigator();
    updateNodeNameUI();
    updateChoicePageVisibility();

    // 刷新分支选项列表（如果是分支节点）
    if (m_currentNode.type == NodeType::Choice) {
        m_choiceList->clear();
        for (const auto &choice : m_currentNode.choices) {
            m_choiceList->addItem(choice.text);
        }
        refreshTargetNodeCombo();

        // 自动选中第一个选项
        if (m_choiceList->count() > 0) {
            if (m_choiceList->currentRow() == -1) {
                m_choiceList->setCurrentRow(0);
                loadChoiceToUI(0);          // 加载第一个选项的详情
            }
        } else {
            // 无选项时清空编辑区
            m_choiceTextEdit->clear();
            m_choiceImageEdit->clear();
            m_targetNodeCombo->clear();
        }
    }

    m_loading = false;
    emit slideChanged(m_currentSlideIndex);
}

void PropertyPanel::processPendingRefresh()
{
    if (!m_pendingNodeId.isEmpty()) {
        QString id = m_pendingNodeId;
        m_pendingNodeId.clear();
        // 再次检查节点是否存在
        if (m_graph->getNode(id).id.isEmpty()) {
            qWarning() << "processPendingRefresh: node no longer exists" << id;
            return;
        }
        setCurrentNode(id);
    }
}

void PropertyPanel::updateNodeChoice(int choiceIndex, const NodeData::Choice &newChoice)
{
    if (choiceIndex < 0 || choiceIndex >= m_currentNode.choices.size()) return;
    m_currentNode.choices[choiceIndex] = newChoice;
    // 仅更新 StoryGraph 中的当前节点，不触发全量 dataChanged 重绘
    NodeData nodeCopy = m_graph->getNode(m_currentNode.id);
    if (nodeCopy.id.isEmpty()) return;
    nodeCopy.choices = m_currentNode.choices;
    m_graph->updateNode(nodeCopy);
    // 不发射 dataChanged，只发射 graphDataChanged 让画布刷新
    emit graphDataChanged();
}

void PropertyPanel::saveCurrentNode()
{
    if (m_currentNode.id.isEmpty()) return;
    NodeData latest = m_graph->getNode(m_currentNode.id);
    if (latest.id.isEmpty()) return;

    latest.name = m_nodeNameEdit->text();
    // latest.audioPath = m_audioPathEdit->text();  // ← 注释或删除这一行，因为 m_audioPathEdit 未初始化
    latest.slides = m_currentNode.slides;
    if (latest.type == NodeType::Choice) {
        latest.choices = m_currentNode.choices;
    }

    m_graph->updateNode(latest);
    m_currentNode = latest;
    emit dataChanged(m_currentNode.id);
}

void PropertyPanel::saveCurrentChoice()
{
    if (m_loading) return;
    int row = m_choiceList->currentRow();
    if (row < 0 || row >= m_currentNode.choices.size()) return;

    QString targetId = m_targetNodeCombo->currentData().toString();
    // 如果目标节点 ID 非空但不存在于剧情图中，则放弃保存并恢复原值
    if (!targetId.isEmpty() && m_graph->getNode(targetId).id.isEmpty()) {
        qWarning() << "Invalid target node id:" << targetId << " - ignoring save";
        // 恢复 UI 显示为原选项的目标 ID
        const auto &origChoice = m_currentNode.choices[row];
        int origIdx = m_targetNodeCombo->findData(origChoice.targetNodeId);
        QSignalBlocker blocker(m_targetNodeCombo);
        if (origIdx >= 0)
            m_targetNodeCombo->setCurrentIndex(origIdx);
        else
            m_targetNodeCombo->setCurrentIndex(-1);
        return;
    }

    NodeData::Choice newChoice = m_currentNode.choices[row];
    newChoice.text = m_choiceTextEdit->text();
    newChoice.imagePath = m_choiceImageEdit->text();
    newChoice.targetNodeId = targetId;
    newChoice.textFontSize = m_choiceFontSizeSpin->value();
    newChoice.textColor = m_selectedChoiceColor;
    newChoice.buttonSize = QSizeF(m_choiceWidthSpin->value(), m_choiceHeightSpin->value());

    updateNodeChoice(row, newChoice);
    m_choiceList->item(row)->setText(newChoice.text);
}
// ------------------- 普通节点槽函数 -------------------
void PropertyPanel::updateSlideNavigator()
{
    int total = m_currentNode.slides.size();
    if (total == 0) {
        m_slideLabel->setText(tr("幻灯片: 0/0"));
        m_prevSlideBtn->setEnabled(false);
        m_nextSlideBtn->setEnabled(false);
    } else {
        m_slideLabel->setText(tr("幻灯片: %1 / %2").arg(m_currentSlideIndex+1).arg(total));
        m_prevSlideBtn->setEnabled(m_currentSlideIndex > 0);
        m_nextSlideBtn->setEnabled(m_currentSlideIndex < total-1);
        m_removeSlideBtn->setEnabled(total > 1);
        m_copySlideBtn->setEnabled(true);
    }

    //分支节点禁用幻灯片
    if (m_currentNode.type == NodeType::Choice) {
        m_addSlideBtn->setEnabled(false);
        m_removeSlideBtn->setEnabled(false);
        m_copySlideBtn->setEnabled(false);
        m_prevSlideBtn->setEnabled(false);
        m_nextSlideBtn->setEnabled(false);
        m_slideLabel->setText(tr("分支节点（单幻灯片"));
    }
    else{
        m_addSlideBtn->setEnabled(true);
    }
}

void PropertyPanel::onPrevSlide()
{
    if (m_currentSlideIndex <= 0) return;
    saveCurrentSlideToData();
    saveCurrentNode();          // ★ 关键：将当前幻灯片数据提交到 graph
    m_currentSlideIndex--;
    loadSlideToUI(m_currentSlideIndex);
    updateSlideNavigator();
    emit slideChanged(m_currentSlideIndex);
}

void PropertyPanel::onAddSlide()
{
    if (m_currentNode.type == NodeType::Choice) {
        QMessageBox::information(this, tr("提示"), tr("分支节点不能添加多个幻灯片"));
        return;
    }
    saveCurrentSlideToData();
    saveCurrentNode();

    Slide newSlide;
    // ★ 复制当前幻灯片的文本和背景布局，避免新幻灯片无法显示文本
    const Slide &curSlide = m_currentNode.slides[m_currentSlideIndex];
    newSlide.textRect = curSlide.textRect;
    newSlide.textFontSize = curSlide.textFontSize;
    newSlide.textColor = curSlide.textColor;
    newSlide.textBgImage = curSlide.textBgImage;
    // 注意：背景图和角色列表可以不复制，让用户自己添加
    // 但文本矩形必须复制，否则可能出现在画面外

    m_currentNode.slides.append(newSlide);
    m_graph->updateNode(m_currentNode);
    m_currentSlideIndex = m_currentNode.slides.size() - 1;
    loadSlideToUI(m_currentSlideIndex);
    updateSlideNavigator();
    saveCurrentNode();
    emit slideChanged(m_currentSlideIndex);
}

void PropertyPanel::onRemoveSlide()
{
    if (m_currentNode.type == NodeType::Choice) {
        QMessageBox::information(this, tr("提示"), tr("分支节点不能删除幻灯片"));
        return;
    }
    if (m_currentNode.slides.size() <= 1) {
        QMessageBox::warning(this, tr("警告"), tr("节点至少需要一个幻灯片"));
        return;
    }
    saveCurrentSlideToData();
    saveCurrentNode();          // ★ 保存当前幻灯片
    int removeIndex = m_currentSlideIndex;
    m_currentNode.slides.removeAt(removeIndex);
    m_graph->updateNode(m_currentNode);
    if (removeIndex >= m_currentNode.slides.size())
        m_currentSlideIndex = m_currentNode.slides.size() - 1;
    else
        m_currentSlideIndex = removeIndex;
    if (m_currentSlideIndex < 0) m_currentSlideIndex = 0;
    loadSlideToUI(m_currentSlideIndex);
    updateSlideNavigator();
    saveCurrentNode();          // ★ 保存删除后的节点
    emit slideChanged(m_currentSlideIndex);
}

void PropertyPanel::onCopySlide()
{
    if (m_currentNode.type == NodeType::Choice) {
        QMessageBox::information(this, tr("提示"), tr("分支节点不能复制幻灯片"));
        return;
    }
    if (m_currentSlideIndex < 0) return;
    saveCurrentSlideToData();
    saveCurrentNode();          // ★ 保存当前幻灯片
    Slide copiedSlide = m_currentNode.slides[m_currentSlideIndex];
    m_currentNode.slides.append(copiedSlide);
    m_graph->updateNode(m_currentNode);
    m_currentSlideIndex = m_currentNode.slides.size() - 1;
    loadSlideToUI(m_currentSlideIndex);
    updateSlideNavigator();
    saveCurrentNode();          // ★ 保存复制后的节点
    emit slideChanged(m_currentSlideIndex);
}

void PropertyPanel::onNextSlide()
{
    if (m_currentSlideIndex >= m_currentNode.slides.size() - 1) return;
    saveCurrentSlideToData();
    saveCurrentNode();          // ★ 关键
    m_currentSlideIndex++;
    loadSlideToUI(m_currentSlideIndex);
    updateSlideNavigator();
    emit slideChanged(m_currentSlideIndex);
}
void PropertyPanel::loadSlideToUI(int index)
{
    if (index < 0 || index >= m_currentNode.slides.size()) return;
    const Slide &slide = m_currentNode.slides[index];

    // 阻塞所有可能触发保存的控件
    QSignalBlocker blocker1(m_imagePathEdit);
    QSignalBlocker blocker2(m_textEdit);
    QSignalBlocker blocker3(m_durationSpin);
    QSignalBlocker blocker4(m_textXSpin);
    QSignalBlocker blocker5(m_textYSpin);
    QSignalBlocker blocker6(m_textWSpin);
    QSignalBlocker blocker7(m_textHSpin);
    QSignalBlocker blocker8(m_fontSizeSpin);
    QSignalBlocker blocker9(m_textBgEdit);
    QSignalBlocker blocker10(m_textColorBtn);
    QSignalBlocker blocker11(m_characterList);
    // 新增音频控件的阻塞
    QSignalBlocker blocker12(m_slideAudioEdit);
    QSignalBlocker blocker13(m_bgmActionCombo);
    QSignalBlocker blocker14(m_bgmPathEdit);

    // 设置数值
    m_imagePathEdit->setText(slide.backgroundImage);
    m_textEdit->setPlainText(slide.text);
    m_durationSpin->setValue(slide.duration);

    m_textXSpin->setValue(slide.textRect.x());
    m_textYSpin->setValue(slide.textRect.y());
    m_textWSpin->setValue(slide.textRect.width());
    m_textHSpin->setValue(slide.textRect.height());

    m_fontSizeSpin->setValue(slide.textFontSize);
    m_textBgEdit->setText(slide.textBgImage);
    // 文字颜色
    QPalette pal = m_textColorBtn->palette();
    pal.setColor(QPalette::Button, slide.textColor.isValid() ? slide.textColor : Qt::gray);
    m_textColorBtn->setPalette(pal);

    // 角色列表（不清除也行，但最好刷新）
    m_characterList->clear();
    for (const auto &ch : slide.characters) {
        m_characterList->addItem(ch.name.isEmpty() ? QFileInfo(ch.imagePath).baseName() : ch.name);
    }
    m_characterList->clearSelection();

    // 音频控件
    m_slideAudioEdit->setText(slide.slideAudioPath);
    int actionIdx = m_bgmActionCombo->findData(static_cast<int>(slide.bgmAction));
    if (actionIdx >= 0) m_bgmActionCombo->setCurrentIndex(actionIdx);
    m_bgmPathEdit->setText(slide.bgmPath);
    bool enablePath = (slide.bgmAction == BgmAction::Switch);
    m_bgmPathEdit->setEnabled(enablePath);
    if (m_browseBgmBtn) m_browseBgmBtn->setEnabled(enablePath);
    // 其他 UI 状态
    enableCharacterDetailControls(false);
}
void PropertyPanel::saveCurrentSlideToData()
{
    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= m_currentNode.slides.size()) return;
    Slide &slide = m_currentNode.slides[m_currentSlideIndex];
    slide.backgroundImage = m_imagePathEdit->text();
    slide.text = m_textEdit->toPlainText();
    slide.duration = m_durationSpin->value();

    // 角色列表的保存依赖于角色管理函数直接操作 slide.characters，这里无需额外处理
}


void PropertyPanel::onNodeNameChanged()
{
    if (m_loading) return;
    if (m_currentNode.id.isEmpty()) return;
    m_currentNode.name = m_nodeNameEdit->text();
    saveCurrentNode();
    emit graphDataChanged();
}

void PropertyPanel::onBackgroundImageChanged()
{
    if (m_loading || m_currentNode.id.isEmpty() || m_currentSlideIndex < 0) return;
    QString relPath = selectMaterialFromLibrary({"jpg","png","jpeg"}, tr("选择背景图片"));
    if (relPath.isEmpty()) return;
    if (m_currentNode.slides.isEmpty())
        m_currentNode.slides.append(Slide());
    m_currentNode.slides[m_currentSlideIndex].backgroundImage = relPath;
    m_imagePathEdit->setText(relPath);
    saveCurrentNode();
}

void PropertyPanel::onTextChanged()
{
    if (m_loading) return;
    if (m_currentNode.id.isEmpty()) return;
    if (m_currentSlideIndex < 0) return;
    if (m_currentNode.slides.isEmpty())
        m_currentNode.slides.append(Slide());
    m_currentNode.slides[m_currentSlideIndex].text = m_textEdit->toPlainText();
    saveCurrentNode();
}

//--------------音频-------------


void PropertyPanel::onSlideAudioChanged()
{
    if (m_loading) return;
    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= m_currentNode.slides.size()) return;
    m_currentNode.slides[m_currentSlideIndex].slideAudioPath = m_slideAudioEdit->text();
    saveCurrentNode();
}

void PropertyPanel::onBgmActionChanged()
{
    if (m_loading) return;
    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= m_currentNode.slides.size()) return;

    int action = m_bgmActionCombo->currentData().toInt();
    m_currentNode.slides[m_currentSlideIndex].bgmAction = static_cast<BgmAction>(action);

    bool enablePath = (action == static_cast<int>(BgmAction::Switch));
    m_bgmPathEdit->setEnabled(enablePath);
    // ★ 控制浏览按钮的启用状态
    if (m_browseBgmBtn) m_browseBgmBtn->setEnabled(enablePath);

    if (!enablePath) {
        m_bgmPathEdit->clear();
        m_currentNode.slides[m_currentSlideIndex].bgmPath.clear();
    }
    saveCurrentNode();
}
void PropertyPanel::onBgmPathChanged()
{
    if (m_loading) return;
    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= m_currentNode.slides.size()) return;
    m_currentNode.slides[m_currentSlideIndex].bgmPath = m_bgmPathEdit->text();
    saveCurrentNode();
}

void PropertyPanel::onDurationChanged()
{
    if (m_loading) return;
    if (m_currentNode.id.isEmpty()) return;
    if (m_currentSlideIndex < 0) return;
    if (m_currentNode.slides.isEmpty())
        m_currentNode.slides.append(Slide());
    m_currentNode.slides[m_currentSlideIndex].duration = m_durationSpin->value();
    saveCurrentNode();
}

void PropertyPanel::onAddCharacter()
{
    if (m_loading || m_currentNode.id.isEmpty() || m_currentSlideIndex < 0) return;
    QString relPath = selectMaterialFromLibrary({"jpg","png","jpeg"}, tr("选择角色图片"));
    if (relPath.isEmpty()) return;
    if (m_currentNode.slides.isEmpty())
        m_currentNode.slides.append(Slide());

    Slide::Character ch;
    ch.imagePath = relPath;
    ch.position = QPointF(500, 200);
    ch.scale = 1.0;
    // 名字默认取文件名（不含扩展名）
    QFileInfo info(relPath);
    ch.name = info.baseName();
    ch.nameFontSize = 20;
    // 名字文本框默认位于角色图片下方居中
    ch.nameRect = QRectF(ch.position.x() - 100, ch.position.y() + 100, 200, 40);
    ch.nameColor = Qt::white;
    ch.nameBgImage = "";

    m_currentNode.slides[m_currentSlideIndex].characters.append(ch);
    m_characterList->addItem(ch.name);   // 显示角色名字而非图片名
    saveCurrentNode();
}

void PropertyPanel::onRemoveCharacter()
{
    if (m_loading) return;
    if (m_currentNode.id.isEmpty()) return;
    if (m_currentSlideIndex < 0) return;
    int row = m_characterList->currentRow();
    if (row >= 0 && !m_currentNode.slides.isEmpty()) {
        m_currentNode.slides[m_currentSlideIndex].characters.removeAt(row);
        delete m_characterList->takeItem(row);
        saveCurrentNode();
    }
}


void PropertyPanel::onCharacterSelected(int row)
{
    if (m_loading) return;
    if (row >= 0 && !m_currentNode.slides.isEmpty() &&
        row < m_currentNode.slides[m_currentSlideIndex].characters.size()) {
        auto &ch = m_currentNode.slides[m_currentSlideIndex].characters[row];
        // 名字内容
        m_charNameEdit->blockSignals(true);
        m_charNameEdit->setText(ch.name);
        m_charNameEdit->setEnabled(true);
        m_charNameEdit->blockSignals(false);

        // 字体大小
        m_charNameFontSpin->blockSignals(true);
        m_charNameFontSpin->setValue(ch.nameFontSize);
        m_charNameFontSpin->setEnabled(true);
        m_charNameFontSpin->blockSignals(false);

        // 颜色
        QPalette pal = m_charNameColorBtn->palette();
        pal.setColor(QPalette::Button, ch.nameColor);
        m_charNameColorBtn->setPalette(pal);
        m_charNameColorBtn->setEnabled(true);

        // 背景图
        m_charNameBgEdit->setText(ch.nameBgImage);
        m_charNameBgEdit->setEnabled(true);
        m_browseCharNameBgBtn->setEnabled(true);

        // 位置/大小
        m_charNameXSpin->blockSignals(true);
        m_charNameYSpin->blockSignals(true);
        m_charNameWSpin->blockSignals(true);
        m_charNameHSpin->blockSignals(true);
        m_charNameXSpin->setValue(ch.nameRect.x());
        m_charNameYSpin->setValue(ch.nameRect.y());
        m_charNameWSpin->setValue(ch.nameRect.width());
        m_charNameHSpin->setValue(ch.nameRect.height());
        m_charNameXSpin->setEnabled(true);
        m_charNameYSpin->setEnabled(true);
        m_charNameWSpin->setEnabled(true);
        m_charNameHSpin->setEnabled(true);
        m_charNameXSpin->blockSignals(false);
        m_charNameYSpin->blockSignals(false);
        m_charNameWSpin->blockSignals(false);
        m_charNameHSpin->blockSignals(false);

        // 角色图片缩放
        m_charScaleSpin->blockSignals(true);
        m_charScaleSpin->setValue(ch.scale);
        m_charScaleSpin->setEnabled(true);
        m_charScaleSpin->blockSignals(false);
    } else {
        m_charNameEdit->setEnabled(false);
        m_charNameFontSpin->setEnabled(false);
        m_charNameColorBtn->setEnabled(false);
        m_charNameBgEdit->setEnabled(false);
        m_browseCharNameBgBtn->setEnabled(false);
        m_charNameXSpin->setEnabled(false);
        m_charNameYSpin->setEnabled(false);
        m_charNameWSpin->setEnabled(false);
        m_charNameHSpin->setEnabled(false);
        m_charScaleSpin->setEnabled(false);
    }
}

void PropertyPanel::onCharacterScaleChanged(double scale)
{
    if (m_loading) return;
    int row = m_characterList->currentRow();
    if (row >= 0 && !m_currentNode.slides.isEmpty() &&
        row < m_currentNode.slides[m_currentSlideIndex].characters.size()) {
        m_currentNode.slides[m_currentSlideIndex].characters[row].scale = scale;
        saveCurrentNode();
        emit graphDataChanged();  // 刷新画布
    }
}

// 角色名字内容改变
void PropertyPanel::onCharacterNameChanged()
{
    if (m_loading) return;
    int row = m_characterList->currentRow();
    if (row < 0 || m_currentNode.slides.isEmpty()) return;
    auto &ch = m_currentNode.slides[m_currentSlideIndex].characters[row];
    QString newName = m_charNameEdit->text();
    if (ch.name == newName) return;
    ch.name = newName;
    m_characterList->currentItem()->setText(newName);
    saveCurrentNode();
    emit graphDataChanged();
}

// 名字字体大小改变
void PropertyPanel::onCharacterNameFontChanged()
{
    if (m_loading) return;
    int row = m_characterList->currentRow();
    if (row < 0 || m_currentNode.slides.isEmpty()) return;
    auto &ch = m_currentNode.slides[m_currentSlideIndex].characters[row];
    ch.nameFontSize = m_charNameFontSpin->value();
    saveCurrentNode();
    emit graphDataChanged();
}
void PropertyPanel::onCharacterContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_characterList->itemAt(pos);
    if (!item) return;
    QMenu menu;
    menu.addAction(tr("删除角色"), this, &PropertyPanel::onRemoveCharacter);
    menu.exec(m_characterList->mapToGlobal(pos));
}

// 名字颜色改变
void PropertyPanel::onCharacterNameColorChanged()
{
    if (m_loading) return;
    int row = m_characterList->currentRow();
    if (row < 0 || m_currentNode.slides.isEmpty()) return;
    QColor color = QColorDialog::getColor(m_currentNode.slides[m_currentSlideIndex].characters[row].nameColor,
                                          this, tr("选择角色名字颜色"));
    if (!color.isValid()) return;
    auto &ch = m_currentNode.slides[m_currentSlideIndex].characters[row];
    ch.nameColor = color;
    QPalette pal = m_charNameColorBtn->palette();
    pal.setColor(QPalette::Button, color);
    m_charNameColorBtn->setPalette(pal);
    saveCurrentNode();
    emit graphDataChanged();
}

// 名字背景图片浏览
void PropertyPanel::onBrowseCharNameBg()
{
    if (m_loading) return;
    int row = m_characterList->currentRow();
    if (row < 0 || m_currentNode.slides.isEmpty()) return;
    QString relPath = selectMaterialFromLibrary({"jpg","png","jpeg"}, tr("选择名字背景图"));
    if (relPath.isEmpty()) return;
    auto &ch = m_currentNode.slides[m_currentSlideIndex].characters[row];
    ch.nameBgImage = relPath;
    m_charNameBgEdit->setText(relPath);
    saveCurrentNode();
    emit graphDataChanged();
}

// 名字矩形改变（微调）
void PropertyPanel::onCharacterNameRectChanged()
{
    if (m_loading) return;
    int row = m_characterList->currentRow();
    if (row < 0 || m_currentNode.slides.isEmpty()) return;
    auto &ch = m_currentNode.slides[m_currentSlideIndex].characters[row];
    ch.nameRect = QRectF(m_charNameXSpin->value(), m_charNameYSpin->value(),
                         m_charNameWSpin->value(), m_charNameHSpin->value());
    saveCurrentNode();
    emit graphDataChanged();   // 触发画布刷新
}

void PropertyPanel::onTextRectChanged()
{
    if (m_loading || m_currentNode.id.isEmpty() || m_currentSlideIndex < 0) return;
    QRectF rect(m_textXSpin->value(), m_textYSpin->value(),
                m_textWSpin->value(), m_textHSpin->value());
    m_currentNode.slides[m_currentSlideIndex].textRect = rect;
    saveCurrentNode();
    emit graphDataChanged();
}

void PropertyPanel::onTextFontSizeChanged()
{
    if (m_loading || m_currentNode.id.isEmpty() || m_currentSlideIndex < 0) return;
    m_currentNode.slides[m_currentSlideIndex].textFontSize = m_fontSizeSpin->value();
    saveCurrentNode();
    emit graphDataChanged();
}

void PropertyPanel::onTextBgImageChanged()
{
    if (m_loading || m_currentNode.id.isEmpty() || m_currentSlideIndex < 0) return;
    m_currentNode.slides[m_currentSlideIndex].textBgImage = m_textBgEdit->text();
    saveCurrentNode();
    emit graphDataChanged();
}

void PropertyPanel::onTextColorChanged()
{
    if (m_loading || m_currentNode.id.isEmpty() || m_currentSlideIndex < 0) return;
    QColor currentColor = m_currentNode.slides[m_currentSlideIndex].textColor;
    if (!currentColor.isValid()) currentColor = QColor(128, 128, 128);
    QColor color = QColorDialog::getColor(currentColor, this, tr("选择文字颜色"));
    m_selectedTextColor = color;
    // 更新按钮颜色
    QPalette pal = m_textColorBtn->palette();
    pal.setColor(QPalette::Button, color);
    m_textColorBtn->setPalette(pal);

    // 保存到当前幻灯片的文字颜色
    m_currentNode.slides[m_currentSlideIndex].textColor = color;
    saveCurrentNode();
    emit graphDataChanged();
}


// ------------------- 分支节点槽函数 -------------------

// ==================== 分支选项辅助函数 ====================

void PropertyPanel::refreshTargetNodeCombo()
{
    QSignalBlocker blocker(m_targetNodeCombo);
    m_targetNodeCombo->clear();
    QList<NodeData> allNodes = m_graph->getAllNodes();
    for (const NodeData &node : allNodes) {
        if (node.id == m_currentNode.id) continue;
        QString display = QString("%1 (%2)").arg(node.name, node.id.left(8));
        m_targetNodeCombo->addItem(display, node.id);
    }
}
void PropertyPanel::addChoiceAndRefresh(const NodeData::Choice &newChoice)
{
    m_currentNode.choices.append(newChoice);
    m_graph->updateNode(m_currentNode);
    m_choiceList->addItem(newChoice.text);
    // 自动选中新添加的选项
    int newRow = m_choiceList->count() - 1;
    m_choiceList->setCurrentRow(newRow);
    // 重新同步工作副本
    NodeData fresh = m_graph->getNode(m_currentNode.id);
    if (!fresh.id.isEmpty()) {
        m_currentNode = fresh;
    }
    emit graphDataChanged();
    emit dataChanged(m_currentNode.id);
}

void PropertyPanel::onAddChoice()
{
    if (m_loading) return;
    if (m_currentNode.id.isEmpty() || m_currentNode.type != NodeType::Choice) return;

    QMenu menu;
    QAction *actNormal = menu.addAction(tr("新建普通节点并链接"));
    QAction *actChoice = menu.addAction(tr("新建分支节点并链接"));
    QAction *selected = menu.exec(QCursor::pos());
    if (!selected) return;

    bool isChoiceTarget = (selected == actChoice);
    NodeType targetType = isChoiceTarget ? NodeType::Choice : NodeType::Normal;

    // 创建新节点
    NodeData newSubNode;
    newSubNode.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newSubNode.type = targetType;
    newSubNode.name = isChoiceTarget ? tr("新分支节点") : tr("新剧情节点");
    m_graph->addNode(newSubNode, m_currentNode.id);   // 建立父子关系

    // 创建新选项
    NodeData::Choice newChoice;
    newChoice.text = tr("选项 %1").arg(m_currentNode.choices.size() + 1);
    newChoice.targetNodeId = newSubNode.id;
    newChoice.textFontSize = 20;
    newChoice.textColor = Qt::white;
    newChoice.buttonSize = QSizeF(400, 100);
    // 自动计算位置，避免重叠
    newChoice.buttonPos = QPointF(100, 100 + m_currentNode.choices.size() * 120);

    // 添加到当前节点
    m_currentNode.choices.append(newChoice);
    // 更新到 graph
    NodeData updatedParent = m_graph->getNode(m_currentNode.id);
    updatedParent.choices = m_currentNode.choices;
    m_graph->updateNode(updatedParent);

    // 刷新 UI
    m_choiceList->addItem(newChoice.text);
    int newRow = m_choiceList->count() - 1;
    m_choiceList->setCurrentRow(newRow);

    // 重新获取当前节点数据（确保 m_currentNode 与 graph 同步）
    m_currentNode = m_graph->getNode(m_currentNode.id);

    emit graphDataChanged();
    emit dataChanged(m_currentNode.id);
}

void PropertyPanel::onLinkToExistingNode()
{
    if (m_loading) {
        QTimer::singleShot(50, this, &PropertyPanel::onLinkToExistingNode);
        return;
    }
    if (m_currentNode.id.isEmpty() || m_currentNode.type != NodeType::Choice) return;

    refreshTargetNodeCombo();

    QDialog dlg(this);
    dlg.setWindowTitle(tr("选择目标节点"));
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QComboBox *combo = new QComboBox(&dlg);
    QList<NodeData> allNodes = m_graph->getAllNodes();
    for (const NodeData &node : allNodes) {
        if (node.id == m_currentNode.id) continue;
        combo->addItem(QString("%1 (%2)").arg(node.name, node.id.left(8)), node.id);
    }
    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton(tr("确定"));
    QPushButton *cancelBtn = new QPushButton(tr("取消"));
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addWidget(combo);
    layout->addLayout(btnLayout);
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString targetId = combo->currentData().toString();
        if (targetId.isEmpty()) return;

        NodeData::Choice newChoice;
        NodeData targetNode = m_graph->getNode(targetId);
        newChoice.text = tr("跳转到 %1").arg(targetNode.name.isEmpty() ? targetId.left(8) : targetNode.name);
        newChoice.targetNodeId = targetId;
        newChoice.textFontSize = 20;
        newChoice.textColor = Qt::white;
        newChoice.buttonSize = QSizeF(400, 100);
        newChoice.buttonPos = QPointF(100, 100 + m_currentNode.choices.size() * 120);

        addChoiceAndRefresh(newChoice);
    }
}

void PropertyPanel::onRemoveChoice()
{
    if (m_loading) return;
    int row = m_choiceList->currentRow();
    if (row < 0 || row >= m_currentNode.choices.size()) return;

    // 仅从当前节点中移除选项
    m_currentNode.choices.removeAt(row);
    delete m_choiceList->takeItem(row);

    // 更新到 graph
    NodeData nodeCopy = m_graph->getNode(m_currentNode.id);
    if (!nodeCopy.id.isEmpty()) {
        nodeCopy.choices = m_currentNode.choices;
        m_graph->updateNode(nodeCopy);
    }

    // 刷新 UI
    if (m_choiceList->count() > 0) {
        m_choiceList->setCurrentRow(0);
    } else {
        // 清空编辑区域
        m_choiceTextEdit->clear();
        m_choiceImageEdit->clear();
        m_targetNodeCombo->clear();
    }
    emit graphDataChanged();
}

void PropertyPanel::onChoiceSelected(int row)
{
    if (m_loading) return;
    if (row < 0 || row >= m_currentNode.choices.size()) return;

    // 加载新选项到 UI（使用信号阻塞器）
    loadChoiceToUI(row);

    // 刷新下拉框并设置正确的目标节点
    refreshTargetNodeCombo();
    QString targetId = m_currentNode.choices[row].targetNodeId;
    int idx = m_targetNodeCombo->findData(targetId);
    if (idx >= 0) {
        QSignalBlocker comboBlocker(m_targetNodeCombo);
        m_targetNodeCombo->setCurrentIndex(idx);
    } else {
        // 目标节点无效时，清空下拉框显示（但不要触发保存）
        QSignalBlocker comboBlocker(m_targetNodeCombo);
        m_targetNodeCombo->setCurrentIndex(-1);
    }
}

void PropertyPanel::onChoiceFontSizeChanged()
{
    if (m_loading) return;
    int row = m_choiceList->currentRow();
    if (row < 0 || row >= m_currentNode.choices.size()) return;
    m_currentNode.choices[row].textFontSize = m_choiceFontSizeSpin->value();
    saveCurrentNode();               // 保存到 StoryGraph
    emit graphDataChanged();         // 触发画布刷新
}

void PropertyPanel::onChoiceColorChanged()
{
    if (m_loading) return;
    int row = m_choiceList->currentRow();
    if (row < 0 || row >= m_currentNode.choices.size()) return;
    QColor color = QColorDialog::getColor(m_currentNode.choices[row].textColor,
                                          this, tr("选择选项文字颜色"));
    if (!color.isValid()) return;
    m_currentNode.choices[row].textColor = color;
    // 更新按钮颜色预览
    QPalette pal = m_choiceColorBtn->palette();
    pal.setColor(QPalette::Button, color);
    m_choiceColorBtn->setPalette(pal);
    saveCurrentNode();
    emit graphDataChanged();
}

void PropertyPanel::onChoiceSizeChanged()
{
    if (m_loading) return;
    int row = m_choiceList->currentRow();
    if (row >= 0 && row < m_currentNode.choices.size()) {
        double w = m_choiceWidthSpin->value();
        double h = m_choiceHeightSpin->value();
        m_currentNode.choices[row].buttonSize = QSizeF(w, h);
        saveCurrentNode();
        emit graphDataChanged();  // 刷新画布
    }
}
//--------图片素材库---------
QString PropertyPanel::selectMaterialFromLibrary(const QStringList &suffixes, const QString &title)
{
    if (!m_materialWidget) return QString();
    QStringList files = m_materialWidget->getMaterialPathsBySuffix(suffixes);
    if (files.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("素材库中没有符合要求的文件，请先导入。"));
        return QString();
    }
    QMenu menu;
    for (const QString &file : files) {
        QFileInfo info(file);
        menu.addAction(info.fileName(), [=]() { /* 暂不处理，需要在循环外记录选择 */ });
    }
    // 由于lambda捕获问题，改用QSignalMapper或简单方式：使用QInputDialog 带下拉列表
    bool ok;
    QString selected = QInputDialog::getItem(this, title, tr("选择素材:"), files, 0, false, &ok);
    if (ok && !selected.isEmpty()) {
        return selected;  // 返回相对路径，如 "resources/bg.jpg"
    }
    return QString();
}

QString PropertyPanel::getCurrentProjectPath() const
{
    return ProjectManager::instance().currentProjectPath();
}

void PropertyPanel::loadChoiceToUI(int row)
{
    if (row < 0 || row >= m_currentNode.choices.size()) return;
    const auto &choice = m_currentNode.choices[row];

    QSignalBlocker textBlocker(m_choiceTextEdit);
    QSignalBlocker imageBlocker(m_choiceImageEdit);
    QSignalBlocker fontSizeBlocker(m_choiceFontSizeSpin);
    QSignalBlocker colorBlocker(m_choiceColorBtn);
    QSignalBlocker widthBlocker(m_choiceWidthSpin);
    QSignalBlocker heightBlocker(m_choiceHeightSpin);

    m_choiceTextEdit->setText(choice.text);
    m_choiceImageEdit->setText(choice.imagePath);
    m_choiceFontSizeSpin->setValue(choice.textFontSize);
    m_selectedChoiceColor = choice.textColor;
    QPalette pal = m_choiceColorBtn->palette();
    pal.setColor(QPalette::Button, choice.textColor);
    m_choiceColorBtn->setPalette(pal);
    m_choiceWidthSpin->setValue(choice.buttonSize.width());
    m_choiceHeightSpin->setValue(choice.buttonSize.height());
}

void PropertyPanel::refreshCurrentSlideUI()
{
    if (m_loading) return;
    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= m_currentNode.slides.size()) return;
    const Slide &slide = m_currentNode.slides[m_currentSlideIndex];

    // 更新文本框矩形 (使用信号阻塞器)
    {
        QSignalBlocker tx(m_textXSpin), ty(m_textYSpin), tw(m_textWSpin), th(m_textHSpin);
        m_textXSpin->setValue(slide.textRect.x());
        m_textYSpin->setValue(slide.textRect.y());
        m_textWSpin->setValue(slide.textRect.width());
        m_textHSpin->setValue(slide.textRect.height());
    }

    // 更新文字颜色按钮
    QPalette pal = m_textColorBtn->palette();
    pal.setColor(QPalette::Button, slide.textColor);
    m_textColorBtn->setPalette(pal);

    // 如果当前有选中的角色，更新其名字矩形数值
    int selectedRow = m_characterList->currentRow();
    if (selectedRow >= 0 && selectedRow < slide.characters.size()) {
        const auto &ch = slide.characters[selectedRow];
        QSignalBlocker cx(m_charNameXSpin), cy(m_charNameYSpin), cw(m_charNameWSpin), chh(m_charNameHSpin);
        m_charNameXSpin->setValue(ch.nameRect.x());
        m_charNameYSpin->setValue(ch.nameRect.y());
        m_charNameWSpin->setValue(ch.nameRect.width());
        m_charNameHSpin->setValue(ch.nameRect.height());
        // 更新缩放、字体等？通常不需要，因为这些控件不会在外部直接修改。
    }
}

void PropertyPanel::updateNodeNameUI()
{
    QSignalBlocker blocker(m_nodeNameEdit);
    m_nodeNameEdit->setText(m_currentNode.name);
}


void PropertyPanel::updateTextRect(const QRectF &rect)
{
    if (m_loading) return;
    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= m_currentNode.slides.size()) return;
    m_currentNode.slides[m_currentSlideIndex].textRect = rect;

    // 同步 UI 中的 spinbox
    QSignalBlocker tx(m_textXSpin), ty(m_textYSpin), tw(m_textWSpin), th(m_textHSpin);
    m_textXSpin->setValue(rect.x());
    m_textYSpin->setValue(rect.y());
    m_textWSpin->setValue(rect.width());
    m_textHSpin->setValue(rect.height());
}

void PropertyPanel::updateUIFromCurrentSlide()
{
    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= m_currentNode.slides.size()) return;
    const Slide &slide = m_currentNode.slides[m_currentSlideIndex];

    QSignalBlocker blocker1(m_imagePathEdit);
    QSignalBlocker blocker2(m_textEdit);
    QSignalBlocker blocker3(m_durationSpin);
    QSignalBlocker blocker4(m_textXSpin);
    QSignalBlocker blocker5(m_textYSpin);
    QSignalBlocker blocker6(m_textWSpin);
    QSignalBlocker blocker7(m_textHSpin);
    QSignalBlocker blocker8(m_fontSizeSpin);
    QSignalBlocker blocker9(m_textBgEdit);
    QSignalBlocker blocker10(m_textColorBtn);
    QSignalBlocker blocker11(m_characterList);
    QSignalBlocker blocker12(m_slideAudioEdit);
    QSignalBlocker blocker13(m_bgmActionCombo);
    QSignalBlocker blocker14(m_bgmPathEdit);

    m_imagePathEdit->setText(slide.backgroundImage);
    m_textEdit->setPlainText(slide.text);
    m_durationSpin->setValue(slide.duration);

    m_textXSpin->setValue(slide.textRect.x());
    m_textYSpin->setValue(slide.textRect.y());
    m_textWSpin->setValue(slide.textRect.width());
    m_textHSpin->setValue(slide.textRect.height());

    m_fontSizeSpin->setValue(slide.textFontSize);
    m_textBgEdit->setText(slide.textBgImage);
    QPalette pal = m_textColorBtn->palette();
    pal.setColor(QPalette::Button, slide.textColor.isValid() ? slide.textColor : Qt::gray);
    m_textColorBtn->setPalette(pal);

    m_characterList->clear();
    for (const auto &ch : slide.characters) {
        m_characterList->addItem(ch.name.isEmpty() ? QFileInfo(ch.imagePath).baseName() : ch.name);
    }

    m_slideAudioEdit->setText(slide.slideAudioPath);
    int actionIdx = m_bgmActionCombo->findData(static_cast<int>(slide.bgmAction));
    if (actionIdx >= 0) m_bgmActionCombo->setCurrentIndex(actionIdx);
    m_bgmPathEdit->setText(slide.bgmPath);
    bool enablePath = (slide.bgmAction == BgmAction::Switch);
    m_bgmPathEdit->setEnabled(enablePath);
    if (m_browseBgmBtn) m_browseBgmBtn->setEnabled(enablePath);
}
void PropertyPanel::updateChoicePageVisibility()
{
    int tabIndex = m_tabWidget->indexOf(m_choicePage);
    bool isChoice = (m_currentNode.type == NodeType::Choice);
    if (isChoice && tabIndex == -1) {
        m_tabWidget->addTab(m_choicePage, tr("分支选项"));
    } else if (!isChoice && tabIndex != -1) {
        m_tabWidget->removeTab(tabIndex);
    }

    // 同时更新幻灯片导航按钮的启用状态
    m_prevSlideBtn->setEnabled(!isChoice);
    m_nextSlideBtn->setEnabled(!isChoice);
    m_addSlideBtn->setEnabled(!isChoice);
    m_removeSlideBtn->setEnabled(!isChoice);
    m_copySlideBtn->setEnabled(!isChoice);
    if (isChoice) {
        m_slideLabel->setText(tr("分支节点（单幻灯片）"));
    }
}

void PropertyPanel::enableCharacterDetailControls(bool enable)
{
    m_charNameEdit->setEnabled(enable);
    m_charNameFontSpin->setEnabled(enable);
    m_charNameColorBtn->setEnabled(enable);
    m_charNameBgEdit->setEnabled(enable);
    m_browseCharNameBgBtn->setEnabled(enable);
    m_charNameXSpin->setEnabled(enable);
    m_charNameYSpin->setEnabled(enable);
    m_charNameWSpin->setEnabled(enable);
    m_charNameHSpin->setEnabled(enable);
    m_charScaleSpin->setEnabled(enable);
}
void PropertyPanel::onNodeDataChanged(const QString &nodeId)
{
    if (m_loading) return;
    if (nodeId != m_currentNode.id) return;   // 只关心当前节点

    // 重新从 graph 加载当前节点数据
    NodeData fresh = m_graph->getNode(nodeId);
    if (fresh.id.isEmpty()) return;

    // 保留当前的幻灯片索引（因为外部修改可能只是角色位置，不需要改变索引）
    int oldSlideIndex = m_currentSlideIndex;
    m_currentNode = fresh;

    // 修正索引边界
    if (oldSlideIndex >= m_currentNode.slides.size())
        oldSlideIndex = m_currentNode.slides.isEmpty() ? 0 : m_currentNode.slides.size() - 1;
    if (oldSlideIndex < 0) oldSlideIndex = 0;
    m_currentSlideIndex = oldSlideIndex;

    // 刷新 UI 显示（确保数值同步）
    refreshCurrentSlideUI();
    updateNodeNameUI();

    // 如果节点类型变化，更新分支选项页的显示
    updateChoicePageVisibility();
}

