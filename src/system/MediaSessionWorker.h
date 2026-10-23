#pragma once

#include <windows.h>

#include <QThread>
#include <QString>
#include <QByteArray>
#include <atomic>

/** Value-type media snapshot, cross-thread safe (queued connections only). */
struct MediaSnapshot
{
    bool hasSession = false;
    int status = 0;          // 0 stopped/idle, 1 playing, 2 paused
    QString sourceAumid;
    QString title;
    QString artist;
    QString album;
    QString coverKey;        // empty when no cover
    qint64 positionMs = 0;
    qint64 durationMs = 0;
};
Q_DECLARE_METATYPE(MediaSnapshot)

/**
 * WinRT SMTC worker on a dedicated MTA thread.
 * Filters the NetEase Cloud Music session (falls back to current session).
 * All WinRT calls stay on this thread; WinRT events only wake the loop.
 */
class MediaSessionWorker : public QThread
{
    Q_OBJECT
public:
    explicit MediaSessionWorker(QObject *parent = nullptr);
    ~MediaSessionWorker() override;

    void stop();
    void setOnlyCloudMusic(bool only);

signals:
    void snapshotChanged(const MediaSnapshot &snapshot);
    void coverReady(const QString &key, const QByteArray &imageData);
    void failed(const QString &reason);

protected:
    void run() override;

private:
    struct Impl;
    friend struct Impl;
    Impl *m_impl = nullptr;

    HANDLE m_stopEvent = nullptr;
    HANDLE m_notifyEvent = nullptr;
    std::atomic<bool> m_onlyCloud { false };
};
