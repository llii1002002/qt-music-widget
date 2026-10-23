#include "PlayerController.h"

#include "ConfigStore.h"
#include "CoverImageProvider.h"
#include "PlaybackClock.h"
#include "LyricsEngine.h"
#include "ThemeEngine.h"
#include "SpectrumProvider.h"
#include "../system/AudioCaptureWorker.h"
#include "../system/DeviceMonitor.h"

#include <QImage>
#include <QDebug>
#include <QFile>
#include <QTextStream>

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
{}

PlayerController::~PlayerController()
{
    shutdown();
}

void PlayerController::init(ConfigStore *config, CoverImageProvider *coverProvider)
{
    m_config = config;
    m_coverProvider = coverProvider;

    m_clock = new PlaybackClock(this);
    m_lyrics = new LyricsEngine(config, this);
    m_theme = new ThemeEngine(config, this);
    m_spectrumProvider = new SpectrumProvider(this);
    m_media = new MediaSessionWorker(this);
    m_audio = new AudioCaptureWorker(this);
    m_device = new DeviceMonitor(this);

    m_spectrum.fill(0.0, config->intValue(CfgKey::SpectrumBars));
    m_media->setOnlyCloudMusic(config->boolValue(CfgKey::MediaOnlyCloudMusic));

    connect(m_media, &MediaSessionWorker::snapshotChanged,
            this, &PlayerController::onSnapshot, Qt::QueuedConnection);
    connect(m_media, &MediaSessionWorker::coverReady,
            this, &PlayerController::onCover, Qt::QueuedConnection);
    connect(m_media, &MediaSessionWorker::failed,
            this, [](const QString &reason) { qWarning("%s", qPrintable(reason)); });

    connect(m_audio, &AudioCaptureWorker::bandsReady,
            this, &PlayerController::onBands, Qt::QueuedConnection);
    connect(m_audio, &AudioCaptureWorker::simulatedChanged,
            this, &PlayerController::onSimulatedChanged, Qt::QueuedConnection);

    connect(m_device, &DeviceMonitor::defaultRenderDeviceChanged,
            m_audio, &AudioCaptureWorker::requestRestart, Qt::QueuedConnection);

    connect(m_clock, &PlaybackClock::tick, this, &PlayerController::onClockTick);
    connect(m_lyrics, &LyricsEngine::lyricsChanged,
            this, &PlayerController::onLyricsChanged);
    connect(m_theme, &ThemeEngine::paletteReady, this, &PlayerController::onPalette);

    connect(m_config, &ConfigStore::changed,
            this, &PlayerController::onConfigChanged);

    onPalette(m_theme->palette().accent, m_theme->palette().bg1,
              m_theme->palette().bg2, m_theme->palette().text);
}

void PlayerController::start()
{
    applyAudioConfig();
    m_media->start();
    m_audio->start();
    m_device->start();
}

void PlayerController::shutdown()
{
    if (m_media) {
        m_media->stop();
    }
    if (m_audio) {
        m_audio->stop();
    }
    if (m_device) {
        m_device->wait(2000);
    }
}

void PlayerController::applyAudioConfig()
{
    m_audio->requestReconfigure(m_config->intValue(CfgKey::PerfFftSize),
                                m_config->intValue(CfgKey::SpectrumBars),
                                m_config->intValue(CfgKey::SpectrumFps));
}

void PlayerController::onSnapshot(const MediaSnapshot &s)
{
    const bool newSong = s.hasSession
                         && ((s.title != m_title) || (s.artist != m_artist));

    // Detect state changes for lyric alignment
    const bool wasPlaying = m_playing;
    const qint64 oldPos = m_positionMs;

    m_hasSession = s.hasSession;
    m_playing = (s.status == 1);
    m_title = s.title;
    m_artist = s.artist;
    m_durationMs = s.durationMs;
    m_expectedCoverKey = s.coverKey;

    const bool stateChanged = (m_playing != wasPlaying);

    // --- Lyric alignment logic ---
    // SMTC positionMs is unreliable (always 0 for NetEase Cloud Music).
    // Only hard-sync position on song change.
    // Update playing state on pause/resume, but keep local position (don't jump).
    if (newSong) {
        m_clock->sync(s.positionMs, s.durationMs, m_playing);
    } else if (stateChanged) {
        // Pause/resume: update playing state, keep current local position
        m_clock->sync(oldPos, s.durationMs, m_playing);
    }

    m_positionMs = m_clock->position();
    m_audio->setPlaying(m_playing && m_hasSession);

    if (newSong) {
        m_lyrics->setSong(m_title, m_artist);
    }

    emit mediaChanged();
    emit positionMsChanged();

    // Refresh scroll immediately
    onClockTick(m_positionMs);
}

void PlayerController::onCover(const QString &key, const QByteArray &imageData)
{
    if (key != m_expectedCoverKey && m_expectedCoverKey.isEmpty() == false) {
        // Cover for a previous song; still cache it but don't surface.
        QImage img;
        img.loadFromData(imageData);
        if (!img.isNull())
            m_coverProvider->add(key, img);
        return;
    }

    QImage img;
    if (!img.loadFromData(imageData))
        return;

    m_coverProvider->add(key, img);

    static quint64 s_coverVersion = 0;
    m_coverUrl = QUrl(QStringLiteral("image://cover/%1#%2")
                          .arg(key).arg(++s_coverVersion));
    emit coverUrlChanged();
    m_theme->analyze(img);
}

void PlayerController::onBands(const QVector<qreal> &bands)
{
    const qreal sensitivity = m_config->realValue(CfgKey::SpectrumSensitivity);
    QVector<qreal> scaled(bands.size());
    for (int i = 0; i < bands.size(); ++i)
        scaled[i] = qBound(0.0, bands[i] * sensitivity, 1.0);

    if (scaled.size() != m_spectrum.size())
        m_spectrum = QVector<qreal>(scaled.size(), 0.0);
    m_spectrum = m_spectrumProvider->process(scaled);
    emit spectrumChanged();
}

void PlayerController::onClockTick(qint64 positionMs)
{
    m_positionMs = positionMs;
    emit positionMsChanged();

    if (!m_lyrics->hasLyric())
        return;

    int line = -1;
    const qreal scroll = m_lyrics->scrollAt(positionMs, &line);
    if (line != m_currentLine) {
        m_currentLine = line;
        emit currentLineChanged();
    }
    if (!qFuzzyCompare(scroll, m_lyricScroll)) {
        m_lyricScroll = scroll;
        emit lyricScrollChanged();
    }
}

void PlayerController::onLyricsChanged()
{
    m_lyricTexts.clear();
    const auto &lines = m_lyrics->lines();
    m_lyricTexts.reserve(lines.size());
    for (const auto &l : lines)
        m_lyricTexts << l.text;
    m_noLyrics = m_lyricTexts.isEmpty();
    m_currentLine = -1;
    m_lyricScroll = -1;
    emit lyricsChanged();
    emit lyricScrollChanged();
}

void PlayerController::onPalette(const QColor &accent, const QColor &bg1,
                                 const QColor &bg2, const QColor &text)
{
    m_accent = accent;
    m_bg1 = bg1;
    m_bg2 = bg2;
    m_textColor = text;
    emit paletteChanged();
}

void PlayerController::onSimulatedChanged(bool simulated)
{
    if (m_simulated != simulated) {
        m_simulated = simulated;
        emit simulatedChanged();
    }
}

void PlayerController::onConfigChanged(const QString &key)
{
    if (key == QLatin1String(CfgKey::PerfFftSize)
        || key == QLatin1String(CfgKey::SpectrumBars)
        || key == QLatin1String(CfgKey::SpectrumFps)) {
        if (key == QLatin1String(CfgKey::SpectrumBars))
            m_spectrum.fill(0.0, m_config->intValue(CfgKey::SpectrumBars));
        applyAudioConfig();
    } else if (key == QLatin1String(CfgKey::ThemeAuto)
               || key == QLatin1String(CfgKey::ThemeAccent)) {
        m_theme->reapply();
    } else if (key == QLatin1String(CfgKey::MediaOnlyCloudMusic)) {
        m_media->setOnlyCloudMusic(m_config->boolValue(CfgKey::MediaOnlyCloudMusic));
    }
}
