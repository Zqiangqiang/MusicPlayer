#include "lyricsmanager.h"

#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

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

void LyricsManager::requestLyrics(const QString &musicPath, const QString &title, const QString &artist, const QString &album)
{
    // 1️⃣ 先尝试本地缓存
    QString lrc;
    if (loadFromCache(musicPath, lrc)) {
        emit lyricsReady(musicPath, lrc);
        return;
    }

    // 2️⃣ 构造请求
    QUrl url("https://api.lrc.cx/lyrics");
    QUrlQuery query;
    query.addQueryItem("title", title);
    query.addQueryItem("artist", artist);
    query.addQueryItem("album", album);
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

        // 新接口：直接返回 LRC 文本
        QString lrcText = QString::fromUtf8(reply->readAll()).trimmed();

        // 基本合法性判断
        if (lrcText.isEmpty() || !lrcText.contains('[')) {
            emit lyricsFailed(musicPath);
            return;
        }

        // 非法LRC数据不保存
        if (!lrcText.contains(QRegularExpression(R"(\[\d+:\d+\.\d+\])"))) {
            emit lyricsFailed(musicPath);
            return;
        }

        // 保存到缓存
        saveToCache(musicPath, lrcText);

        // 发射信号
        emit lyricsReady(musicPath, lrcText);
    });
}
