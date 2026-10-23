#include "ConfigStore.h"

#include <QDir>
#include <QStandardPaths>
#include <QFontDatabase>
#include <cmath>

ConfigStore::ConfigStore(QObject *parent)
    : QObject(parent)
    , m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QStringLiteral("MusicDesktop"), QStringLiteral("MusicDesktop"))
{
    const QString appDataLyrics =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/lyrics");
    const QString musicDir =
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation);

    m_defaults = {
        { QLatin1String(CfgKey::WindowClickThrough), false },
        { QLatin1String(CfgKey::WindowBgOpacity), 0.18 },

        { QLatin1String(CfgKey::LyricFontFamily), QStringLiteral("Microsoft YaHei UI") },
        { QLatin1String(CfgKey::LyricFontSize), 30 },
        { QLatin1String(CfgKey::LyricAlign), QStringLiteral("center") },
        { QLatin1String(CfgKey::LyricLineHeight), 1.9 },
        { QLatin1String(CfgKey::LyricOffsetX), 0 },
        { QLatin1String(CfgKey::LyricOffsetY), -60 },
        { QLatin1String(CfgKey::LyricOpacity), 0.95 },
        { QLatin1String(CfgKey::LyricFolders), QStringList { musicDir, appDataLyrics } },

        { QLatin1String(CfgKey::CoverSize), 220 },
        { QLatin1String(CfgKey::CoverRotate), true },
        { QLatin1String(CfgKey::CoverVinyl), false },
        { QLatin1String(CfgKey::CoverOffsetX), 0 },
        { QLatin1String(CfgKey::CoverOffsetY), 200 },

        { QLatin1String(CfgKey::SpectrumStyle), QStringLiteral("bars") },
        { QLatin1String(CfgKey::SpectrumMode), QStringLiteral("normal") },
        { QLatin1String(CfgKey::SpectrumBars), 48 },
        { QLatin1String(CfgKey::SpectrumFps), 60 },
        { QLatin1String(CfgKey::SpectrumSensitivity), 1.0 },
        { QLatin1String(CfgKey::SpectrumFollowTheme), true },
        { QLatin1String(CfgKey::SpectrumColor), QStringLiteral("#6aa6ff") },
        { QLatin1String(CfgKey::SpectrumOffsetY), 0 },
        { QLatin1String(CfgKey::SpectrumMirrorOffset), 0 },

        { QLatin1String(CfgKey::ThemeAuto), true },
        { QLatin1String(CfgKey::ThemeAccent), QStringLiteral("#4f8cff") },

        { QLatin1String(CfgKey::LyricScale), 1.10 },

        { QLatin1String(CfgKey::PerfFftSize), 1024 },
        { QLatin1String(CfgKey::PerfReduceMotion), false },

        { QLatin1String(CfgKey::MediaOnlyCloudMusic), false },
    };

    // Corrupted value fallback: remove() invalid keys, keep valid ones.
    if (m_settings.status() == QSettings::FormatError)
        m_settings.clear();

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(300);
    connect(&m_saveTimer, &QTimer::timeout, this, &ConfigStore::syncNow);
}

ConfigStore::~ConfigStore()
{
    if (m_saveTimer.isActive()) {
        m_saveTimer.stop();
        syncNow();
    }
}

QVariant ConfigStore::value(const QString &key) const
{
    const QVariant def = m_defaults.value(key);
    QVariant v = m_settings.value(key, def);
    if (def.isNull())
        return v;

    // Strict bool coercion: QVariant(QString).convert(Bool) treats arbitrary
    // non-empty strings like "garbage" as true, so an edited/corrupt INI
    // could silently flip a feature on. Accept only canonical tokens.
    if (def.userType() == QMetaType::Bool) {
        if (v.userType() == QMetaType::Bool)
            return v;
        const QString s = v.toString().trimmed().toLower();
        if (s == QLatin1String("1") || s == QLatin1String("true"))
            return true;
        if (s == QLatin1String("0") || s == QLatin1String("false"))
            return false;
        return def;
    }

    if (v.userType() != def.userType()) {
        if (!v.convert(def.userType()))
            return def;
    }
    // Apply the same range/enum guards used on panel writes to externally
    // edited values (clamps corrupt numbers, normalises bad enums).
    return validate(key, v);
}

void ConfigStore::setValue(const QString &key, const QVariant &v)
{
    if (!m_defaults.contains(key))
        return;
    const QVariant validated = validate(key, v);
    if (value(key) == validated)
        return;
    m_settings.setValue(key, validated);
    emit changed(key);
    scheduleSave();
}

void ConfigStore::syncNow()
{
    m_settings.sync();
}

QVariant ConfigStore::validate(const QString &key, const QVariant &v) const
{
    const QVariant def = m_defaults.value(key);

    if (key == QLatin1String(CfgKey::WindowBgOpacity))
        return qBound(0.0, v.toReal(), 1.0);
    if (key == QLatin1String(CfgKey::LyricFontSize))
        return qBound(14, v.toInt(), 72);
    if (key == QLatin1String(CfgKey::LyricLineHeight))
        return qBound(1.2, v.toReal(), 3.5);
    if (key == QLatin1String(CfgKey::LyricOffsetX))
        return qBound(-1000, v.toInt(), 1000);
    if (key == QLatin1String(CfgKey::LyricOffsetY))
        return qBound(-1000, v.toInt(), 1000);
    if (key == QLatin1String(CfgKey::CoverOffsetX))
        return qBound(-1000, v.toInt(), 1000);
    if (key == QLatin1String(CfgKey::CoverOffsetY))
        return qBound(-1000, v.toInt(), 1000);
    if (key == QLatin1String(CfgKey::LyricOpacity))
        return qBound(0.1, v.toReal(), 1.0);
    if (key == QLatin1String(CfgKey::LyricScale))
        return qBound(1.0, v.toReal(), 1.25);
    if (key == QLatin1String(CfgKey::LyricAlign)) {
        const QString s = v.toString().toLower();
        return (s == QLatin1String("left") || s == QLatin1String("right")) ? s
                                                                            : QStringLiteral("center");
    }
    if (key == QLatin1String(CfgKey::CoverSize))
        return qBound(120, v.toInt(), 480);
    if (key == QLatin1String(CfgKey::SpectrumStyle)) {
        const QString s = v.toString().toLower();
        return s == QLatin1String("wave") ? s : QStringLiteral("bars");
    }
    if (key == QLatin1String(CfgKey::SpectrumMode)) {
        const QString s = v.toString().toLower();
        return s == QLatin1String("mirror") ? s : QStringLiteral("normal");
    }
    if (key == QLatin1String(CfgKey::SpectrumBars)) {
        const int n = v.toInt();
        return (n == 24 || n == 32 || n == 48 || n == 64) ? n : 48;
    }
    if (key == QLatin1String(CfgKey::SpectrumFps)) {
        const int n = v.toInt();
        return (n == 30 || n == 60) ? n : 60;
    }
    if (key == QLatin1String(CfgKey::SpectrumSensitivity))
        return qBound(0.3, v.toReal(), 3.0);
    if (key == QLatin1String(CfgKey::SpectrumOffsetY)) {
        const int n = v.toInt(); // NaN/non-numeric -> 0 -> within valid range
        return qBound(-20000, n, 20000); // allow full-screen travel on any display
    }
    if (key == QLatin1String(CfgKey::SpectrumMirrorOffset)) {
        const int n = v.toInt(); // NaN/non-numeric -> 0 -> within valid range
        return qBound(0, n, 50);
    }
    if (key == QLatin1String(CfgKey::PerfFftSize)) {
        const int n = v.toInt();
        return (n == 512 || n == 1024 || n == 2048) ? n : 1024;
    }
    if (key == QLatin1String(CfgKey::ThemeAccent) || key == QLatin1String(CfgKey::SpectrumColor)) {
        QString s = v.toString().trimmed();
        if (!s.startsWith(QLatin1Char('#')))
            s.prepend(QLatin1Char('#'));
        return s.size() == 7 ? s : def;
    }
    return v.userType() == def.userType() ? v : def;
}

void ConfigStore::scheduleSave()
{
    m_saveTimer.start();
}

void ConfigStore::resetToDefaults()
{
    for (auto it = m_defaults.cbegin(); it != m_defaults.cend(); ++it) {
        if (value(it.key()) != it.value())
            setValue(it.key(), it.value());
        else
            m_settings.remove(it.key());
    }
    syncNow();
}
