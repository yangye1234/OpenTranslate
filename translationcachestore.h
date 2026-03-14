#ifndef TRANSLATIONCACHESTORE_H
#define TRANSLATIONCACHESTORE_H

#include <optional>

#include <QString>

class QJsonArray;

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
    int findIndex(const QJsonArray &entries,
                  const QString &provider,
                  const QString &from,
                  const QString &to,
                  const QString &sourceNorm) const;

private:
    QString m_cacheFilePath;
};

#endif // TRANSLATIONCACHESTORE_H
