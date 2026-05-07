#include "dictionarytranslatorservice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

namespace {
TranslationResult makeDictionaryResult(bool success,
                                       const QString &from,
                                       const QString &to,
                                       const QString &text,
                                       const QString &phonetic,
                                       const QString &audioUrl,
                                       const QString &error)
{
    TranslationResult result;
    result.success = success;
    result.provider = "dictionary";
    result.sourceLanguage = from;
    result.targetLanguage = to;
    result.translatedText = text;
    result.phoneticText = phonetic;
    result.audioUrl = audioUrl;
    result.errorMessage = error;
    return result;
}
}

DictionaryTranslatorService::DictionaryTranslatorService(QObject *parent)
    : TranslatorService(parent)
{
    connect(&m_network, &QNetworkAccessManager::finished,
            this, &DictionaryTranslatorService::onReplyFinished);
}

void DictionaryTranslatorService::setConfig(const AppConfig &config)
{
    m_config = config.dictionary;
}

void DictionaryTranslatorService::translate(const QString &text, const QString &from, const QString &to)
{
    Q_UNUSED(to);
    if (!m_config.enabled) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeDictionaryResult(false, from, to, QString(), QString(), QString(), "Dictionary provider is disabled in settings."));
        });
        return;
    }

    const QString word = text.trimmed();
    if (word.isEmpty() || word.contains(QRegularExpression("\\s"))) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeDictionaryResult(false, from, to, QString(), QString(), QString(), "Dictionary provider supports single words only."));
        });
        return;
    }

    m_pendingFrom = from;
    m_pendingTo = to;

    QUrl url("https://api.dictionaryapi.dev/api/v2/entries/en/" + QUrl::toPercentEncoding(word));
    m_network.get(QNetworkRequest(url));
}

void DictionaryTranslatorService::onReplyFinished(QNetworkReply *reply)
{
    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorString = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError) {
        emit translationFinished(makeDictionaryResult(false, m_pendingFrom, m_pendingTo, QString(), QString(), QString(), "Network error: " + networkErrorString));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isArray() || doc.array().isEmpty()) {
        emit translationFinished(makeDictionaryResult(false, m_pendingFrom, m_pendingTo, QString(), QString(), QString(), "Dictionary returned no result."));
        return;
    }

    const QJsonObject entry = doc.array().first().toObject();
    QString phonetic = entry.value("phonetic").toString();
    if (phonetic.isEmpty()) {
        const QJsonArray phonetics = entry.value("phonetics").toArray();
        for (const QJsonValue &value : phonetics) {
            phonetic = value.toObject().value("text").toString();
            if (!phonetic.isEmpty()) {
                break;
            }
        }
    }

    QString audioUrl;
    const QJsonArray phonetics = entry.value("phonetics").toArray();
    for (const QJsonValue &value : phonetics) {
        audioUrl = value.toObject().value("audio").toString().trimmed();
        if (!audioUrl.isEmpty()) {
            break;
        }
    }

    QStringList lines;
    const QJsonArray meanings = entry.value("meanings").toArray();
    for (const QJsonValue &meaningValue : meanings) {
        const QJsonObject meaning = meaningValue.toObject();
        const QString part = meaning.value("partOfSpeech").toString();
        const QJsonArray definitions = meaning.value("definitions").toArray();
        if (definitions.isEmpty()) {
            continue;
        }
        const QString definition = definitions.first().toObject().value("definition").toString();
        if (!definition.isEmpty()) {
            lines << (part.isEmpty() ? definition : QString("%1. %2").arg(part, definition));
        }
    }

    if (lines.isEmpty()) {
        emit translationFinished(makeDictionaryResult(false, m_pendingFrom, m_pendingTo, QString(), phonetic, audioUrl, "Dictionary returned no definitions."));
        return;
    }

    emit translationFinished(makeDictionaryResult(true, m_pendingFrom, m_pendingTo, lines.join("\n"), phonetic, audioUrl, QString()));
}
