#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QAtomicInt>
#include <QList>
#include "ConfigStore.h"

struct LyricLine
{
    qint64 ms = 0;
    QString text;
};

class QNetworkAccessManager;
class QNetworkReply;

/**
 * Parses LRC files, fuzzy-matches them locally by title (+artist).
 * Local parsing runs on the global thread pool; when no local file
 * matches, falls back to the public NetEase Cloud Music web API.
 */
class LyricsEngine : public QObject
{
    Q_OBJECT
public:
    explicit LyricsEngine(ConfigStore *config, QObject *parent = nullptr);
    ~LyricsEngine() override;

    /** Cancels any previous lookup asynchronously. */
    void setSong(const QString &title, const QString &artist);

    const QVector<LyricLine> &lines() const { return m_lines; }
    bool hasLyric() const { return !m_lines.isEmpty(); }

    /** Fractional line index (for smooth scroll) at position ms. */
    qreal scrollAt(qint64 positionMs, int *currentLine) const;

    static QVector<LyricLine> parseLrc(const QByteArray &raw);

signals:
    void lyricsChanged();

private:
    void applyResult(int token, QVector<LyricLine> lines);
    void startOnlineLookup(int token, const QString &title, const QString &artist);
    void handleSearchReply(QNetworkReply *reply, int token,
                           const QString &title, const QString &artist);
    void handleLyricReply(QNetworkReply *reply, int token);
    void trackReply(QNetworkReply *reply);
    void abortOnline();

    ConfigStore *m_config;
    QVector<LyricLine> m_lines;
    QAtomicInt m_token { 0 };
    QNetworkAccessManager *m_nam = nullptr;
    QList<QNetworkReply *> m_replies;
};
