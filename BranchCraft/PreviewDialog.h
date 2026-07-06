#ifndef PREVIEWDIALOG_H
#define PREVIEWDIALOG_H

#include <QDialog>

class QLabel;
class QMediaPlayer;
class QVideoWidget;
class QSlider;
class QPushButton;

class PreviewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreviewDialog(const QString &filePath, QWidget *parent = nullptr);
    ~PreviewDialog();

private slots:
    void onPlayPause();
    void onStop();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onSliderMoved(int value);
    void onError();

private:
    void setupImagePreview(const QString &path);
    void setupAudioPreview(const QString &path);
    void setupVideoPreview(const QString &path);
    void showError(const QString &errorMsg);
    void clearMediaPlayer();

    // 图片预览用
    QLabel *m_label = nullptr;

    // 音视频预览用
    QMediaPlayer *m_player = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QSlider *m_positionSlider = nullptr;
    QPushButton *m_playPauseBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    bool m_isAudio = false;   // 用于区分音频和视频（视频需要显示视频窗口）
};

#endif // PREVIEWDIALOG_H