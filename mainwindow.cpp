#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
#include <QPixmap>
#include <QPalette>
#include <QBrush>
#include <QMessageBox>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("MusicPlayer");
    setFixedSize(800, 600);
    // 初始化背景
    setBackGround(":/background.png");

    // 初始化所有按钮
    initButton();

    // Qt6 必须显式创建音频输出，否则没有声音
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    // 设置音量（0.0 ~ 1.0）
    m_audioOutput->setVolume(1.0);
    // 设置音频源
    m_player->setSource(QUrl::fromLocalFile("/Users/xiang/Desktop/music/Fa_Ru_Xue.mp3"));

    // 关联播放按钮
    connect(ui->playPauseBtn, &QPushButton::clicked, this, &MainWindow::handlePlaySlot);

    QString musicDir = "/Users/xiang/Desktop/music";
    loadAppointMusicDir(musicDir);

    // 默认隐藏MusicList列表
    ui->rightPanel->hide();
    // 设置音乐列表背景透明
    ui->musicList->setAttribute(Qt::WA_TranslucentBackground);
    ui->musicList->setStyleSheet("background: transparent;");

    // 初始化动画
    m_listAnim = new QPropertyAnimation(ui->rightPanel, "geometry");
    m_listAnim->setDuration(300);
    m_listAnim->setEasingCurve(QEasingCurve::OutCubic);

    // 关联音乐列表按钮
    connect(ui->musicListBtn, &QPushButton::clicked, this, [=](){
        QRect startRect, endRect;

        int w = ui->rightPanel->width();
        int h = ui->centralwidget->height();

        if (!m_listVisible) {
            // 从右侧外面滑入
            startRect = QRect(width(), 0, w, h);
            endRect   = QRect(width() - w, 0, w, h);
            ui->rightPanel->show();
        } else {
            // 滑出到右侧
            startRect = QRect(width() - w, 0, w, h);
            endRect   = QRect(width(), 0, w, h);
        }

        m_listAnim->stop();
        m_listAnim->setStartValue(startRect);
        m_listAnim->setEndValue(endRect);
        m_listAnim->start();

        m_listVisible = !m_listVisible;
    });

    // 关联歌曲列表中表项
    connect(ui->musicList, &QListWidget::itemClicked, this, &MainWindow::onMusicItemClicked);

    // 当前歌曲播放结束自动播放下一曲（默认顺序播放）
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            playNext();
        }
    });

    // 关联随机播放按钮
    connect(ui->modeBtn, &QPushButton::clicked, this, &MainWindow::onModeBtnClicked);

    // 每次播放歌曲时设置进度条
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);

    // 歌曲播放时修改进度值
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);

    // 拖动进度
    connect(ui->progressSlider, &QSlider::sliderReleased, this, &MainWindow::onSliderReleased);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handlePlaySlot()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        ui->playPauseBtn->setIcon(QIcon(":/pause.png"));
    } else {
        m_player->play();
        ui->playPauseBtn->setIcon(QIcon(":/play.png"));
    }
}

void MainWindow::onMusicItemClicked(QListWidgetItem *item)
{
    if (!item)
        return;


    int index = ui->musicList->row(item);

    if (index < 0 || index >= m_musicList.size())
        return;

    playMusicByIndex(index);
}

void MainWindow::onModeBtnClicked()
{
    if (m_playMode == PlayMode::Sequential) {
        m_playMode = PlayMode::Random;
        ui->modeBtn->setIcon(QIcon(":/shuffle.png"));
    } else {
        m_playMode = PlayMode::Sequential;
        ui->modeBtn->setIcon(QIcon(":/order.png"));
    }
}

void MainWindow::setBackGround(const QString &filename)
{
    QPixmap pm = QPixmap(filename);
    // 获取窗口大小
    QSize windowSize = this->size();
    QPixmap scaledPm = pm.scaled(windowSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    // 创建调色板
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, QBrush(scaledPm));
    // 将调色板绘制到窗口
    this->setPalette(palette);
    this->setAutoFillBackground(true);
}

void MainWindow::setButtonStyle(QPushButton *btn,const QString& iconPath)
{
    btn->setFixedSize(80, 80);
    btn->setIcon(QIcon(iconPath));
    btn->setIconSize(QSize(64, 64));
    btn->setStyleSheet(
        "QPushButton {"
        " background-color: transparent;"
        " padding: 0px;"
        "}"
    );
}

void MainWindow::initButton()
{
    setButtonStyle(ui->prevBtn, ":/previous.png");
    setButtonStyle(ui->playPauseBtn, ":/pause.png");
    setButtonStyle(ui->nextBtn, ":/next.png");
    setButtonStyle(ui->modeBtn, ":/order.png");
    setButtonStyle(ui->musicListBtn, ":/musicList.png");
}

void MainWindow::loadAppointMusicDir(const QString &dirPath)
{
    ui->musicList->clear();

    QDir dir(dirPath);
    if (!dir.exists()) {
        QMessageBox::warning(this, "waring", "Current Dir isn't exist");
        return ;
    }

    QStringList filters = {"*.mp3", "*.wav", "*.m4a"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    for (QFileInfo &info : files) {
        // 显示给用户看的：歌名
        QListWidgetItem *item = new QListWidgetItem(info.baseName());
        // 把路径保存到成员变量中
        m_musicList.append(info.absoluteFilePath());
        ui->musicList->addItem(item);
    }
}

void MainWindow::playMusicByIndex(int index)
{
    if (index < 0 || index >= m_musicList.size())
        return;

    m_currentIndex = index;
    m_currentMusicPath = m_musicList[index];

    m_player->setSource(QUrl::fromLocalFile(m_currentMusicPath));
    m_player->play();

    updatePlayingItemState(index);
    recordHistory(index);       // 记录播放历史
    ui->playPauseBtn->setIcon(QIcon(":/play.png"));

}

void MainWindow::updatePlayingItemState(int newIndex)
{
    // 清除旧状态
    for (int i = 0; i < ui->musicList->count(); ++i) {
        QListWidgetItem *item = ui->musicList->item(i);
        item->setForeground(Qt::white);
        item->setFont(QFont());
        item->setIcon(QIcon());
    }

    // 设置当前播放项
    QListWidgetItem *currentItem = ui->musicList->item(newIndex);

    QFont font = currentItem->font();
    font.setBold(true); // 粗体
    currentItem->setFont(font);

    currentItem->setForeground(QColor(0, 200, 255)); // 播放中颜色
    currentItem->setIcon(QIcon(":/playing.png"));

    ui->musicList->setCurrentRow(newIndex);
}

int MainWindow::getNextIndex()
{
    if (m_musicList.isEmpty())
        return -1;

    if (m_playMode == PlayMode::Sequential) {
        return (m_currentIndex + 1) % m_musicList.size();
    }

    // Random
    if (m_musicList.size() == 1)
        return m_currentIndex;

    int next;
    do {
        next = QRandomGenerator::global()->bounded(m_musicList.size());
    } while (next == m_currentIndex);

    return next;
}

void MainWindow::playNext()
{
    if (m_musicList.isEmpty())
        return;

    // 获取下一首歌曲下标
    int nextIndex = getNextIndex();
    if (nextIndex != -1)
        playMusicByIndex(nextIndex);
}

void MainWindow::recordHistory(int index)
{
    // 如果从中间跳转，截断后面的历史
    if (m_historyPos + 1 < m_playHistory.size()) {
        m_playHistory.resize(m_historyPos + 1);
    }

    m_playHistory.append(index);
    m_historyPos = m_playHistory.size() - 1;
}

void MainWindow::playPrev()
{
    if (m_musicList.isEmpty())
        return;

    // 顺序播放：直接 -1
    if (m_playMode == PlayMode::Sequential) {
        int prev = (m_currentIndex - 1 + m_musicList.size()) % m_musicList.size();
        playMusicByIndex(prev);
        return;
    }

    // 随机播放：从历史回退
    // 随机模式的上一曲不是随机歌曲，而是刚刚听过的上一曲
    if (m_historyPos > 0) {
        m_historyPos--;
        int prevIndex = m_playHistory[m_historyPos];
        playMusicByIndex(prevIndex);
    }
}

void MainWindow::onDurationChanged(qint64 duration)
{
    ui->progressSlider->setRange(0, duration);
}

void MainWindow::onPositionChanged(qint64 position)
{
    if (!ui->progressSlider->isSliderDown())
        ui->progressSlider->setValue(position);
}

void MainWindow::onSliderReleased()
{
    // 播放对应进度条位置
    m_player->setPosition(ui->progressSlider->value());
}

void MainWindow::on_prevBtn_clicked()
{
    playPrev();
}


void MainWindow::on_nextBtn_clicked()
{
    playNext();
}

