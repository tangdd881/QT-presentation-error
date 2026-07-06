#include "PlayerMainWindow.h"
#include "PlayCanvas.h"
#include <QTimer>
#include <QMediaPlayer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QMessageBox>
#include <QStatusBar>
#include <QDebug>
#include <QAudioOutput>

PlayerMainWindow::PlayerMainWindow(const QString &projectPath, QWidget *parent)
    : QMainWindow(parent)
    , m_projectPath(projectPath)
    , m_graph(nullptr)
    , m_canvas(nullptr)
    , m_currentSlideIndex(0)
    , m_audioPlayer(nullptr)
    , m_bgmPlayer(nullptr)
    , m_slideAudioPlayer(nullptr)
    , m_isPlaying(false)
    , m_bgmPlaying(false)
    , m_bgmPausePosition(0)
{
    setWindowTitle(tr("互动剧情播放器"));
    resize(1280, 720);
    setMinimumSize(800, 600);

    m_graph = new StoryGraph(this);
    m_canvas = new PlayCanvas(m_graph, m_projectPath, this);
    setCentralWidget(m_canvas);

    connect(m_canvas, &PlayCanvas::choiceSelected, this, &PlayerMainWindow::onChoiceSelected);
    connect(m_canvas, &PlayCanvas::canvasClicked, this, &PlayerMainWindow::onCanvasClicked);
    connect(m_canvas, &PlayCanvas::historyBackRequested, this, &PlayerMainWindow::onHistoryBackRequested);

    m_audioPlayer = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this);
    audioOutput->setVolume(1.0);
    m_audioPlayer->setAudioOutput(audioOutput);
    connect(m_audioPlayer, &QMediaPlayer::errorOccurred, this, &PlayerMainWindow::onMediaError);

    m_bgmPlayer = new QMediaPlayer(this);
    QAudioOutput *bgmOutput = new QAudioOutput(this);
    bgmOutput->setVolume(1.0);
    m_bgmPlayer->setAudioOutput(bgmOutput);
    connect(m_bgmPlayer, &QMediaPlayer::errorOccurred, this, &PlayerMainWindow::onMediaError);

    m_slideAudioPlayer = new QMediaPlayer(this);
    QAudioOutput *slideOutput = new QAudioOutput(this);
    slideOutput->setVolume(1.0);
    m_slideAudioPlayer->setAudioOutput(slideOutput);
    connect(m_slideAudioPlayer, &QMediaPlayer::errorOccurred, this, &PlayerMainWindow::onMediaError);

    loadStory();
    statusBar()->showMessage(tr("已加载工程: %1").arg(m_projectPath));
}

PlayerMainWindow::~PlayerMainWindow()
{
    stopCurrentAudio();
    if (m_bgmPlayer) m_bgmPlayer->stop();
    if (m_slideAudioPlayer) m_slideAudioPlayer->stop();
}

void PlayerMainWindow::loadStory()
{
    QString storyPath = m_projectPath + "/story.json";
    QFile file(storyPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("错误"), tr("无法读取剧情文件:\n%1").arg(storyPath));
        return;
    }
    QByteArray data = file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        QMessageBox::critical(this, tr("错误"), tr("剧情文件解析失败:\n%1").arg(err.errorString()));
        return;
    }
    if (!m_graph->fromJson(doc.object())) {
        QMessageBox::critical(this, tr("错误"), tr("剧情数据加载失败"));
        return;
    }
    QString rootId = m_graph->getRootId();
    if (rootId.isEmpty()) {
        QMessageBox::critical(this, tr("错误"), tr("工程中没有根节点"));
        return;
    }
    setCurrentNode(rootId, 0, false); // 初始进入不记录历史
}

void PlayerMainWindow::setCurrentNode(const QString &nodeId, int slideIndex, bool recordHistory)
{
    if (m_currentNodeId == nodeId && m_currentSlideIndex == slideIndex && m_isPlaying)
        return;

    // 记录当前状态到历史栈（仅当 recordHistory 为 true 且状态发生改变）
    if (recordHistory && !m_currentNodeId.isEmpty()) {
        pushToHistory(m_currentNodeId, m_currentSlideIndex);
    }

    // 停止旧音频
    stopCurrentAudio();
    stopSlideAudio();

    m_currentNodeId = nodeId;
    m_currentSlideIndex = slideIndex;

    NodeData node = m_graph->getNode(nodeId);
    if (node.id.isEmpty() || node.slides.isEmpty()) {
        qWarning() << "节点无效或无幻灯片:" << nodeId;
        return;
    }

    if (m_currentSlideIndex < 0 || m_currentSlideIndex >= node.slides.size())
        m_currentSlideIndex = 0;

    m_canvas->setCurrentNode(nodeId, m_currentSlideIndex);
    m_isPlaying = true;

    playCurrentAudio();
    applySlideAudio(m_currentSlideIndex);
    startCurrentNode();
    updateWindowTitle();
}

void PlayerMainWindow::startCurrentNode()
{
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty() || node.slides.isEmpty()) return;

    if (m_currentSlideIndex >= node.slides.size()) {
        nextSlide();
        return;
    }

    if (node.type == NodeType::Choice) {
        m_canvas->setCurrentSlideIndex(m_currentSlideIndex);
        return;
    }
    m_canvas->setCurrentSlideIndex(m_currentSlideIndex);
}

void PlayerMainWindow::nextSlide()
{
    if (!m_isPlaying) return;

    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty()) return;
    if (node.type == NodeType::Choice) return;

    int newIndex = m_currentSlideIndex + 1;
    if (newIndex < node.slides.size()) {
        // 切换到下一张幻灯片，记录当前状态到历史栈
        setCurrentNode(m_currentNodeId, newIndex, true);
    } else {
        // 播放完毕，跳转子节点
        if (node.childIds.isEmpty()) {
            qWarning() << "普通节点没有子节点，无法跳转:" << node.name;
            return;
        }
        QString childId = node.childIds.first();
        setCurrentNode(childId, 0, true);
    }
}

void PlayerMainWindow::onCanvasClicked()
{
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty()) return;
    if (node.type == NodeType::Choice) return;
    if (!m_isPlaying) return;
    nextSlide();
}

void PlayerMainWindow::onChoiceSelected(const QString &targetNodeId)
{
    if (!m_isPlaying) return;
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty() || node.type != NodeType::Choice) return;
    if (targetNodeId.isEmpty()) {
        qWarning() << "选项的目标节点为空";
        return;
    }
    NodeData targetNode = m_graph->getNode(targetNodeId);
    if (targetNode.id.isEmpty()) {
        qWarning() << "目标节点不存在:" << targetNodeId;
        return;
    }
    setCurrentNode(targetNodeId, 0, true);
}

// -------------------- 音频 --------------------
void PlayerMainWindow::stopCurrentAudio()
{
    if (m_audioPlayer && !m_audioPlayer->source().isEmpty()) {
        m_audioPlayer->stop();
        m_audioPlayer->setSource(QUrl());
    }
}

void PlayerMainWindow::playCurrentAudio()
{
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.audioPath.isEmpty()) return;
    QString fullPath = m_projectPath + "/" + node.audioPath;
    if (!QFile::exists(fullPath)) {
        qWarning() << "节点音频文件不存在:" << fullPath;
        return;
    }
    stopCurrentAudio();
    m_audioPlayer->setSource(QUrl::fromLocalFile(fullPath));
    m_audioPlayer->play();
}

void PlayerMainWindow::handleBgmAction(const Slide &slide)
{
    switch (slide.bgmAction) {
    case BgmAction::Stop:
        if (m_bgmPlayer && m_bgmPlayer->playbackState() == QMediaPlayer::PlayingState) {
            m_bgmPausePosition = m_bgmPlayer->position();
            m_bgmPlayer->pause();
            m_bgmPlaying = false;
        }
        break;

    case BgmAction::Continue:
        // 如果有已暂停的背景音乐且当前没有在播放，则继续
        if (m_bgmPlayer && !m_bgmPlaying && !m_currentBgmPath.isEmpty()) {
            m_bgmPlayer->setPosition(m_bgmPausePosition);
            m_bgmPlayer->play();
            m_bgmPlaying = true;
        }
        break;

    case BgmAction::Switch:
        if (!slide.bgmPath.isEmpty()) {
            QString fullPath = m_projectPath + "/" + slide.bgmPath;
            if (QFile::exists(fullPath)) {
                // 停止当前播放
                if (m_bgmPlayer->playbackState() != QMediaPlayer::StoppedState)
                    m_bgmPlayer->stop();
                // 设置新源
                m_bgmPlayer->setSource(QUrl::fromLocalFile(fullPath));
                // 设置循环播放（Qt6 使用 setLoops）
                m_bgmPlayer->setLoops(QMediaPlayer::Infinite);
                m_bgmPlayer->play();
                m_bgmPlaying = true;
                m_currentBgmPath = fullPath;
                m_bgmPausePosition = 0;  // 重置暂停位置
            } else {
                qWarning() << "背景音乐文件不存在:" << fullPath;
            }
        }
        break;
    }
}
void PlayerMainWindow::playSlideAudio(const Slide &slide)
{
    if (slide.slideAudioPath.isEmpty()) {
        stopSlideAudio();
        return;
    }
    QString fullPath = m_projectPath + "/" + slide.slideAudioPath;
    if (!QFile::exists(fullPath)) {
        qWarning() << "幻灯片音频文件不存在:" << fullPath;
        stopSlideAudio();
        return;
    }
    stopSlideAudio();
    m_slideAudioPlayer->setSource(QUrl::fromLocalFile(fullPath));
    m_slideAudioPlayer->play();
}

void PlayerMainWindow::stopSlideAudio()
{
    if (m_slideAudioPlayer) {
        m_slideAudioPlayer->stop();
        m_slideAudioPlayer->setSource(QUrl());
    }
}

void PlayerMainWindow::applySlideAudio(int slideIndex)
{
    NodeData node = m_graph->getNode(m_currentNodeId);
    if (node.id.isEmpty() || node.slides.isEmpty()) return;
    if (slideIndex < 0 || slideIndex >= node.slides.size()) return;
    const Slide &slide = node.slides[slideIndex];
    handleBgmAction(slide);   // 这里处理背景音乐
    playSlideAudio(slide);    // 处理独立音频
}

void PlayerMainWindow::onMediaError(QMediaPlayer::Error error, const QString &errorString)
{
    qWarning() << "音频播放错误:" << error << errorString;
}

void PlayerMainWindow::updateWindowTitle()
{
    NodeData node = m_graph->getNode(m_currentNodeId);
    QString nodeName = node.name.isEmpty() ? tr("未命名节点") : node.name;
    setWindowTitle(tr("互动剧情播放器 - %1").arg(nodeName));
}

// -------------------- 历史栈 --------------------
void PlayerMainWindow::pushToHistory(const QString &nodeId, int slideIndex)
{
    if (!m_history.isEmpty() && m_history.last().first == nodeId && m_history.last().second == slideIndex)
        return;
    m_history.append(qMakePair(nodeId, slideIndex));
    while (m_history.size() > 50)
        m_history.removeFirst();
}

void PlayerMainWindow::goBack()
{
    if (m_history.isEmpty()) {
        statusBar()->showMessage(tr("没有更早的记录"), 2000);
        return;
    }
    auto last = m_history.takeLast();
    QString prevNodeId = last.first;
    int prevSlideIndex = last.second;

    // 防止与当前状态完全相同（一般不会）
    if (prevNodeId == m_currentNodeId && prevSlideIndex == m_currentSlideIndex && !m_history.isEmpty()) {
        last = m_history.takeLast();
        prevNodeId = last.first;
        prevSlideIndex = last.second;
    }

    // 回退时不要再次记录历史
    setCurrentNode(prevNodeId, prevSlideIndex, false);
}

void PlayerMainWindow::onHistoryBackRequested()
{
    if (!m_isPlaying) return;
    goBack();
}