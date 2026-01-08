#ifndef ROTATINGDISCWIDGET_H
#define ROTATINGDISCWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QPropertyAnimation>

class RotatingDiscWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal angle READ angle WRITE setAngle)

public:
    explicit RotatingDiscWidget(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pixmap);

    qreal angle() const { return m_angle; }
    void setAngle(qreal angle);

private:
    // override绘制事件
    void paintEvent(QPaintEvent *event) override;

    QPixmap m_pixmap;
    qreal m_angle = 0;
};

#endif // ROTATINGDISCWIDGET_H
