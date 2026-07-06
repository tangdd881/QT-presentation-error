#include "PreviewDialog.h"
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QImageReader>
#include <QFileInfo>
#include <QMessageBox>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QUrl>

PreviewDialog::PreviewDialog(const QString &filePath, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("预览: ") + QFileInfo(filePath).fileName());

    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();

    if (suffix == "jpg" || suffix == "jpeg" || suffix == "png" ||
        suffix == "bmp" || suffix == "gif" || suffix == "webp") {
        setupImagePreview(filePath);
    } else if (suffix == "mp3" || suffix == "wav" || suffix == "ogg" || suffix == "flac") {
        setupAudioPreview(filePath);
    } else if (suffix == "mp4" || suffix == "avi" || suffix == "mov" || suffix == "mkv") {
        setupVideoPreview(filePath);
    } else {
        showError(tr("不支持预览此文件类型: %1").arg(suffix));
    }
}

PreviewDialog::~PreviewDialog()
{
    clearMediaPlayer();
}

// ---------- 图片预览 ----------
void PreviewDialog::setupImagePreview(const QString &path)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);

    QImageReader reader(path);
    if (reader.canRead()) {
        QSize originalSize = reader.size();
        if (originalSize.isValid()) {
            QSize maxSize(800, 600);
            QSize targetSize = originalSize.scaled(maxSize, Qt::KeepAspectRatio);
            reader.setScaledSize(targetSize);
            QImage image = reader.read();
            if (!image.isNull()) {
                QPixmap pixmap = QPixmap::fromImage(image);
                m_label->setPixmap(pixmap);
                resize(pixmap.size());
            } else {
                showError(tr("无法解码图片"));
                return;
            }
        } else {
            showError(tr("无法获取图片尺寸"));
            return;
        }
    } else {
        showError(tr("无法加载图片: %1").arg(path));
        return;
    }

    layout->addWidget(m_label);

    QPushButton *closeBtn = new QPushButton(tr("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
}

// ---------- 音频预览（带声音）----------
void PreviewDialog::setupAudioPreview(const QString &path)
{
    m_isAudio = true;
    QVBoxLayout *layout = new QVBoxLayout(this);

    // 显示文件名
    QLabel *infoLabel = new QLabel(this);
    infoLabel->setText(tr("正在播放: %1").arg(QFileInfo(path).fileName()));
    infoLabel->setAlignment(Qt::AlignCenter);
    QFont font = infoLabel->font();
    font.setPointSize(12);
    infoLabel->setFont(font);
    layout->addWidget(infoLabel);

    // 进度条
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_positionSlider->setRange(0, 0);
    layout->addWidget(m_positionSlider);

    // 控制按钮
    QHBoxLayout *controlLayout = new QHBoxLayout;
    m_playPauseBtn = new QPushButton(tr("播放"), this);
    m_stopBtn = new QPushButton(tr("停止"), this);
    QPushButton *closeBtn = new QPushButton(tr("关闭"), this);

    controlLayout->addStretch();
    controlLayout->addWidget(m_playPauseBtn);
    controlLayout->addWidget(m_stopBtn);
    controlLayout->addStretch();
    controlLayout->addWidget(closeBtn);
    controlLayout->addStretch();
    layout->addLayout(controlLayout);

    // 创建音频输出（必须！）
    QAudioOutput *audioOutput = new QAudioOutput(this);
    audioOutput->setVolume(0.8); // 音量 80%

    // 创建播放器并绑定音频输出
    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(audioOutput);   // 关键：连接声音输出

    // 连接信号
    connect(m_player, &QMediaPlayer::positionChanged, this, &PreviewDialog::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &PreviewDialog::onDurationChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &PreviewDialog::onError);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &PreviewDialog::onPlayPause);
    connect(m_stopBtn, &QPushButton::clicked, this, &PreviewDialog::onStop);
    connect(m_positionSlider, &QSlider::sliderMoved, this, &PreviewDialog::onSliderMoved);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    // 设置媒体源并播放
    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();
    m_playPauseBtn->setText(tr("暂停"));

    resize(400, 200);
}

// ---------- 视频预览（带声音）----------
void PreviewDialog::setupVideoPreview(const QString &path)
{
    m_isAudio = false;
    QVBoxLayout *layout = new QVBoxLayout(this);

    // 视频显示窗口
    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_videoWidget, 1);

    // 进度条和控制栏
    QHBoxLayout *controlLayout = new QHBoxLayout;
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_positionSlider->setRange(0, 0);
    m_playPauseBtn = new QPushButton(tr("播放"), this);
    m_stopBtn = new QPushButton(tr("停止"), this);
    QPushButton *closeBtn = new QPushButton(tr("关闭"), this);

    controlLayout->addWidget(m_playPauseBtn);
    controlLayout->addWidget(m_stopBtn);
    controlLayout->addWidget(m_positionSlider, 1);
    controlLayout->addWidget(closeBtn);
    layout->addLayout(controlLayout);

    // 创建音频输出（视频也需要，否则无声）
    QAudioOutput *audioOutput = new QAudioOutput(this);
    audioOutput->setVolume(0.8);

    // 创建播放器并绑定音频输出
    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(audioOutput);   // 关键：让视频的声音也能输出

    // 设置视频输出
    m_player->setVideoOutput(m_videoWidget);

    // 连接信号
    connect(m_player, &QMediaPlayer::positionChanged, this, &PreviewDialog::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &PreviewDialog::onDurationChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &PreviewDialog::onError);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &PreviewDialog::onPlayPause);
    connect(m_stopBtn, &QPushButton::clicked, this, &PreviewDialog::onStop);
    connect(m_positionSlider, &QSlider::sliderMoved, this, &PreviewDialog::onSliderMoved);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    // 设置媒体源并播放
    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();
    m_playPauseBtn->setText(tr("暂停"));

    resize(800, 600);
}

void PreviewDialog::showError(const QString &errorMsg)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *errorLabel = new QLabel(errorMsg, this);
    errorLabel->setWordWrap(true);
    errorLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(errorLabel);

    QPushButton *closeBtn = new QPushButton(tr("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignCenter);

    resize(400, 150);
}

void PreviewDialog::clearMediaPlayer()
{
    if (m_player) {
        m_player->stop();
        delete m_player;
        m_player = nullptr;
    }
}

// ---------- 播放控制槽函数 ----------
void PreviewDialog::onPlayPause()
{
    if (!m_player) return;
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_playPauseBtn->setText(tr("播放"));
    } else {
        m_player->play();
        m_playPauseBtn->setText(tr("暂停"));
    }
}

void PreviewDialog::onStop()
{
    if (!m_player) return;
    m_player->stop();
    m_playPauseBtn->setText(tr("播放"));
    if (m_positionSlider)
        m_positionSlider->setValue(0);
}

void PreviewDialog::onPositionChanged(qint64 position)
{
    if (m_positionSlider && m_positionSlider->maximum() > 0) {
        m_positionSlider->blockSignals(true);
        m_positionSlider->setValue(position);
        m_positionSlider->blockSignals(false);
    }
}

void PreviewDialog::onDurationChanged(qint64 duration)
{
    if (m_positionSlider) {
        m_positionSlider->setRange(0, duration);
    }
}

void PreviewDialog::onSliderMoved(int value)
{
    if (m_player) {
        m_player->setPosition(value);
    }
}

void PreviewDialog::onError()
{
    if (!m_player) return;
    QString errorString = m_player->errorString();
    showError(tr("播放错误: %1").arg(errorString));
    clearMediaPlayer();
}