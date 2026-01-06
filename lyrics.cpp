#include "lyrics.h"
#include <QRegularExpression>
#include <QTextCursor>

Lyrics::Lyrics(QTextEdit *edit, QObject *parent)
    : QObject(parent)
    , m_edit(edit)
{
    m_edit->setReadOnly(true);
    m_edit->setFrameShape(QFrame::NoFrame);
    m_edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_edit->setStyleSheet("background: transparent;");
    m_edit->setAlignment(Qt::AlignCenter);
}

void Lyrics::setLyrics(const QString &lrcText)
{
    m_lines.clear();
    m_currentIndex = -1;
    parseLrc(lrcText);
    m_edit->clear();
}

void Lyrics::parseLrc(const QString &lrcText)
{
    // [mm:ss.xxx]text
    QRegularExpression re(R"(\[(\d+):(\d+)\.(\d+)\](.*))");

    const QStringList lines = lrcText.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        auto match = re.match(line);
        if (!match.hasMatch())
            continue;

        QString text = match.captured(4).trimmed();
        if (text.startsWith("录音室") || text.startsWith("混音录音室") || text.isEmpty())
            continue; // 不加入 m_lines

        int min = match.captured(1).toInt();
        int sec = match.captured(2).toInt();
        int ms  = match.captured(3).left(3).toInt(); // 防止 2867 这种

        qint64 timeMs = min * 60 * 1000 + sec * 1000 + ms;
        m_lines.push_back({timeMs, text});
    }
}

int Lyrics::findCurrentLine(qint64 positionMs) const
{
    if (m_lines.isEmpty())
        return -1;

    for (int i = 0; i < m_lines.size(); ++i) {
        if (i + 1 < m_lines.size()) {
            if (positionMs >= m_lines[i].timeMs && positionMs < m_lines[i + 1].timeMs)
                return i;
        } else {
            if (positionMs >= m_lines[i].timeMs)
                return i;
        }
    }
    return -1;
}

void Lyrics::updatePosition(qint64 positionMs)
{
    if (m_lines.isEmpty()) {
        // 没有歌词，显示提示
        m_edit->setHtml(QString("<p style='text-align:center; font-size:24px; color:rgba(255,255,255,0.5);'>%1</p>").arg(m_preludeTip));
        return;
    }

    int index = findCurrentLine(positionMs);

    if (index == -1) {
        // 前奏阶段，还没到第一句歌词
        m_edit->setHtml(QString("<p style='text-align:center; font-size:24px; color:rgba(255,255,255,0.5);'>%1</p>").arg(m_preludeTip));
        return;
    }

    if (index == m_currentIndex)
        return;

    m_currentIndex = index;
    updateDisplay(index);
}

void Lyrics::updateDisplay(int index)
{
    QString html;

    // 显示 7 行，当前行居中
    for (int i = index - 3; i <= index + 3; ++i) {
        QString lineHtml;
        if (i < 0 || i >= m_lines.size()) {
            // 占位空行
            lineHtml = "<p style='text-align:center; min-height:36px;'>&nbsp;</p>";
        } else {
            const LyricLine &line = m_lines[i];
            if (i == index) {
                lineHtml = QString(
                               "<p style='text-align:center; margin:12px 0; font-size:24px; font-weight:bold; color:rgba(255,255,255,1.0);'>%1</p>"
                               ).arg(line.text.toHtmlEscaped());
            } else if (qAbs(i - index) == 1) {
                lineHtml = QString(
                               "<p style='text-align:center; margin:10px 0; font-size:20px; color:rgba(255,255,255,0.55);'>%1</p>"
                               ).arg(line.text.toHtmlEscaped());
            } else {
                lineHtml = QString(
                               "<p style='text-align:center; margin:8px 0; font-size:18px; color:rgba(255,255,255,0.25);'>%1</p>"
                               ).arg(line.text.toHtmlEscaped());
            }
        }
        html += lineHtml;
    }

    m_edit->setHtml(html);
}
