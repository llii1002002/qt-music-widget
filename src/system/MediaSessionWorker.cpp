// Windows + C++/WinRT headers first, then Qt headers.
#include <windows.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

#include "MediaSessionWorker.h"

#include <QCryptographicHash>
#include <QDebug>

#include <algorithm>
#include <cmath>

using winrt::hresult_error;
namespace WMC = winrt::Windows::Media::Control;
namespace WSS = winrt::Windows::Storage::Streams;

namespace {

QString toQString(const winrt::hstring &h)
{
    // hstring is UTF-16; never route through winrt::to_string() (ACP) or CJK
    // characters become '?'.
    return QString::fromWCharArray(h.c_str(), int(h.size()));
}

bool isCloudMusicAumid(const QString &aumid)
{
    const QString s = aumid.toLower();
    return s.contains(QStringLiteral("cloudmusic"))
        || s.contains(QStringLiteral("orpheus"))
        || s.contains(QStringLiteral("netease"))
        || s.contains(QStringLiteral("网易云"));
}

qint64 now100ns()
{
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    ULARGE_INTEGER ul;
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    return static_cast<qint64>(ul.QuadPart);
}

} // namespace

struct MediaSessionWorker::Impl
{
    WMC::GlobalSystemMediaTransportControlsSessionManager manager { nullptr };
    WMC::GlobalSystemMediaTransportControlsSession session { nullptr };
    winrt::event_token currentChangedToken {};
    winrt::event_token sessionsChangedToken {};
    winrt::event_token mediaToken {};
    winrt::event_token playbackToken {};
    winrt::event_token timelineToken {};

    MediaSnapshot lastEmitted;
    QString lastSongKey;
    QString lastCoverKey;
    qint64 songChangeTime = 0;   // 100ns ticks when the current song started

    void detachSession(WMC::GlobalSystemMediaTransportControlsSession &s)
    {
        if (!s)
            return;
        try { s.MediaPropertiesChanged(mediaToken); } catch (...) {}
        try { s.PlaybackInfoChanged(playbackToken); } catch (...) {}
        try { s.TimelinePropertiesChanged(timelineToken); } catch (...) {}
        s = nullptr;
    }
};

MediaSessionWorker::MediaSessionWorker(QObject *parent)
    : QThread(parent)
    , m_impl(new Impl())
{}

MediaSessionWorker::~MediaSessionWorker()
{
    stop();
    delete m_impl;
}

void MediaSessionWorker::stop()
{
    if (m_stopEvent)
        SetEvent(m_stopEvent);
    wait(5000);
}

void MediaSessionWorker::setOnlyCloudMusic(bool only)
{
    m_onlyCloud.store(only);
    if (m_notifyEvent)
        SetEvent(m_notifyEvent);
}

void MediaSessionWorker::run()
{
    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    m_notifyEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    winrt::init_apartment(winrt::apartment_type::multi_threaded);

    try {
        m_impl->manager =
            WMC::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();

        m_impl->currentChangedToken = m_impl->manager.CurrentSessionChanged(
            [this](const auto &, const auto &) { SetEvent(m_notifyEvent); });
        m_impl->sessionsChangedToken = m_impl->manager.SessionsChanged(
            [this](const auto &, const auto &) { SetEvent(m_notifyEvent); });
    } catch (const hresult_error &e) {
        emit failed(QStringLiteral("SMTC unavailable: %1")
                        .arg(toQString(e.message())));
        m_impl->manager = nullptr;
    }

    auto pickTarget = [this]() -> WMC::GlobalSystemMediaTransportControlsSession {
        WMC::GlobalSystemMediaTransportControlsSession target { nullptr };
        if (!m_impl->manager)
            return target;
        try {
            const auto sessions = m_impl->manager.GetSessions();
            for (const auto &s : sessions) {
                if (isCloudMusicAumid(toQString(s.SourceAppUserModelId()))) {
                    target = s;
                    break;
                }
            }
            if (!target && !m_onlyCloud.load())
                target = m_impl->manager.GetCurrentSession();
        } catch (...) {
            target = nullptr;
        }
        return target;
    };

    auto refresh = [this, &pickTarget]() {
        if (!m_impl->manager)
            return;

        const WMC::GlobalSystemMediaTransportControlsSession target = pickTarget();

        if (target != m_impl->session) {
            m_impl->detachSession(m_impl->session);
            m_impl->session = target;
            m_impl->lastSongKey.clear();
            m_impl->lastCoverKey.clear();
            if (target) {
                try {
                    m_impl->mediaToken = target.MediaPropertiesChanged(
                        [this](const auto &, const auto &) { SetEvent(m_notifyEvent); });
                    m_impl->playbackToken = target.PlaybackInfoChanged(
                        [this](const auto &, const auto &) { SetEvent(m_notifyEvent); });
                    m_impl->timelineToken = target.TimelinePropertiesChanged(
                        [this](const auto &, const auto &) { SetEvent(m_notifyEvent); });
                } catch (...) {}
            }
        }

        if (!target) {
            MediaSnapshot empty;
            if (m_impl->lastEmitted.hasSession) {
                m_impl->lastEmitted = empty;
                m_impl->lastSongKey.clear();
                m_impl->lastCoverKey.clear();
                emit snapshotChanged(empty);
            }
            return;
        }

        MediaSnapshot snap;
        QString pendingCoverKey;
        QByteArray pendingCoverBytes;
        snap.hasSession = true;
        try {
            snap.sourceAumid = toQString(target.SourceAppUserModelId());

            const auto playback = target.GetPlaybackInfo();
            switch (playback.PlaybackStatus()) {
            case WMC::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
                snap.status = 1;
                break;
            case WMC::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
                snap.status = 2;
                break;
            default:
                snap.status = 0;
                break;
            }

            double rate = 1.0;
            try {
                if (const auto r = playback.PlaybackRate())
                    rate = r.Value();
            } catch (...) {}
            if (rate <= 0.0 || rate > 4.0)
                rate = 1.0;

            const auto timeline = target.GetTimelineProperties();
            const qint64 start = timeline.StartTime().count() / 10000;
            const qint64 end = timeline.EndTime().count() / 10000;
            if (end > start)
                snap.durationMs = end - start;

            qint64 positionMs = timeline.Position().count() / 10000;
            const qint64 lastUpdated =
                timeline.LastUpdatedTime().time_since_epoch().count();
            if (snap.status == 1 && lastUpdated != 0) {
                const qint64 elapsedMs = (now100ns() - lastUpdated) / 10000;
                if (elapsedMs > 0)
                    positionMs += qint64(elapsedMs * rate);
            }
            if (snap.durationMs > 0)
                positionMs = qBound<qint64>(0, positionMs, snap.durationMs);
            snap.positionMs = positionMs;

            try {
                const auto props = target.TryGetMediaPropertiesAsync().get();
                if (props) {
                    snap.title = toQString(props.Title());
                    snap.artist = toQString(props.Artist());
                    if (snap.artist.isEmpty())
                        snap.artist = toQString(props.AlbumArtist());
                    snap.album = toQString(props.AlbumTitle());

                    const QString songKey = snap.title + QLatin1Char('\n')
                                          + snap.artist + QLatin1Char('\n')
                                          + snap.album;
                    const QString coverKey = QString::fromLatin1(
                        QCryptographicHash::hash(songKey.toUtf8(),
                                                 QCryptographicHash::Md5).toHex());

                    if (songKey != m_impl->lastSongKey) {
                        m_impl->lastSongKey = songKey;
                        // Do NOT clear lastCoverKey here: keep showing the
                        // previous cover until the SMTC thumbnail has had
                        // time to settle. We start a 500 ms grace period
                        // during which cover fetches are skipped.
                        m_impl->songChangeTime = now100ns();
                    }

                    // Only fetch the thumbnail after the 500 ms grace period
                    // so we don't pick up the previous song's thumbnail.
                    const bool coverReady =
                        m_impl->songChangeTime == 0
                        || (now100ns() - m_impl->songChangeTime) >= 500LL * 10000LL;

                    if (coverReady && coverKey != m_impl->lastCoverKey) {
                        const auto thumb = props.Thumbnail();
                        if (thumb) {
                            try {
                                const auto stream = thumb.OpenReadAsync().get();
                                const uint32_t size = uint32_t(stream.Size());
                                if (size > 0 && size < 8u * 1024u * 1024u) {
                                    WSS::Buffer buffer(size);
                                    const auto loaded =
                                        stream.ReadAsync(buffer, size,
                                                         WSS::InputStreamOptions::None).get();
                                    const uint32_t len = loaded.Length();
                                    if (len > 0) {
                                        // Defer the coverReady emit until AFTER
                                        // snapshotChanged: PlayerController::
                                        // onCover compares key against
                                        // m_expectedCoverKey, which is only
                                        // refreshed by onSnapshot. If we emit
                                        // coverReady first, the new cover is
                                        // mistaken for a stale one and cached
                                        // but never surfaced.
                                        pendingCoverKey = coverKey;
                                        pendingCoverBytes =
                                            QByteArray(reinterpret_cast<const char *>(loaded.data()),
                                                       int(len));
                                        m_impl->lastCoverKey = coverKey;
                                        m_impl->songChangeTime = 0; // cover fetched, stop polling
                                    }
                                }
                            } catch (...) {}
                        }
                    }
                    snap.coverKey = m_impl->lastCoverKey;
                }
            } catch (...) {}
        } catch (...) {
            return;
        }

        const MediaSnapshot &prev = m_impl->lastEmitted;
        const bool sameMeta =
            prev.hasSession == snap.hasSession
            && prev.status == snap.status
            && prev.title == snap.title
            && prev.artist == snap.artist
            && prev.album == snap.album
            && prev.coverKey == snap.coverKey
            && prev.durationMs == snap.durationMs
            && std::abs(prev.positionMs - snap.positionMs) < 1500;
        if (!sameMeta) {
            m_impl->lastEmitted = snap;
            emit snapshotChanged(snap);
        }

        if (!pendingCoverKey.isEmpty())
            emit coverReady(pendingCoverKey, pendingCoverBytes);
    };

    // Initial enumeration.
    refresh();

    // Event-driven, plus a 500 ms safety net: SMTC occasionally drops
    // SessionsChanged when a player is killed, and WinRT event delivery can
    // stall. The 500 ms timeout also doubles as the cover-fetch grace
    // period trigger after a song switch.
    const HANDLE handles[2] = { m_stopEvent, m_notifyEvent };
    while (true) {
        const DWORD wr = WaitForMultipleObjects(2, handles, FALSE, 500);
        if (wr == WAIT_OBJECT_0)
            break;
        if (wr == WAIT_OBJECT_0 + 1) {
            refresh();
        } else if (wr == WAIT_TIMEOUT) {
            try {
                // A song switch started a 500 ms grace period before we
                // fetch the new thumbnail. If that period has elapsed and
                // no SMTC event woke us up, force a refresh so the cover
                // is pulled (the thumbnail should be settled by then).
                if (m_impl->songChangeTime != 0
                    && (now100ns() - m_impl->songChangeTime) >= 500LL * 10000LL) {
                    refresh();
                } else if (pickTarget() != m_impl->session) {
                    refresh();
                }
            } catch (...) {}
        }
    }

    m_impl->detachSession(m_impl->session);
    if (m_impl->manager) {
        try { m_impl->manager.CurrentSessionChanged(m_impl->currentChangedToken); } catch (...) {}
        try { m_impl->manager.SessionsChanged(m_impl->sessionsChangedToken); } catch (...) {}
        m_impl->manager = nullptr;
    }
    m_impl->lastEmitted = MediaSnapshot();

    winrt::uninit_apartment();
    CloseHandle(m_notifyEvent);
    CloseHandle(m_stopEvent);
    m_notifyEvent = nullptr;
    m_stopEvent = nullptr;
}
