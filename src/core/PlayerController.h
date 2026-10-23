#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QUrl>
#include <QColor>
#include <QImage>

#include "../system/MediaSessionWorker.h"

class ConfigStore;
class CoverImageProvider;
class AudioCaptureWorker;
class DeviceMonitor;
class PlaybackClock;
class LyricsEngine;
class ThemeEngine;
class SpectrumProvider;

/** Global business facade exposed to QML as `player`. Main thread. */
class PlayerController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool hasSession READ hasSession NOTIFY mediaChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY mediaChanged)
    Q_PROPERTY(QString title READ title NOTIFY mediaChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY mediaChanged)
    Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY positionMsChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY mediaChanged)
    Q_PROPERTY(QUrl coverUrl READ coverUrl NOTIFY coverUrlChanged)
    Q_PROPERTY(QStringList lyricTexts READ lyricTexts NOTIFY lyricsChanged)
    Q_PROPERTY(int currentLine READ currentLine NOTIFY lyricScrollChanged)
    Q_PROPERTY(qreal lyricScroll READ lyricScroll NOTIFY lyricScrollChanged)
    Q_PROPERTY(bool noLyrics READ noLyrics NOTIFY lyricsChanged)
    Q_PROPERTY(QVector<qreal> spectrum READ spectrum NOTIFY spectrumChanged)
    Q_PROPERTY(QColor accent READ accent NOTIFY paletteChanged)
    Q_PROPERTY(QColor bgColor1 READ bgColor1 NOTIFY paletteChanged)
    Q_PROPERTY(QColor bgColor2 READ bgColor2 NOTIFY paletteChanged)
    Q_PROPERTY(QColor textColor READ textColor NOTIFY paletteChanged)
    Q_PROPERTY(bool simulated READ simulated NOTIFY simulatedChanged)

public:
    explicit PlayerController(QObject *parent = nullptr);
    ~PlayerController() override;

    void init(ConfigStore *config, CoverImageProvider *coverProvider);
    void start();
    void shutdown();

    bool hasSession() const { return m_hasSession; }
    bool playing() const { return m_playing; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    qint64 positionMs() const { return m_positionMs; }
    qint64 durationMs() const { return m_durationMs; }
    QUrl coverUrl() const { return m_coverUrl; }
    QStringList lyricTexts() const { return m_lyricTexts; }
    int currentLine() const { return m_currentLine; }
    qreal lyricScroll() const { return m_lyricScroll; }
    bool noLyrics() const { return m_noLyrics; }
    QVector<qreal> spectrum() const { return m_spectrum; }
    QColor accent() const { return m_accent; }
    QColor bgColor1() const { return m_bg1; }
    QColor bgColor2() const { return m_bg2; }
    QColor textColor() const { return m_textColor; }
    bool simulated() const { return m_simulated; }

signals:
    void mediaChanged();
    void positionMsChanged();
    void coverUrlChanged();
    void lyricsChanged();
    void currentLineChanged();
    void lyricScrollChanged();
    void spectrumChanged();
    void paletteChanged();
    void simulatedChanged();

private slots:
    void onSnapshot(const MediaSnapshot &snapshot);
    void onCover(const QString &key, const QByteArray &imageData);
    void onBands(const QVector<qreal> &bands);
    void onClockTick(qint64 positionMs);
    void onLyricsChanged();
    void onPalette(const QColor &accent, const QColor &bg1, const QColor &bg2,
                   const QColor &text);
    void onSimulatedChanged(bool simulated);
    void onConfigChanged(const QString &key);

private:
    void applyAudioConfig();

    ConfigStore *m_config = nullptr;
    CoverImageProvider *m_coverProvider = nullptr;

    MediaSessionWorker *m_media = nullptr;
    AudioCaptureWorker *m_audio = nullptr;
    DeviceMonitor *m_device = nullptr;
    PlaybackClock *m_clock = nullptr;
    LyricsEngine *m_lyrics = nullptr;
    ThemeEngine *m_theme = nullptr;
    SpectrumProvider *m_spectrumProvider = nullptr;

    bool m_hasSession = false;
    bool m_playing = false;
    QString m_title;
    QString m_artist;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    QUrl m_coverUrl;
    QString m_expectedCoverKey;
    QStringList m_lyricTexts;
    int m_currentLine = -1;
    qreal m_lyricScroll = -1;
    bool m_noLyrics = true;
    QVector<qreal> m_spectrum;
    QColor m_accent { 79, 140, 255 };
    QColor m_bg1 { 12, 18, 34 };
    QColor m_bg2 { 24, 16, 40 };
    QColor m_textColor { 238, 242, 255 };
    bool m_simulated = false;
};
