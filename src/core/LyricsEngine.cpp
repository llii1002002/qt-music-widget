#include "LyricsEngine.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QThreadPool>
#include <QtConcurrent>
#include <QPointer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QThread>
#include <algorithm>

namespace {

QString normalize(const QString &s)
{
    QString out = s.toLower();
    // Drop "(feat. ...)", "- xxx version" style suffixes and punctuation.
    // Match half/full-width brackets and 【】 with their content.
    static const QRegularExpression brackets(
        QStringLiteral("[[\\(\\x{FF08}\\x{3010}].*?[])\\)\\x{FF09}\\x{3011}]"));
    out.remove(brackets);
    static const QRegularExpression nonWord(QStringLiteral("[^0-9a-z\\x{4e00}-\\x{9fa5}]"));
    out.remove(nonWord);
    return out;
}

int scoreFile(const QString &fileBase, const QString &titleNorm, const QString &artistNorm)
{
    const QString f = normalize(fileBase);
    if (f.isEmpty() || titleNorm.isEmpty())
        return 0;
    if (f == titleNorm)
        return 3;
    if (!artistNorm.isEmpty() && f == artistNorm + titleNorm)
        return 4;
    if (f.contains(titleNorm) && (artistNorm.isEmpty() || f.contains(artistNorm)))
        return 2;
    return 0;
}

} // namespace

LyricsEngine::LyricsEngine(ConfigStore *config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{}

LyricsEngine::~LyricsEngine()
{
    abortOnline();
}

void LyricsEngine::setSong(const QString &title, const QString &artist)
{
    const int token = m_token.fetchAndAddAcquire(1) + 1;
    abortOnline();

    if (title.trimmed().isEmpty()) {
        applyResult(token, {});
        return;
    }

    const QStringList folders = m_config->stringListValue(CfgKey::LyricFolders);
    const QString titleCopy = title;
    const QString artistCopy = artist;

    QtConcurrent::run([this, token, folders, titleCopy, artistCopy]() {
        const QString titleNorm = normalize(titleCopy);
        const QString artistNorm = normalize(artistCopy);

        QString bestPath;
        int bestScore = 0;

        for (const QString &folder : folders) {
            QDir dir(folder);
            if (!dir.exists())
                continue;
            QDirIterator it(folder, QStringList { QStringLiteral("*.lrc") },
                            QDir::Files, QDirIterator::Subdirectories);
            int visited = 0;
            while (it.hasNext() && visited < 4000) {
                const QString path = it.next();
                ++visited;
                const int score = scoreFile(QFileInfo(path).completeBaseName(),
                                            titleNorm, artistNorm);
                if (score > bestScore) {
                    bestScore = score;
                    bestPath = path;
                }
            }
        }

        QVector<LyricLine> result;
        if (!bestPath.isEmpty()) {
            QFile f(bestPath);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text))
                result = parseLrc(f.readAll());
        }

        if (m_token.loadAcquire() != token)
            return; // A newer song superseded this lookup.

        QPointer<LyricsEngine> guard(this);
        const bool found = !result.isEmpty();
        QMetaObject::invokeMethod(this, [guard, token, result = std::move(result),
                                         titleCopy, artistCopy, found]() mutable {
            if (!guard || guard->m_token.loadAcquire() != token)
                return;
            if (found)
                guard->applyResult(token, std::move(result));
            else
                guard->startOnlineLookup(token, titleCopy, artistCopy);
        }, Qt::QueuedConnection);
    });
}

QVector<LyricLine> LyricsEngine::parseLrc(const QByteArray &raw)
{
    QString text = QString::fromUtf8(raw);
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);

    static const QRegularExpression tagRe(
        QStringLiteral("\\[(\\d{1,2}):(\\d{1,2})(?:[.:](\\d{1,3}))?\\]"));

    QVector<LyricLine> lines;
    const QStringList rawLines = text.split(QRegularExpression(QStringLiteral("[\r\n]+")),
                                            Qt::SkipEmptyParts);
    for (const QString &rawLine : rawLines) {
        QList<qint64> stamps;
        auto it = tagRe.globalMatch(rawLine);
        int lastEnd = 0;
        while (it.hasNext()) {
            const auto m = it.next();
            const qint64 min = m.captured(1).toLongLong();
            const qint64 sec = m.captured(2).toLongLong();
            QString frac = m.captured(3);
            while (frac.size() < 3)
                frac += QLatin1Char('0');
            const qint64 ms = (min * 60 + sec) * 1000 + frac.toLongLong();
            stamps << ms;
            lastEnd = m.capturedEnd();
        }
        if (stamps.isEmpty())
            continue;
        QString content = rawLine.mid(lastEnd).trimmed();
        for (qint64 ms : qAsConst(stamps))
            lines.push_back({ ms, content });
    }
    std::sort(lines.begin(), lines.end(),
              [](const LyricLine &a, const LyricLine &b) { return a.ms < b.ms; });
    return lines;
}

qreal LyricsEngine::scrollAt(qint64 positionMs, int *currentLine) const
{
    if (m_lines.isEmpty()) {
        if (currentLine)
            *currentLine = -1;
        return -1;
    }
    int idx = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        if (m_lines[i].ms <= positionMs)
            idx = i;
        else
            break;
    }
    if (idx < 0) {
        if (currentLine)
            *currentLine = -1;
        return -1.0;
    }
    if (currentLine)
        *currentLine = idx;
    const qint64 t0 = m_lines[idx].ms;
    const qint64 t1 = (idx + 1 < m_lines.size()) ? m_lines[idx + 1].ms : t0 + 5000;
    const qreal frac = (t1 > t0) ? qreal(positionMs - t0) / qreal(t1 - t0) : 0.0;
    return idx + qBound(0.0, frac, 1.0);
}

void LyricsEngine::trackReply(QNetworkReply *reply)
{
    m_replies.append(reply);
    // 6s hard timeout; abort triggers finished with OperationCanceledError.
    QTimer::singleShot(6000, reply, [reply]() { reply->abort(); });
    connect(reply, &QNetworkReply::finished, reply, [this, reply]() {
        m_replies.removeAll(reply);
        reply->deleteLater();
    });
}

void LyricsEngine::abortOnline()
{
    const auto replies = m_replies;
    for (QNetworkReply *r : replies) {
        QObject::disconnect(r, &QNetworkReply::finished, nullptr, nullptr);
        r->abort();
        r->deleteLater();
    }
    m_replies.clear();
}

static QNetworkRequest buildApiRequest(const QUrl &url)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                                 "AppleWebKit/537.36 (KHTML, like Gecko) "
                                 "Chrome/120.0 Safari/537.36"));
    req.setRawHeader("Referer", "https://music.163.com/");
    req.setRawHeader("Cookie", "appver=2.0.2; os=pc");
    return req;
}

void LyricsEngine::startOnlineLookup(int token, const QString &title, const QString &artist)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, token, title, artist]() {
            if (m_token.loadAcquire() == token)
                startOnlineLookup(token, title, artist);
        }, Qt::QueuedConnection);
        return;
    }

    if (!m_nam)
        m_nam = new QNetworkAccessManager(this);

    QString query = title;
    if (!artist.trimmed().isEmpty())
        query += QLatin1Char(' ') + artist;

    QUrl url(QStringLiteral("https://music.163.com/api/search/get/web"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("s"), query);
    q.addQueryItem(QStringLiteral("type"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("8"));
    q.addQueryItem(QStringLiteral("offset"), QStringLiteral("0"));
    url.setQuery(q);

    QNetworkReply *reply = m_nam->get(buildApiRequest(url));
    trackReply(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, token, title, artist]() {
        handleSearchReply(reply, token, title, artist);
    });
}

void LyricsEngine::handleSearchReply(QNetworkReply *reply, int token,
                                     const QString &title, const QString &artist)
{
    if (m_token.loadAcquire() != token)
        return;
    if (reply->error() != QNetworkReply::NoError) {
        applyResult(token, {});
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    const QJsonArray songs = doc.object()
                                 .value(QStringLiteral("result")).toObject()
                                 .value(QStringLiteral("songs")).toArray();

    const QString wantTitle = normalize(title);
    const QString wantArtist = normalize(artist);
    long long chosenId = 0;
    int chosenScore = -1;
    for (const QJsonValue &v : songs) {
        const QJsonObject song = v.toObject();
        const QString name = song.value(QStringLiteral("name")).toString();
        const QString nameNorm = normalize(name);
        int score = 0;
        if (nameNorm == wantTitle)
            score += 10;
        else if (!wantTitle.isEmpty() && nameNorm.contains(wantTitle))
            score += 5;
        QStringList artists;
        const QJsonArray artistArr = song.value(QStringLiteral("artists")).toArray();
        for (const QJsonValue &a : artistArr)
            artists << normalize(a.toObject().value(QStringLiteral("name")).toString());
        if (!wantArtist.isEmpty() && artists.contains(wantArtist))
            score += 4;
        else if (!wantArtist.isEmpty() &&
                 std::any_of(artists.begin(), artists.end(),
                             [&](const QString &a) { return a.contains(wantArtist); }))
            score += 2;
        if (score > chosenScore) {
            chosenScore = score;
            chosenId = static_cast<long long>(song.value(QStringLiteral("id")).toDouble());
        }
    }

    if (chosenId == 0) {
        applyResult(token, {});
        return;
    }

    QUrl url(QStringLiteral("https://music.163.com/api/song/lyric"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("os"), QStringLiteral("pc"));
    q.addQueryItem(QStringLiteral("id"), QString::number(chosenId));
    q.addQueryItem(QStringLiteral("lv"), QStringLiteral("-1"));
    q.addQueryItem(QStringLiteral("kv"), QStringLiteral("-1"));
    q.addQueryItem(QStringLiteral("tv"), QStringLiteral("-1"));
    url.setQuery(q);

    QNetworkReply *lyricReply = m_nam->get(buildApiRequest(url));
    trackReply(lyricReply);
    connect(lyricReply, &QNetworkReply::finished, this, [this, lyricReply, token]() {
        handleLyricReply(lyricReply, token);
    });
}

void LyricsEngine::handleLyricReply(QNetworkReply *reply, int token)
{
    if (m_token.loadAcquire() != token)
        return;
    if (reply->error() != QNetworkReply::NoError) {
        applyResult(token, {});
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    const QString lrc = doc.object()
                            .value(QStringLiteral("lrc")).toObject()
                            .value(QStringLiteral("lyric")).toString();
    if (lrc.isEmpty()) {
        applyResult(token, {});
        return;
    }

    QVector<LyricLine> lines = parseLrc(lrc.toUtf8());
    applyResult(token, std::move(lines));
}

void LyricsEngine::applyResult(int token, QVector<LyricLine> lines)
{
    if (m_token.loadAcquire() != token)
        return;
    m_lines = std::move(lines);
    emit lyricsChanged();
}
