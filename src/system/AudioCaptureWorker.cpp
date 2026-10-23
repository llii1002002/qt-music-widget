#include "AudioCaptureWorker.h"

#include <QElapsedTimer>
#include <QtGlobal>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

constexpr int TIMEOUT_MS = 150;

void appendMono(float *dst, int &count, int capacity, const BYTE *data,
                UINT32 frames, const WAVEFORMATEX *fmt, bool silentFlag)
{
    const int channels = fmt->nChannels ? fmt->nChannels : 1;
    if (silentFlag) {
        const int n = qMin<int>(frames, capacity - count);
        for (int i = 0; i < n; ++i)
            dst[count++] = 0.0f;
        return;
    }

    const bool floatFormat = fmt->wBitsPerSample == 32
        && (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT
            || (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE
                && reinterpret_cast<const WAVEFORMATEXTENSIBLE *>(fmt)->SubFormat
                       == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT));
    const int cap = capacity - count;
    int n = qMin<int>(frames, cap);

    if (floatFormat) {
        const float *src = reinterpret_cast<const float *>(data);
        for (UINT32 i = 0; i < UINT32(n); ++i) {
            float mix = 0.0f;
            for (int c = 0; c < channels; ++c)
                mix += src[i * channels + c];
            dst[count++] = mix / channels;
        }
    } else {
        const int bytes = fmt->wBitsPerSample / 8;
        for (UINT32 i = 0; i < UINT32(n); ++i) {
            const BYTE *frame = data + (i * channels * bytes);
            qint32 mix = 0;
            for (int c = 0; c < channels; ++c) {
                if (bytes == 2)
                    mix += reinterpret_cast<const qint16 *>(frame)[c];
                else if (bytes == 4)
                    mix += reinterpret_cast<const qint32 *>(frame)[c] >> 16;
                else
                    mix += qint32(frame[c]) - 128;
            }
            dst[count++] = float(mix / channels) / 32768.0f;
        }
    }
}
} // namespace

AudioCaptureWorker::AudioCaptureWorker(QObject *parent)
    : QThread(parent)
{}

AudioCaptureWorker::~AudioCaptureWorker()
{
    stop();
}

void AudioCaptureWorker::setPlaying(bool playing)
{
    m_playing.store(playing);
}

void AudioCaptureWorker::requestReconfigure(int fftSize, int bandCount, int fps)
{
    m_fftSize.store(fftSize);
    m_bandCount.store(bandCount);
    m_fps.store(fps);
    m_restartPending.store(true);
    if (m_reconfigEvent)
        SetEvent(m_reconfigEvent);
}

void AudioCaptureWorker::requestRestart()
{
    m_restartPending.store(true);
    if (m_reconfigEvent)
        SetEvent(m_reconfigEvent);
}

void AudioCaptureWorker::stop()
{
    if (m_stopEvent)
        SetEvent(m_stopEvent);
    wait(3000);
}

bool AudioCaptureWorker::initCapture(IMMDeviceEnumerator *enumerator, IAudioClient **client,
                                     IAudioCaptureClient **capture, WAVEFORMATEX **format,
                                     HANDLE audioEvent)
{
    IMMDevice *device = nullptr;
    HRESULT hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr) || !device)
        return false;

    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                          reinterpret_cast<void **>(client));
    device->Release();
    if (FAILED(hr) || !*client)
        return false;

    hr = (*client)->GetMixFormat(format);
    if (FAILED(hr)) {
        (*client)->Release();
        *client = nullptr;
        return false;
    }

    constexpr REFERENCE_TIME bufferDuration = 5LL * 10000000LL; // 5s, shared loopback
    hr = (*client)->Initialize(AUDCLNT_SHAREMODE_SHARED,
                               AUDCLNT_STREAMFLAGS_LOOPBACK
                                   | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                               bufferDuration, 0, *format, nullptr);
    if (FAILED(hr)) {
        qWarning("[Audio] Initialize failed hr=0x%08lX ch=%d rate=%lu bits=%d",
                 (unsigned long)hr, (*format)->nChannels,
                 (unsigned long)(*format)->nSamplesPerSec,
                 (*format)->wBitsPerSample);
        CoTaskMemFree(*format);
        (*client)->Release();
        *client = nullptr;
        return false;
    }

    hr = (*client)->GetService(__uuidof(IAudioCaptureClient),
                               reinterpret_cast<void **>(capture));
    if (FAILED(hr)) {
        CoTaskMemFree(*format);
        (*client)->Release();
        *client = nullptr;
        return false;
    }

    (*client)->SetEventHandle(audioEvent);

    UINT32 bufferFrames = 0;
    (*client)->GetBufferSize(&bufferFrames);
    (*client)->Start();
    return true;
}

void AudioCaptureWorker::releaseCapture(IAudioClient *client, IAudioCaptureClient *capture,
                                        WAVEFORMATEX *format)
{
    if (client) {
        client->Stop();
        client->Release();
    }
    if (capture)
        capture->Release();
    if (format)
        CoTaskMemFree(format);
}

void AudioCaptureWorker::emitBands(const float *magnitudes, int sampleRate)
{
    const int n = m_fft.size();
    const int bands = m_bandCount.load();
    QVector<qreal> out(bands);

    const double fMin = 30.0;
    const double fMax = qMin(16000.0, sampleRate / 2.0 - 200.0);
    const double ratio = std::pow(fMax / fMin, 1.0 / bands);

    for (int b = 0; b < bands; ++b) {
        const double f0 = fMin * std::pow(ratio, b);
        const double f1 = f0 * ratio;
        int bin0 = qBound(1, int(f0 * n / sampleRate), n / 2);
        int bin1 = qBound(bin0 + 1, int(f1 * n / sampleRate), n / 2 + 1);
        float sum = 0.0f;
        for (int k = bin0; k < bin1; ++k)
            sum += magnitudes[k];
        const float mean = sum / float(bin1 - bin0);
        // Broadband music per-bin magnitude is only ~1e-3..2e-2; apply a
        // noise gate plus sqrt perceptual compression into roughly 0..1.
        const float level = std::sqrt((std::max)(0.0f, mean));
        const float v = (level - 0.012f) / 0.11f;
        out[b] = qBound(0.0, qreal(v), 1.0);
    }
    emit bandsReady(out);
}

void AudioCaptureWorker::captureLoop(IAudioClient *client, IAudioCaptureClient *capture,
                                     const WAVEFORMATEX *format, HANDLE audioEvent,
                                     HANDLE reconfigEvent)
{
    const int n = m_fftSize.load();
    m_fft.init(n);
    m_accum.clear();
    m_accum.resize(n * 2);
    int filled = 0;

    QVector<float> block(4096 * 8);
    QVector<float> magnitude(n / 2 + 1);

    while (WaitForSingleObject(m_stopEvent, 0) != WAIT_OBJECT_0) {
        if (WaitForSingleObject(reconfigEvent, 0) == WAIT_OBJECT_0)
            break;

        const DWORD wr = WaitForSingleObject(audioEvent, TIMEOUT_MS);
        if (wr != WAIT_OBJECT_0)
            continue;

        UINT32 padding = 0;
        if (FAILED(client->GetCurrentPadding(&padding)))
            break; // device likely lost; force restart / fallback
        if (padding == 0)
            continue;

        UINT32 remaining = padding;
        while (remaining > 0) {
            UINT32 packetLength = 0;
            if (FAILED(capture->GetNextPacketSize(&packetLength)) || packetLength == 0)
                break;

            BYTE *data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            HRESULT hr = capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
            if (FAILED(hr))
                break;

            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
            if (int(frames) > block.size())
                block.resize(frames);
            int monoCount = 0;
            appendMono(block.data(), monoCount, block.size(), data, frames, format, silent);
            capture->ReleaseBuffer(frames);
            remaining -= frames;

            int consumed = 0;
            while (monoCount - consumed >= n - filled) {
                const int need = n - filled;
                memcpy(m_accum.data() + filled, block.data() + consumed,
                       size_t(need) * sizeof(float));
                consumed += need;
                filled = 0;

                m_fft.magnitude(m_accum.constData(), magnitude.data());
                const qint64 now = qint64(GetTickCount64());
                const qint64 interval = 1000 / qMax(1, m_fps.load());
                if (now - m_lastEmitMs >= interval) {
                    emitBands(magnitude.constData(), int(format->nSamplesPerSec));
                    m_lastEmitMs = now;
                }

                // 50% overlap for smoother animation
                memmove(m_accum.data(), m_accum.data() + n / 2,
                        size_t(n / 2) * sizeof(float));
                filled = n / 2;
            }
            if (monoCount > consumed) {
                const int left = monoCount - consumed;
                memmove(m_accum.data() + filled, block.data() + consumed,
                        size_t(left) * sizeof(float));
                filled += left;
            }
        }
    }
}

void AudioCaptureWorker::simulatedLoop(HANDLE reconfigEvent)
{
    const int bands = m_bandCount.load();
    QElapsedTimer t;
    t.start();

    while (WaitForSingleObject(m_stopEvent, 0) != WAIT_OBJECT_0) {
        if (WaitForSingleObject(reconfigEvent, 0) == WAIT_OBJECT_0)
            break;
        WaitForSingleObject(m_stopEvent, 16);

        const qint64 now = qint64(GetTickCount64());
        const qint64 interval = 1000 / qMax(1, m_fps.load());
        if (now - m_lastEmitMs < interval)
            continue;
        m_lastEmitMs = now;

        QVector<qreal> out(bands);
        if (m_playing.load()) {
            const double secs = t.nsecsElapsed() / 1.0e9;
            const double pulse = 0.35 + 0.65 * (0.5 + 0.5 * std::sin(secs * 2.0 * M_PI * 0.55));
            for (int i = 0; i < bands; ++i) {
                const double f = 1.0 + 5.0 * i / bands;
                double v = 0.25 + 0.55 * std::fabs(std::sin(secs * 2.0 * M_PI * f + i * 0.4));
                v *= pulse * (0.5 + 0.5 * std::sin(secs * 7.3 + i * 1.7));
                out[i] = qBound(0.0, v, 1.0);
            }
        } else {
            out.fill(0.0);
        }
        emit bandsReady(out);
    }
}

void AudioCaptureWorker::run()
{
    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    m_reconfigEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
        return;

    while (WaitForSingleObject(m_stopEvent, 0) != WAIT_OBJECT_0) {
        ResetEvent(m_reconfigEvent);
        m_restartPending.store(false);
        m_lastEmitMs = 0;

        IMMDeviceEnumerator *enumerator = nullptr;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                      __uuidof(IMMDeviceEnumerator),
                                      reinterpret_cast<void **>(&enumerator));
        if (FAILED(hr) || !enumerator) {
            if (!m_simulated) {
                m_simulated = true;
                emit simulatedChanged(true);
            }
            simulatedLoop(m_reconfigEvent);
            continue;
        }

        IAudioClient *client = nullptr;
        IAudioCaptureClient *capture = nullptr;
        WAVEFORMATEX *format = nullptr;
        const HANDLE audioEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        if (initCapture(enumerator, &client, &capture, &format, audioEvent)) {
            if (m_simulated) {
                m_simulated = false;
                emit simulatedChanged(false);
            }
            captureLoop(client, capture, format, audioEvent, m_reconfigEvent);
        } else if (!m_simulated) {
            m_simulated = true;
            emit simulatedChanged(true);
        }

        if (!client && !m_simulated) {
            m_simulated = true;
            emit simulatedChanged(true);
        }
        if (m_simulated && (!client || m_restartPending.load()))
            simulatedLoop(m_reconfigEvent);

        if (client)
            releaseCapture(client, capture, format);
        CloseHandle(audioEvent);
        enumerator->Release();
    }

    CoUninitialize();
    CloseHandle(m_reconfigEvent);
    CloseHandle(m_stopEvent);
    m_reconfigEvent = nullptr;
    m_stopEvent = nullptr;
}
