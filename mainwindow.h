#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QMediaPlayer>
#include <QAudioOutput>

QT_BEGIN_NAMESPACE
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

private:
    // 设置背景
    void setBackGround(const QString& filename);
    // 设置按钮样式
    void setButtonStyle(QPushButton* btn, const QString& filename);
    // 初始化按钮
    void initButton();

private:
    Ui::MainWindow *ui;
    QMediaPlayer* m_player;
    QAudioOutput *m_audioOutput;
};
#endif // MAINWINDOW_H
