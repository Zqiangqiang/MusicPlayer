#include "lyricsmanager.h"

#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QNetworkReply>
#include <QUrlQuery>

LyricsManager::LyricsManager(QObject *parent)
    : QObject(parent)
{
}

QString LyricsManager::lyricsCachePath(const QString &musicPath) const
{
    QString baseDir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation
        );

    QDir dir(baseDir + "/lyrics");
    if (!dir.exists())
        dir.mkpath(".");

    QByteArray hash = QCryptographicHash::hash(
        musicPath.toUtf8(),
        QCryptographicHash::Md5
        );

    return dir.filePath(hash.toHex() + ".lrc");
}

bool LyricsManager::loadFromCache(const QString &musicPath, QString &outLrc) const
{
    QFile file(lyricsCachePath(musicPath));
    if (!file.exists())
        return false;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    outLrc = in.readAll();
    return true;
}

void LyricsManager::saveToCache(const QString &musicPath, const QString &lrcText)
{
    QFile file(lyricsCachePath(musicPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << lrcText;
}

void LyricsManager::requestLyrics(const QString &musicPath, const QString &title, const QString &artist)
{
    // 1️⃣ 先尝试本地缓存
    QString lrc;
    if (loadFromCache(musicPath, lrc)) {
        emit lyricsReady(musicPath, lrc);
        return;
    }

    // 2️⃣ 构造请求（示例，用你之前的 API）
    QUrl url("https://tools.rangotec.com/api/anon/lrc");
    QUrlQuery query;
    query.addQueryItem("title", title);
    query.addQueryItem("artist", artist);
    query.addQueryItem("od", "desc");
    url.setQuery(query);

    QNetworkRequest req(url);
    auto reply = m_net.get(req);

    // 3️⃣ 异步回调
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit lyricsFailed(musicPath);
            return;
        }

        QString lrcText = reply->readAll();

        if (lrcText.isEmpty()) {
            emit lyricsFailed(musicPath);
            return;
        }

        saveToCache(musicPath, lrcText);
        emit lyricsReady(musicPath, lrcText);
    });
}
