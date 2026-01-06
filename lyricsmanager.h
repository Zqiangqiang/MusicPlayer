#pragma once

#include <QObject>
#include <QNetworkAccessManager>

class LyricsManager : public QObject
{
    Q_OBJECT

public:
    explicit LyricsManager(QObject *parent = nullptr);

    // 对外唯一接口
    void requestLyrics(const QString &musicPath, const QString &title, const QString &artist);

signals:
    // 成功拿到歌词（不管来自缓存还是网络）
    void lyricsReady(const QString &musicPath, const QString &lrcText);

    // 获取失败
    void lyricsFailed(const QString &musicPath);

private:
    // 缓存相关
    QString lyricsCachePath(const QString &musicPath) const;
    bool loadFromCache(const QString &musicPath, QString &outLrc) const;
    void saveToCache(const QString &musicPath, const QString &lrcText);

private:
    QNetworkAccessManager m_net;
};
