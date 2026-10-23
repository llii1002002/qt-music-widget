#pragma once

#include <QObject>
#include <QVariant>
#include <QStringList>
#include <QTimer>
#include <QSettings>

/**
 * Persistent configuration (QSettings/INI).
 * Validated, debounced persistence, change notification.
 * Thread: main thread only. Workers receive plain values via atomics/setters.
 */
class ConfigStore : public QObject
{
    Q_OBJECT
public:
    explicit ConfigStore(QObject *parent = nullptr);
    ~ConfigStore() override;

    Q_INVOKABLE QVariant value(const QString &key) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &v);

    bool boolValue(const QString &key) const { return value(key).toBool(); }
    int intValue(const QString &key) const { return value(key).toInt(); }
    qreal realValue(const QString &key) const { return value(key).toReal(); }
    QString stringValue(const QString &key) const { return value(key).toString(); }
    QStringList stringListValue(const QString &key) const { return value(key).toStringList(); }

    void syncNow();
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE QVariantMap defaults() const { return m_defaults; }

signals:
    /** Emitted (after validation) whenever a key changes. */
    void changed(const QString &key);

private:
    QVariant validate(const QString &key, const QVariant &v) const;
    void scheduleSave();

    QSettings m_settings;
    QVariantMap m_defaults;
    QTimer m_saveTimer;
};

namespace CfgKey {
    // Window
    constexpr auto WindowClickThrough = "window.clickThrough";
    constexpr auto WindowBgOpacity    = "window.bgOpacity";
    // Lyrics
    constexpr auto LyricFontFamily = "lyric.fontFamily";
    constexpr auto LyricFontSize   = "lyric.fontSize";
    constexpr auto LyricAlign      = "lyric.align";
    constexpr auto LyricLineHeight = "lyric.lineHeight";
    constexpr auto LyricOffsetX    = "lyric.offsetX";
    constexpr auto LyricOffsetY    = "lyric.offsetY";
    constexpr auto LyricOpacity    = "lyric.opacity";
    constexpr auto LyricFolders    = "lyrics.folders";
    // Cover
    constexpr auto CoverSize   = "cover.size";
    constexpr auto CoverRotate = "cover.rotate";
    constexpr auto CoverVinyl  = "cover.vinyl";
    constexpr auto CoverOffsetX = "cover.offsetX";
    constexpr auto CoverOffsetY = "cover.offsetY";
    // Spectrum
    constexpr auto SpectrumStyle       = "spectrum.style";
    constexpr auto SpectrumMode        = "spectrum.mode";
    constexpr auto SpectrumBars        = "spectrum.bars";
    constexpr auto SpectrumFps         = "spectrum.fps";
    constexpr auto SpectrumSensitivity = "spectrum.sensitivity";
    constexpr auto SpectrumFollowTheme = "spectrum.followTheme";
    constexpr auto SpectrumColor       = "spectrum.color";
    constexpr auto SpectrumOffsetY     = "spectrum.offsetY";
    constexpr auto SpectrumMirrorOffset = "spectrum.mirrorOffset";
    // Theme
    constexpr auto ThemeAuto   = "theme.auto";
    constexpr auto ThemeAccent = "theme.accent";
    // Lyrics extras
    constexpr auto LyricScale  = "lyric.scale";
    // Performance
    constexpr auto PerfFftSize      = "perf.fftSize";
    constexpr auto PerfReduceMotion = "perf.reduceMotion";
    // Media
    constexpr auto MediaOnlyCloudMusic = "media.onlyCloudMusic";
}
