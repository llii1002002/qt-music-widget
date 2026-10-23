#pragma once

#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

#include <QThread>
#include <QVector>
#include <atomic>

#include "FftAnalyzer.h"

/**
 * WASAPI loopback capture on a dedicated MTA thread.
 * FFT -> log-spaced frequency bands. Falls back to simulated rhythm when
 * no capture endpoint / WASAPI initialization is available.
 */
class AudioCaptureWorker : public QThread
{
    Q_OBJECT
public:
    explicit AudioCaptureWorker(QObject *parent = nullptr);
    ~AudioCaptureWorker() override;

    void setPlaying(bool playing);
    void requestReconfigure(int fftSize, int bandCount, int fps);
    void requestRestart();
    void stop();

signals:
    void bandsReady(const QVector<qreal> &bands);
    void simulatedChanged(bool simulated);

protected:
    void run() override;

private:
    bool initCapture(IMMDeviceEnumerator *enumerator, IAudioClient **client,
                     IAudioCaptureClient **capture, WAVEFORMATEX **format,
                     HANDLE audioEvent);
    void releaseCapture(IAudioClient *client, IAudioCaptureClient *capture,
                        WAVEFORMATEX *format);
    void captureLoop(IAudioClient *client, IAudioCaptureClient *capture,
                     const WAVEFORMATEX *format, HANDLE audioEvent,
                     HANDLE reconfigEvent);
    void simulatedLoop(HANDLE reconfigEvent);
    void emitBands(const float *magnitudes, int sampleRate);
    qint64 nextEmitDeadline(qint64 nowMs) const;

    HANDLE m_stopEvent = nullptr;
    HANDLE m_reconfigEvent = nullptr;

    std::atomic<bool> m_playing { false };
    std::atomic<int> m_fftSize { 1024 };
    std::atomic<int> m_bandCount { 48 };
    std::atomic<int> m_fps { 60 };
    std::atomic<bool> m_restartPending { false };

    FftAnalyzer m_fft;
    QVector<float> m_accum;
    qint64 m_lastEmitMs = 0;
    bool m_simulated = false;
};
