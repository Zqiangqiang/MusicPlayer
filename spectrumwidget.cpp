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
    m_timer->start();

    // 所有的update事件都在onTick中处理
    connect(m_timer, &QTimer::timeout, this, &SpectrumWidget::onTick);
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

void SpectrumWidget::onTick()
{
    if (!m_playing) {
        // 暂停态：统一回落
        for (float &v : m_levels) {
            v *= 0.85f;
            if (v < 0.01f)
                v = 0.0f;
        }
        update();
        return;
    }

    updateSpectrum();
}

void SpectrumWidget::setPlaying(bool playing)
{
    m_playing = playing;
}

void SpectrumWidget::setVolume(float volume)
{
    m_volume = qBound(0.0f, volume, 1.0f);
}

void SpectrumWidget::updateSpectrum()
{
    for (int i = 0; i < m_barCount; ++i) {

        // 1. 中频权重（人声更高）
        float mid = (m_barCount - 1) / 2.0f;
        float dist = std::abs(i - mid) / mid;
        float freqWeight = 1.0f - dist * dist;

        // 2. 随机扰动（避免死板）
        float noise = 0.6f + 0.4f * QRandomGenerator::global()->generateDouble();

        // 3. 能量合成
        float target = m_volume * freqWeight * noise;

        // 4. 非线性放大落差
        target = std::pow(target, 1.8f);

        // 5. 平滑上升 + 快速衰减
        if (target > m_levels[i])
            m_levels[i] = target;
        else
            m_levels[i] *= 0.85f;
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
