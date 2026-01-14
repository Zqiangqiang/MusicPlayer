#include "spectrumwidget.h"
#include <QPainter>
#include <QRandomGenerator>

SpectrumWidget::SpectrumWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumHeight(80);

    m_levels.resize(m_barCount);
    m_levels.fill(0.0f);

    m_timer = new QTimer(this);
    m_timer->setInterval(40); // ~25 FPS

    connect(m_timer, &QTimer::timeout,
            this, &SpectrumWidget::updateSpectrum);
}

void SpectrumWidget::setBarCount(int count)
{
    if (count <= 0)
        return;

    m_barCount = count;
    m_levels.resize(m_barCount);
    m_levels.fill(0.0f);
    update();
}

void SpectrumWidget::setVolume(float volume)
{
    m_volume = qBound(0.0f, volume, 1.0f);
}

void SpectrumWidget::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState) {
        m_timer->start();
    } else {
        m_timer->stop();

        // 平滑归零
        for (float &v : m_levels) {
            v *= 0.3f;
        }
        update();
    }
}

void SpectrumWidget::updateSpectrum()
{
    for (int i = 0; i < m_barCount; ++i) {
        // 基础音量
        float base = m_volume * 0.6f;

        // 随机扰动（制造节奏感）
        float random = QRandomGenerator::global()->bounded(0.4f);

        float target = qMin(1.0f, base + random);

        // 平滑上升 + 缓慢衰减
        if (target > m_levels[i]) {
            m_levels[i] = target;
        } else {
            m_levels[i] *= 0.88f;
        }
    }

    update();
}

void SpectrumWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    if (m_barCount <= 0)
        return;

    int barWidth = w / m_barCount;

    QColor color(170, 190, 200, 140);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);

    for (int i = 0; i < m_barCount; ++i) {
        float heightScale = 0.6f;          // 总体高度缩放
        int maxBarHeight = h * 0.7;        // 最大不超过 70%

        int barHeight = static_cast<int>(m_levels[i] * maxBarHeight * heightScale);
        QRect rect(
            i * barWidth,
            h - barHeight,
            barWidth - 2,
            barHeight
            );
        painter.drawRoundedRect(rect, 3, 3);
    }
}
