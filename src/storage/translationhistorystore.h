#ifndef TRANSLATIONHISTORYSTORE_H
#define TRANSLATIONHISTORYSTORE_H

#include <QList>
#include <QString>

struct TranslationHistoryEntry {
    QString sourceText;
    QString translatedText;
    QString phoneticText;
    QString provider;
    QString sourceLanguage;
    QString targetLanguage;
    QString createdAt;
};

class QJsonArray;
class QJsonObject;

class TranslationHistoryStore
{
public:
    TranslationHistoryStore();

    QList<TranslationHistoryEntry> entries() const;
    void add(const TranslationHistoryEntry &entry, int maxEntries) const;
    bool clear() const;
    bool exportTo(const QString &filePath) const;

    QString historyFilePath() const;

private:
    QJsonArray loadArray() const;
    bool saveArray(const QJsonArray &array) const;
    QJsonObject toJson(const TranslationHistoryEntry &entry) const;
    TranslationHistoryEntry fromJson(const QJsonObject &object) const;

private:
    QString m_historyFilePath;
};

#endif // TRANSLATIONHISTORYSTORE_H
