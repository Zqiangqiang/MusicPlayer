#include "rotatingdiscwidget.h"
#include <QPainter>

RotatingDiscWidget::RotatingDiscWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void RotatingDiscWidget::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
    update();
}

void RotatingDiscWidget::setAngle(qreal angle)
{
    m_angle = angle;
    update();
}

void RotatingDiscWidget::paintEvent(QPaintEvent *)
{
    if (m_pixmap.isNull())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 移动原点到中心
    painter.translate(width() / 2, height() / 2);
    painter.rotate(m_angle);
    painter.translate(-width() / 2, -height() / 2);

    // 绘制图片
    painter.drawPixmap(rect(), m_pixmap);
}
