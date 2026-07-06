#ifndef PLAYERMAINWINDOW_H
#define PLAYERMAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QVector>
#include <QPair>
#include "PlayerStoryGraph.h"

class PlayCanvas;
class QTimer;

class PlayerMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PlayerMainWindow(const QString &projectPath, QWidget *parent = nullptr);
    ~PlayerMainWindow();

private slots:
    void onChoiceSelected(const QString &targetNodeId);
    void onMediaError(QMediaPlayer::Error error, const QString &errorString);
    void onCanvasClicked();
    void onHistoryBackRequested();

private:
    void loadStory();
    void setCurrentNode(const QString &nodeId, int slideIndex = 0, bool recordHistory = true);
    void startCurrentNode();
    void stopCurrentAudio();
    void playCurrentAudio();
    void nextSlide();
    void updateWindowTitle();

    void handleBgmAction(const Slide &slide);
    void playSlideAudio(const Slide &slide);
    void stopSlideAudio();
    void applySlideAudio(int slideIndex);

    void pushToHistory(const QString &nodeId, int slideIndex);
    void goBack();

    QString m_projectPath;
    StoryGraph *m_graph;
    PlayCanvas *m_canvas;

    QString m_currentNodeId;
    int m_currentSlideIndex;
    QMediaPlayer *m_audioPlayer;
    QMediaPlayer *m_bgmPlayer;
    QMediaPlayer *m_slideAudioPlayer;
    qint64 m_bgmPausePosition;

    bool m_isPlaying;
    bool m_bgmPlaying;
    QString m_currentBgmPath;

    QVector<QPair<QString, int>> m_history;
};

#endif // PLAYERMAINWINDOW_H