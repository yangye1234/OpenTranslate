#ifndef TRANSLATIONCACHESTORE_H
#define TRANSLATIONCACHESTORE_H

#include <optional>

#include <QString>

class QJsonArray;
class QJsonObject;

class TranslationCacheStore
{
public:
    TranslationCacheStore();

    std::optional<QString> find(const QString &provider,
                                const QString &from,
                                const QString &to,
                                const QString &sourceText) const;

    void upsert(const QString &provider,
                const QString &from,
                const QString &to,
                const QString &sourceText,
                const QString &result) const;

    QString cacheFilePath() const;

private:
    QString normalizedSource(const QString &sourceText) const;
    QJsonArray loadEntries() const;
    void saveEntries(const QJsonArray &entries) const;
    int findBidirectionalIndex(const QJsonArray &entries,
                               const QString &provider,
                               const QString &from,
                               const QString &to,
                               const QString &sourceNorm) const;
    QJsonObject makeCanonicalEntry(const QString &provider,
                                   const QString &langA,
                                   const QString &textA,
                                   const QString &langB,
                                   const QString &textB,
                                   const QString &updatedAt) const;
    QJsonObject canonicalizeExistingEntry(const QJsonObject &obj) const;
    QJsonArray migrateIfNeeded(const QJsonArray &entries, bool &changed) const;
    QString entryIdentityKey(const QString &provider,
                             const QString &langA,
                             const QString &textANorm,
                             const QString &langB,
                             const QString &textBNorm) const;

private:
    QString m_cacheFilePath;
};

#endif // TRANSLATIONCACHESTORE_H
