#pragma once

#include <QObject>
#include <QTimer>
#include <QtGlobal>
#include <chrono>

/**
 * High-precision interpolation clock (main thread).
 * Re-syncs against SMTC timeline anchors, ticks ~30Hz for smooth lyrics.
 */
class PlaybackClock : public QObject
{
    Q_OBJECT
public:
    explicit PlaybackClock(QObject *parent = nullptr);

    /** Hard sync (immediate jump). */
    void sync(qint64 positionMs, qint64 durationMs, bool playing);

    /** Smoothly transition to target over ~300ms, then continue interpolating. */
    void syncSmooth(qint64 targetPositionMs, qint64 durationMs, bool playing);

    qint64 position() const;
    bool isPlaying() const { return m_playing; }

signals:
    void tick(qint64 positionMs);

private:
    using Clock = std::chrono::steady_clock;

    QTimer m_timer;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    bool m_playing = false;
    Clock::time_point m_anchor = Clock::now();

    // Smooth transition state
    bool m_transitioning = false;
    qint64 m_transitionStartPos = 0;
    qint64 m_transitionTargetPos = 0;
    Clock::time_point m_transitionStart;
    static constexpr int TRANSITION_MS = 300;
};
