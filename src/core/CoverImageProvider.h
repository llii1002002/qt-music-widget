#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QHash>
#include <QMutex>

/** Serves album covers to QML via image://cover/<key>. */
class CoverImageProvider : public QQuickImageProvider
{
public:
    CoverImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {}

    void add(const QString &key, const QImage &image)
    {
        QMutexLocker locker(&m_mutex);
        m_cache.insert(key, image);
        // Bound memory: keep the 12 most recent covers.
        while (m_cache.size() > 12)
            m_cache.erase(m_cache.begin());
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        QMutexLocker locker(&m_mutex);
        // Strip the "#version" fragment appended by PlayerController to bust
        // QML Image caching; the actual cache key is everything before '#'.
        const QString actualKey = id.section(QLatin1Char('#'), 0, 0);
        QImage img = m_cache.value(actualKey);
        if (img.isNull())
            return QImage();
        if (requestedSize.isValid() && requestedSize.width() > 0)
            img = img.scaled(requestedSize, Qt::KeepAspectRatioByExpanding,
                             Qt::SmoothTransformation);
        if (size)
            *size = img.size();
        return img;
    }

private:
    QHash<QString, QImage> m_cache;
    QMutex m_mutex;
};
