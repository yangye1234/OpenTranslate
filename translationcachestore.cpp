#include "translationcachestore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

TranslationCacheStore::TranslationCacheStore()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + "/.opentranslate";
    }
    m_cacheFilePath = QDir(baseDir).filePath("translations_cache.json");
}

std::optional<QString> TranslationCacheStore::find(const QString &provider,
                                                   const QString &from,
                                                   const QString &to,
                                                   const QString &sourceText) const
{
    const QString sourceNorm = normalizedSource(sourceText);
    const QJsonArray entries = loadEntries();
    const int index = findIndex(entries, provider, from, to, sourceNorm);
    if (index < 0) {
        return std::nullopt;
    }

    const QJsonObject obj = entries.at(index).toObject();
    const QString result = obj.value("result").toString();
    if (result.isEmpty()) {
        return std::nullopt;
    }
    return result;
}

void TranslationCacheStore::upsert(const QString &provider,
                                   const QString &from,
                                   const QString &to,
                                   const QString &sourceText,
                                   const QString &result) const
{
    const QString sourceNorm = normalizedSource(sourceText);
    QJsonArray entries = loadEntries();

    QJsonObject obj;
    obj.insert("provider", provider);
    obj.insert("from", from);
    obj.insert("to", to);
    obj.insert("source", sourceText);
    obj.insert("source_norm", sourceNorm);
    obj.insert("result", result);
    obj.insert("updated_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    const int index = findIndex(entries, provider, from, to, sourceNorm);
    if (index >= 0) {
        entries[index] = obj;
    } else {
        entries.append(obj);
    }

    saveEntries(entries);
}

QString TranslationCacheStore::cacheFilePath() const
{
    return m_cacheFilePath;
}

QString TranslationCacheStore::normalizedSource(const QString &sourceText) const
{
    return sourceText.toCaseFolded();
}

QJsonArray TranslationCacheStore::loadEntries() const
{
    QFile file(m_cacheFilePath);
    if (!file.exists()) {
        return {};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        return {};
    }
    return doc.array();
}

void TranslationCacheStore::saveEntries(const QJsonArray &entries) const
{
    const QFileInfo info(m_cacheFilePath);
    QDir dir(info.absolutePath());
    if (!dir.exists() && !dir.mkpath(".")) {
        return;
    }

    QSaveFile file(m_cacheFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    const QJsonDocument doc(entries);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.commit();
}

int TranslationCacheStore::findIndex(const QJsonArray &entries,
                                     const QString &provider,
                                     const QString &from,
                                     const QString &to,
                                     const QString &sourceNorm) const
{
    for (int i = 0; i < entries.size(); ++i) {
        const QJsonObject obj = entries.at(i).toObject();
        const QString entrySourceNorm = obj.value("source_norm").toString(
            obj.value("source").toString().toCaseFolded());

        if (obj.value("provider").toString() == provider
            && obj.value("from").toString() == from
            && obj.value("to").toString() == to
            && entrySourceNorm == sourceNorm) {
            return i;
        }
    }
    return -1;
}
