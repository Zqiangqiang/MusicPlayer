#pragma once

#include <QObject>
#include <QTextEdit>
#include <QVector>

struct LyricLine
{
    qint64 timeMs;
    QString text;
};

class Lyrics : public QObject
{
    Q_OBJECT
public:
    explicit Lyrics(QTextEdit *edit, QObject *parent = nullptr);

    // 设置 LRC 文本
    void setLyrics(const QString &lrcText);

    // 播放器位置变化时调用（ms）
    void updatePosition(qint64 positionMs);

    // 设置前奏提示信息
    void setPreludeTip(const QString &tip) { m_preludeTip = tip; }

private:
    void parseLrc(const QString &lrcText);
    int  findCurrentLine(qint64 positionMs) const;
    void updateDisplay(int index);

private:
    QTextEdit *m_edit = nullptr;
    QVector<LyricLine> m_lines;
    int m_currentIndex = -1;
    QString m_preludeTip = "歌词即将开始 . . .";
};
