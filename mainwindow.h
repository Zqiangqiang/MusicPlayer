#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPropertyAnimation>
#include <QListWidget>

QT_BEGIN_NAMESPACE
enum class PlayMode {
    Sequential,   // 顺序播放
    Random        // 随机播放
};

namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void handlePlaySlot();
    void onMusicItemClicked(QListWidgetItem *item);
    void onModeBtnClicked();

private slots:
    void on_prevBtn_clicked();
    void on_nextBtn_clicked();

private:
    // 设置背景
    void setBackGround(const QString& filename);
    // 设置按钮样式
    void setButtonStyle(QPushButton* btn, const QString& filename);
    // 初始化按钮
    void initButton();
    // 加载指定目录下的歌曲信息
    void loadAppointMusicDir(const QString & dirPath);
    // 播放指定路径歌曲
    void playMusicByIndex(int idx);
    // 更新音乐列表中当前播放状态
    void updatePlayingItemState(int newIndex);
    // 获取下一首歌曲(下标)
    int getNextIndex();
    // 播放下一首（shuffle/order）
    void playNext();
    // 记录历史播放记录
    void recordHistory(int index);
    // 播放上一首（shuffle/order）
    void playPrev();


private:   
    Ui::MainWindow *ui;
    // 音乐播放部件
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    // 动画显示部件
    QPropertyAnimation *m_listAnim;
    bool m_listVisible = false;
    // 播放歌曲路径
    QStringList m_musicList;
    int m_currentIndex = -1;
    QString m_currentMusicPath;
    // 播放模式
    PlayMode m_playMode = PlayMode::Sequential;
    // 播放历史记录
    QVector<int> m_playHistory;
    int m_historyPos = -1;
};
#endif // MAINWINDOW_H
