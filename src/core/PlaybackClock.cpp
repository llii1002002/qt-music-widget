#include "PlaybackClock.h"

#include <QtMath>

PlaybackClock::PlaybackClock(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(33);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        if (m_transitioning) {
            // Smooth transition: linearly interpolate from start to target
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     Clock::now() - m_transitionStart).count();
            qreal t = qreal(elapsed) / qreal(TRANSITION_MS);
            if (t >= 1.0) {
                // Transition complete
                m_positionMs = m_transitionTargetPos;
                m_transitioning = false;
                m_anchor = Clock::now();
            } else {
                qint64 pos = m_transitionStartPos
                           + qint64(qreal(m_transitionTargetPos - m_transitionStartPos) * t);
                if (m_durationMs > 0 && pos > m_durationMs)
                    pos = m_durationMs;
                emit tick(pos);
                return;
            }
        }

        if (m_playing) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     Clock::now() - m_anchor).count();
            qint64 pos = m_positionMs + static_cast<qint64>(elapsed);
            if (m_durationMs > 0 && pos > m_durationMs)
                pos = m_durationMs;
            emit tick(pos);
        }
    });
    m_timer.start();
}

void PlaybackClock::sync(qint64 positionMs, qint64 durationMs, bool playing)
{
    m_transitioning = false;
    m_positionMs = qMax<qint64>(0, positionMs);
    m_durationMs = qMax<qint64>(0, durationMs);
    m_playing = playing;
    m_anchor = Clock::now();
}

void PlaybackClock::syncSmooth(qint64 targetPositionMs, qint64 durationMs, bool playing)
{
    m_durationMs = qMax<qint64>(0, durationMs);
    m_playing = playing;

    // Start transition from current displayed position
    m_transitionStartPos = m_positionMs;
    m_transitionTargetPos = qMax<qint64>(0, targetPositionMs);
    m_transitionStart = Clock::now();
    m_transitioning = true;
}

qint64 PlaybackClock::position() const
{
    if (m_transitioning) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 Clock::now() - m_transitionStart).count();
        qreal t = qreal(elapsed) / qreal(TRANSITION_MS);
        if (t >= 1.0)
            return m_transitionTargetPos;
        return m_transitionStartPos
             + qint64(qreal(m_transitionTargetPos - m_transitionStartPos) * t);
    }

    if (!m_playing)
        return m_positionMs;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             Clock::now() - m_anchor).count();
    qint64 pos = m_positionMs + static_cast<qint64>(elapsed);
    if (m_durationMs > 0 && pos > m_durationMs)
        pos = m_durationMs;
    return pos;
}
