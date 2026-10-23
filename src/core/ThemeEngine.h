#pragma once

#include <QObject>
#include <QImage>
#include <QColor>
#include "ConfigStore.h"

struct ThemePalette
{
    QColor accent { 79, 140, 255 };
    QColor bg1 { 12, 18, 34 };
    QColor bg2 { 24, 16, 40 };
    QColor text { 238, 242, 255 };
};

/** Extracts dominant cover color (worker thread) and derives the palette. */
class ThemeEngine : public QObject
{
    Q_OBJECT
public:
    explicit ThemeEngine(ConfigStore *config, QObject *parent = nullptr);

    /** Analyze a new cover; emits paletteReady later on the main thread. */
    void analyze(const QImage &cover);

    /** Re-emit the current palette (used on config changes). */
    void reapply();

    ThemePalette palette() const { return m_palette; }

signals:
    void paletteReady(const QColor &accent, const QColor &bg1, const QColor &bg2,
                      const QColor &text);

private:
    void publish(const ThemePalette &p);
    static QColor extractAccent(const QImage &img);
    static ThemePalette derive(const QColor &accent);

    ConfigStore *m_config;
    ThemePalette m_palette;
    QImage m_lastCover;
};
