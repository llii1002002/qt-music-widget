#include "ThemeEngine.h"

#include <QtConcurrent>
#include <QPointer>
#include <QHash>
#include <cmath>

ThemeEngine::ThemeEngine(ConfigStore *config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{
    m_palette = derive(QColor(m_config->stringValue(CfgKey::ThemeAccent)));
}

void ThemeEngine::analyze(const QImage &cover)
{
    m_lastCover = cover;

    if (!m_config->boolValue(CfgKey::ThemeAuto) || cover.isNull()) {
        reapply();
        return;
    }

    QImage small = cover.scaled(56, 56, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                       .convertToFormat(QImage::Format_RGB32);

    QPointer<ThemeEngine> guard(this);
    QtConcurrent::run([guard, small]() {
        const QColor accent = extractAccent(small);
        const ThemePalette pal = derive(accent);
        if (!guard)
            return;
        QMetaObject::invokeMethod(guard.data(), [guard, pal]() {
            if (guard)
                const_cast<ThemeEngine *>(guard.data())->publish(pal);
        }, Qt::QueuedConnection);
    });
}

void ThemeEngine::reapply()
{
    ThemePalette pal;
    if (m_config->boolValue(CfgKey::ThemeAuto) && !m_lastCover.isNull()) {
        pal = derive(extractAccent(m_lastCover.scaled(
            56, 56, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
    } else {
        pal = derive(QColor(m_config->stringValue(CfgKey::ThemeAccent)));
    }
    publish(pal);
}

QColor ThemeEngine::extractAccent(const QImage &img)
{
    // Quantize to 4 bits/channel and weight buckets by "colorfulness*brightness".
    struct Bucket { int count = 0; double r = 0, g = 0, b = 0; };
    QHash<quint32, Bucket> buckets;

    for (int y = 0; y < img.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const QRgb c = line[x];
            const int r = qRed(c), g = qGreen(c), b = qBlue(c);
            const int mx = qMax(r, qMax(g, b));
            const int mn = qMin(r, qMin(g, b));
            const int sat = mx - mn;
            if (sat < 24 || mx < 40 || mx > 245)
                continue; // ignore greys, near-black and near-white
            const quint32 key = (quint32(r & 0xF0) << 16) | (quint32(g & 0xF0) << 8) | (b & 0xF0);
            Bucket &bk = buckets[key];
            const double w = 1.0 + sat / 255.0;
            bk.count++;
            bk.r += r * w;
            bk.g += g * w;
            bk.b += b * w;
        }
    }
    if (buckets.isEmpty())
        return QColor(79, 140, 255);

    // Average the top few weighted buckets for stability.
    QList<Bucket> top = buckets.values();
    std::sort(top.begin(), top.end(), [](const Bucket &a, const Bucket &b) {
        return a.count > b.count;
    });
    double r = 0, g = 0, b = 0;
    int n = qMin(4, top.size());
    int total = 0;
    for (int i = 0; i < n; ++i) {
        const int weight = qMax(1, top[i].count);
        r += top[i].r;
        g += top[i].g;
        b += top[i].b;
        total += weight;
    }
    return QColor(int(r / total), int(g / total), int(b / total));
}

ThemePalette ThemeEngine::derive(const QColor &accentIn)
{
    QColor a = accentIn.isValid() ? accentIn : QColor(79, 140, 255);
    a = a.toHsl();
    const qreal h = a.hslHueF() < 0 ? 0.62 : a.hslHueF();
    const qreal s = qMax<qreal>(0.35, a.hslSaturationF());

    ThemePalette pal;
    pal.accent = QColor::fromHslF(h, s, 0.62);
    pal.bg1 = QColor::fromHslF(h, qBound(0.1, s * 0.55, 1.0), 0.09);
    const qreal h2 = std::fmod(h + 0.08, 1.0);
    pal.bg2 = QColor::fromHslF(h2, qBound(0.1, s * 0.5, 1.0), 0.13);
    pal.text = QColor(238, 242, 255);
    return pal;
}

void ThemeEngine::publish(const ThemePalette &p)
{
    m_palette = p;
    emit paletteReady(p.accent, p.bg1, p.bg2, p.text);
}
