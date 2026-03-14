#include "translationcachestore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
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
    const int index = findBidirectionalIndex(entries, provider, from, to, sourceNorm);
    if (index < 0) {
        return std::nullopt;
    }

    const QJsonObject obj = entries.at(index).toObject();
    const QString langA = obj.value("lang_a").toString();
    const QString langB = obj.value("lang_b").toString();
    const QString textA = obj.value("text_a").toString();
    const QString textB = obj.value("text_b").toString();

    if (langA == from && langB == to
        && obj.value("text_a_norm").toString(textA.toCaseFolded()) == sourceNorm) {
        return textB.isEmpty() ? std::nullopt : std::optional<QString>(textB);
    }

    if (langB == from && langA == to
        && obj.value("text_b_norm").toString(textB.toCaseFolded()) == sourceNorm) {
        return textA.isEmpty() ? std::nullopt : std::optional<QString>(textA);
    }

    return std::nullopt;
}

void TranslationCacheStore::upsert(const QString &provider,
                                   const QString &from,
                                   const QString &to,
                                   const QString &sourceText,
                                   const QString &result) const
{
    QJsonArray entries = loadEntries();
    const QString sourceNorm = normalizedSource(sourceText);
    const QString updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    const int index = findBidirectionalIndex(entries, provider, from, to, sourceNorm);
    if (index >= 0) {
        QJsonObject obj = entries.at(index).toObject();
        const QString langA = obj.value("lang_a").toString();
        const QString langB = obj.value("lang_b").toString();
        QString textA = obj.value("text_a").toString();
        QString textB = obj.value("text_b").toString();

        if (langA == from && langB == to) {
            textA = sourceText;
            textB = result;
        } else {
            textB = sourceText;
            textA = result;
        }

        entries[index] = makeCanonicalEntry(provider, langA, textA, langB, textB, updatedAt);
    } else {
        entries.append(makeCanonicalEntry(provider, from, sourceText, to, result, updatedAt));
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

    bool changed = false;
    const QJsonArray migrated = migrateIfNeeded(doc.array(), changed);
    if (changed) {
        saveEntries(migrated);
    }
    return migrated;
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

int TranslationCacheStore::findBidirectionalIndex(const QJsonArray &entries,
                                                  const QString &provider,
                                                  const QString &from,
                                                  const QString &to,
                                                  const QString &sourceNorm) const
{
    for (int i = 0; i < entries.size(); ++i) {
        const QJsonObject obj = entries.at(i).toObject();
        if (obj.value("provider").toString() != provider) {
            continue;
        }

        const QString langA = obj.value("lang_a").toString();
        const QString langB = obj.value("lang_b").toString();
        const QString textA = obj.value("text_a").toString();
        const QString textB = obj.value("text_b").toString();
        const QString textANorm = obj.value("text_a_norm").toString(textA.toCaseFolded());
        const QString textBNorm = obj.value("text_b_norm").toString(textB.toCaseFolded());

        if ((langA == from && langB == to && textANorm == sourceNorm)
            || (langB == from && langA == to && textBNorm == sourceNorm)) {
            return i;
        }
    }
    return -1;
}

QJsonObject TranslationCacheStore::makeCanonicalEntry(const QString &provider,
                                                      const QString &langA,
                                                      const QString &textA,
                                                      const QString &langB,
                                                      const QString &textB,
                                                      const QString &updatedAt) const
{
    const QString normA = normalizedSource(textA);
    const QString normB = normalizedSource(textB);
    const QString sideA = langA + "\x1f" + normA;
    const QString sideB = langB + "\x1f" + normB;

    QJsonObject obj;
    obj.insert("provider", provider);

    if (sideA <= sideB) {
        obj.insert("lang_a", langA);
        obj.insert("text_a", textA);
        obj.insert("text_a_norm", normA);
        obj.insert("lang_b", langB);
        obj.insert("text_b", textB);
        obj.insert("text_b_norm", normB);
    } else {
        obj.insert("lang_a", langB);
        obj.insert("text_a", textB);
        obj.insert("text_a_norm", normB);
        obj.insert("lang_b", langA);
        obj.insert("text_b", textA);
        obj.insert("text_b_norm", normA);
    }

    obj.insert("updated_at", updatedAt);
    return obj;
}

QJsonObject TranslationCacheStore::canonicalizeExistingEntry(const QJsonObject &obj) const
{
    return makeCanonicalEntry(
        obj.value("provider").toString(),
        obj.value("lang_a").toString(),
        obj.value("text_a").toString(),
        obj.value("lang_b").toString(),
        obj.value("text_b").toString(),
        obj.value("updated_at").toString(QDateTime::currentDateTimeUtc().toString(Qt::ISODate)));
}

QJsonArray TranslationCacheStore::migrateIfNeeded(const QJsonArray &entries, bool &changed) const
{
    QMap<QString, QJsonObject> dedup;
    changed = false;

    for (int i = 0; i < entries.size(); ++i) {
        if (!entries.at(i).isObject()) {
            changed = true;
            continue;
        }

        const QJsonObject raw = entries.at(i).toObject();
        QJsonObject canonical;

        if (raw.contains("lang_a") && raw.contains("lang_b")
            && raw.contains("text_a") && raw.contains("text_b")) {
            canonical = canonicalizeExistingEntry(raw);
            if (canonical != raw || !raw.contains("text_a_norm") || !raw.contains("text_b_norm")) {
                changed = true;
            }
        } else if (raw.contains("from") && raw.contains("to")
                   && raw.contains("source") && raw.contains("result")) {
            canonical = makeCanonicalEntry(
                raw.value("provider").toString("baidu"),
                raw.value("from").toString(),
                raw.value("source").toString(),
                raw.value("to").toString(),
                raw.value("result").toString(),
                raw.value("updated_at").toString(QDateTime::currentDateTimeUtc().toString(Qt::ISODate)));
            changed = true;
        } else {
            changed = true;
            continue;
        }

        const QString key = entryIdentityKey(
            canonical.value("provider").toString(),
            canonical.value("lang_a").toString(),
            canonical.value("text_a_norm").toString(),
            canonical.value("lang_b").toString(),
            canonical.value("text_b_norm").toString());

        if (dedup.contains(key)) {
            changed = true;
        }
        dedup.insert(key, canonical);
    }

    QJsonArray out;
    for (auto it = dedup.cbegin(); it != dedup.cend(); ++it) {
        out.append(it.value());
    }

    if (out.size() != entries.size()) {
        changed = true;
    }
    return out;
}

QString TranslationCacheStore::entryIdentityKey(const QString &provider,
                                                const QString &langA,
                                                const QString &textANorm,
                                                const QString &langB,
                                                const QString &textBNorm) const
{
    const QString sideA = langA + "\x1f" + textANorm;
    const QString sideB = langB + "\x1f" + textBNorm;
    if (sideA <= sideB) {
        return provider + "|" + sideA + "|" + sideB;
    }
    return provider + "|" + sideB + "|" + sideA;
}
