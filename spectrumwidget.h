#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include <QMediaPlayer>

class SpectrumWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpectrumWidget(QWidget *parent = nullptr);

    // 设置柱子数量
    void setBarCount(int count);

public slots:
    // 统一频谱处理接口
    void onTick();
    // 设置播放状态
    void setPlaying(bool playing);
    // 更新音量（可选）
    void setVolume(float volume);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateSpectrum();

private:
    int m_barCount = 32;
    QVector<float> m_levels;   // 0.0 ~ 1.0
    QTimer *m_timer;
    float m_volume = 1.0f;
    bool m_playing = false;
};

#endif // SPECTRUMWIDGET_H
