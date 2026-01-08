#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
#include <QPixmap>
#include <QPalette>
#include <QBrush>
#include <QPainter>
#include <QUrl>
#include <QUrlQuery>
#include <QPainterPath>
#include <QMessageBox>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // 默认不显示progressTipLabel标签
    ui->progressTipLabel->hide();
    setWindowTitle("MusicPlayer");
    setFixedSize(800, 600);

    // 初始化网络管理对象
    m_lyrics = new LyricsManager(this);
    // 歌词加载好时显示歌词
    connect(m_lyrics, &LyricsManager::lyricsReady, this, &MainWindow::onLyricsReady);
    // 初始化歌词对象
    m_lyricsView = new Lyrics(ui->lyricsEdit, this);
    // 初始化网络请求管理对象
    m_networkManager = new QNetworkAccessManager(this);

    // 初始化背景
    setBackGround(":/background.png");

    // 初始化所有按钮
    initButton();
    // 初始化状态栏
    QLabel *statusLabel = new QLabel(this);
    statusLabel->setText("Author:xiang");
    this->statusBar()->addPermanentWidget(statusLabel);
    // 处理菜单栏操作
    connect(ui->actionopen_Dir, &QAction::triggered, this, &MainWindow::onActionOpenDirClicked);
    connect(ui->actionOpen_file, &QAction::triggered, this, &MainWindow::onActionOpenFileClicked);
    ui->actionOpen_file->setShortcut(QKeySequence::Open);
    connect(ui->actionClose_dir, &QAction::triggered, this, &MainWindow::onActionCloseDirClicked);
    ui->actionClose_dir->setShortcut(QKeySequence::Close);

    // Qt6 必须显式创建音频输出，否则没有声音
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    // 设置音量（0.0 ~ 1.0）
    m_audioOutput->setVolume(1.0);
    // 设置音频源
    //m_player->setSource(QUrl::fromLocalFile("/Users/xiang/Desktop/music/Fa_Ru_Xue.mp3"));

    // 关联播放按钮
    connect(ui->playPauseBtn, &QPushButton::clicked, this, &MainWindow::handlePlaySlot);

    // 默认隐藏MusicList列表
    ui->rightPanel->hide();
    // 设置音乐列表背景透明
    ui->musicList->setAttribute(Qt::WA_TranslucentBackground);
    ui->musicList->setStyleSheet("background: transparent;");
    ui->musicList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->musicList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 初始化动画
    m_listAnim = new QPropertyAnimation(ui->rightPanel, "geometry");
    m_listAnim->setDuration(300);
    m_listAnim->setEasingCurve(QEasingCurve::OutCubic);

    // 关联音乐列表按钮
    connect(ui->musicListBtn, &QPushButton::clicked, this, [=](){
        if (m_musicDir.isEmpty()) {
            onActionOpenDirClicked();
        }
        loadAppointMusicDir(m_musicDir);

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

    // 当前歌曲播放结束自动播放下一曲
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

    // 开始拖动进度条时显示标签
    connect(ui->progressSlider, &QSlider::sliderPressed, this, [=](){
        ui->progressTipLabel->show();
    });

    // 拖动中
    connect(ui->progressSlider, &QSlider::sliderMoved, this, [=](int value){
        // 转换为 mm:ss
        int seconds = value / 1000;
        int minutes = seconds / 60;
        seconds = seconds % 60;
        QString text = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
        ui->progressTipLabel->setText(text);

        // 动态移动 QLabel 位置（在 slider 上方）
        // slider 宽度
        int sliderWidth = ui->progressSlider->width();
        // 根据 value / max 得到比例
        double ratio = double(value) / ui->progressSlider->maximum();
        int x = ui->progressSlider->x() + ratio * sliderWidth - ui->progressTipLabel->width()/3;
        int y = ui->progressSlider->y() - ui->progressTipLabel->height() - 5; // 5 px 上方
        ui->progressTipLabel->move(x, y);
    });

    // 松开进度条slider
    connect(ui->progressSlider, &QSlider::sliderReleased, this, [=](){
        ui->progressTipLabel->hide();
        m_player->setPosition(ui->progressSlider->value());
    });

    // 初始化唱片
    m_discWidget = new RotatingDiscWidget(this);
    m_discWidget->setFixedSize(240, 240); // 圆盘大小
    m_discWidget->move(20, 50);           // 放置位置
    m_discWidget->setPixmap(QPixmap(":/disc.png"));

    m_discAnimation = new QPropertyAnimation(m_discWidget, "angle", this);
    m_discAnimation->setStartValue(0);
    m_discAnimation->setEndValue(360);
    m_discAnimation->setDuration(10000); // 10秒一圈
    m_discAnimation->setLoopCount(-1);
    m_discAnimation->setEasingCurve(QEasingCurve::Linear);

    // 关联唱片旋转
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::updateDiscState);

    // 初始化歌词显示窗
    ui->lyricsEdit->setReadOnly(true);
    ui->lyricsEdit->setFont(QFont("Arial", 20));
    ui->lyricsEdit->setFrameShape(QFrame::NoFrame);
    ui->lyricsEdit->setStyleSheet("background: transparent; color: white;");
    ui->lyricsEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // 隐藏竖向滚动条
    ui->lyricsEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 隐藏横向滚动条
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
    btn->setFixedSize(60, 60);
    btn->setIcon(QIcon(iconPath));
    btn->setIconSize(QSize(55, 55));
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

void MainWindow::onActionOpenDirClicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,                     // 父窗口
        "Select Dir",            // 对话框标题
        "./",                   // 默认打开当前目录
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks // 选项
        );

    if (!dir.isEmpty()) {
        m_musicDir = dir;
    }
}

void MainWindow::onActionOpenFileClicked()
{
    QString filepath = QFileDialog::getOpenFileName(this, "Select File", "./", "Audio (*.mp3 *.wav *.M4a)");
    if (!filepath.isEmpty()) m_currentMusicPath = filepath;
    m_player->setSource(QUrl::fromLocalFile(m_currentMusicPath));
    // 更新唱片封面
    updateDiscCover(m_currentMusicPath);
}

void MainWindow::onActionCloseDirClicked()
{
    m_musicDir = nullptr;
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

    QFont font("Arial", 22); // 设置字体大小
    int rowHeight = ui->musicList->height() / 10;       // 每行高度

    for (QFileInfo &info : files) {
        // 显示给用户看的：歌名
        QListWidgetItem *item = new QListWidgetItem(info.baseName());
        item->setFont(font);
        item->setSizeHint(QSize(item->sizeHint().width(), rowHeight)); // 设置行高
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

    // 切换唱片图片
    updateDiscCover(m_currentMusicPath);

    // 加载歌词
    QString fileName = QFileInfo(m_currentMusicPath).completeBaseName();
    QStringList parts = fileName.split('_');
    // 获取歌手(可选项)
    QString currentTitle = parts[0];
    QString currentArtist = parts[1];
    m_lyrics->requestLyrics(m_currentMusicPath, currentTitle, currentArtist);
    // 更新歌词
    m_lyricsView->setPreludeTip("歌词即将开始...");
}

void MainWindow::updatePlayingItemState(int newIndex)
{
    // 清除旧状态
    for (int i = 0; i < ui->musicList->count(); ++i) {
        QListWidgetItem *item = ui->musicList->item(i);
        item->setForeground(Qt::white);
        QFont font = item->font();  // 保留原来的字体大小
        font.setBold(false);        // 取消粗体
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

    if (m_lyricsView)
        m_lyricsView->updatePosition(position);
}

void MainWindow::onSliderReleased()
{
    // 播放对应进度条位置
    m_player->setPosition(ui->progressSlider->value());
}

void MainWindow::updateDiscState()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_discAnimation->start();   // 开始旋转
    } else {
        m_discAnimation->pause();   // 暂停旋转
    }
}

void MainWindow::updateDiscCover(const QString &musicPath)
{
    QPixmap cover;

    // 获取音乐文件所在目录
    QFileInfo fileInfo(musicPath);
    QDir dir = fileInfo.dir();

    // 尝试找同名图片
    QString baseName = fileInfo.completeBaseName(); // 不带扩展名
    QString coverPath1 = dir.filePath(baseName + ".jpg");  // MySong.jpg
    QString coverPath2 = dir.filePath("cover.jpg");        // 或者固定 cover.jpg

    if (QFile::exists(coverPath1)) {
        cover.load(coverPath1);
        cover = getCircularPixmap(cover);
        m_discWidget->setPixmap(cover);
    } else if (QFile::exists(coverPath2)) {
        cover.load(coverPath2);
        cover = getCircularPixmap(cover);
        m_discWidget->setPixmap(cover);
    } else {
        // 本地没找到，使用默认圆盘先占位
        cover.load(":/disc.png");
        m_discWidget->setPixmap(cover);

        // 尝试在线获取封面
        QString title = QFileInfo(musicPath).completeBaseName().split("_")[0];
        QString artist = QFileInfo(musicPath).completeBaseName().split("_")[1];
        QString urlStr = QString("https://api.lrc.cx/cover?title=%1&artist=%2")
                            .arg(QUrl::toPercentEncoding(title)).arg(QUrl::toPercentEncoding(artist));

        QNetworkRequest requestCover(QUrl(urlStr, QUrl::StrictMode));
        QNetworkReply *reply = m_networkManager->get(requestCover);

        connect(reply, &QNetworkReply::finished, this, [this, reply, dir, baseName]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QPixmap pix;
                if (pix.loadFromData(data)) {
                    pix = getCircularPixmap(pix);
                    m_discWidget->setPixmap(pix);
                    // 保存到本地目录，文件名用歌曲名，后缀 jpg
                    QString savePath = dir.filePath(baseName + ".jpg");
                    if (!pix.save(savePath, "JPG")) {
                        qDebug() << "封面保存失败:" << savePath;
                    } else {
                        qDebug() << "封面已保存到:" << savePath;
                    }
                } else {
                    qDebug() << "封面下载失败:" << reply->errorString();
                }
            }
            reply->deleteLater();
        });
    }
}

QPixmap MainWindow::getCircularPixmap(const QPixmap &src)
{
    int size = qMin(src.width(), src.height()); // 取最小边
    QPixmap dest(size, size);
    dest.fill(Qt::transparent); // 背景透明

    QPainter painter(&dest);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, size, size); // 圆形路径
    painter.setClipPath(path);         // 裁剪
    painter.drawPixmap(0, 0, src);     // 绘制原图
    return dest;
}

void MainWindow::onLyricsReady(const QString &, const QString &lrc)
{
    m_lyricsView->setLyrics(lrc);
}

void MainWindow::on_prevBtn_clicked()
{
    playPrev();
}

void MainWindow::on_nextBtn_clicked()
{
    playNext();
}

