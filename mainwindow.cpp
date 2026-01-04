#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
#include <QPixmap>
#include <QPalette>
#include <QBrush>

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


    connect(ui->playPauseBtn, &QPushButton::clicked, this, &MainWindow::handlePlaySlot);
    // connect(player, &QMediaPlayer::playbackStateChanged,
    //         this, [=](QMediaPlayer::PlaybackState state){
    //             switch (state) {
    //             case QMediaPlayer::PlayingState:
    //                 qDebug() << "Playing";
    //                 break;
    //             case QMediaPlayer::PausedState:
    //                 qDebug() << "Paused";
    //                 break;
    //             case QMediaPlayer::StoppedState:
    //                 qDebug() << "Stopped";
    //                 break;
    //             }
    //         });

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
    setButtonStyle(ui->playPauseBtn, ":/play.png");
    setButtonStyle(ui->nextBtn, ":/next.png");
    setButtonStyle(ui->modeBtn, ":/order.png");
    setButtonStyle(ui->musicListBtn, ":/musicList.png");
}
