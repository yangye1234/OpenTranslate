#include "translationhistorystore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtGlobal>

TranslationHistoryStore::TranslationHistoryStore()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + "/.opentranslate";
    }
    m_historyFilePath = QDir(baseDir).filePath("translation_history.json");
}

QList<TranslationHistoryEntry> TranslationHistoryStore::entries() const
{
    QList<TranslationHistoryEntry> result;
    const QJsonArray array = loadArray();
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            result.append(fromJson(value.toObject()));
        }
    }
    return result;
}

void TranslationHistoryStore::add(const TranslationHistoryEntry &entry, int maxEntries) const
{
    if (entry.sourceText.trimmed().isEmpty() || entry.translatedText.trimmed().isEmpty()) {
        return;
    }

    QJsonArray array = loadArray();
    TranslationHistoryEntry normalized = entry;
    if (normalized.createdAt.isEmpty()) {
        normalized.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }
    array.prepend(toJson(normalized));

    const int limit = qMax(1, maxEntries);
    while (array.size() > limit) {
        array.removeLast();
    }

    saveArray(array);
}

bool TranslationHistoryStore::clear() const
{
    return saveArray({});
}

bool TranslationHistoryStore::exportTo(const QString &filePath) const
{
    if (filePath.trimmed().isEmpty()) {
        return false;
    }

    QFileInfo info(filePath);
    QDir dir(info.absolutePath());
    if (!dir.exists() && !dir.mkpath(".")) {
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(loadArray());
    file.write(doc.toJson(QJsonDocument::Indented));
    return file.commit();
}

QString TranslationHistoryStore::historyFilePath() const
{
    return m_historyFilePath;
}

QJsonArray TranslationHistoryStore::loadArray() const
{
    QFile file(m_historyFilePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        return {};
    }
    return doc.array();
}

bool TranslationHistoryStore::saveArray(const QJsonArray &array) const
{
    const QFileInfo info(m_historyFilePath);
    QDir dir(info.absolutePath());
    if (!dir.exists() && !dir.mkpath(".")) {
        return false;
    }

    QSaveFile file(m_historyFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    const QJsonDocument doc(array);
    file.write(doc.toJson(QJsonDocument::Indented));
    return file.commit();
}

QJsonObject TranslationHistoryStore::toJson(const TranslationHistoryEntry &entry) const
{
    QJsonObject object;
    object.insert("source_text", entry.sourceText);
    object.insert("translated_text", entry.translatedText);
    object.insert("phonetic_text", entry.phoneticText);
    object.insert("provider", entry.provider);
    object.insert("source_language", entry.sourceLanguage);
    object.insert("target_language", entry.targetLanguage);
    object.insert("created_at", entry.createdAt);
    return object;
}

TranslationHistoryEntry TranslationHistoryStore::fromJson(const QJsonObject &object) const
{
    TranslationHistoryEntry entry;
    entry.sourceText = object.value("source_text").toString();
    entry.translatedText = object.value("translated_text").toString();
    entry.phoneticText = object.value("phonetic_text").toString();
    entry.provider = object.value("provider").toString();
    entry.sourceLanguage = object.value("source_language").toString();
    entry.targetLanguage = object.value("target_language").toString();
    entry.createdAt = object.value("created_at").toString();
    return entry;
}
