#include "openaitranslatorservice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTimer>

namespace {
TranslationResult makeOpenAIResult(bool success,
                                   const QString &from,
                                   const QString &to,
                                   const QString &text,
                                   const QString &error)
{
    TranslationResult result;
    result.success = success;
    result.provider = "openai";
    result.sourceLanguage = from;
    result.targetLanguage = to;
    result.translatedText = text;
    result.errorMessage = error;
    return result;
}
}

OpenAITranslatorService::OpenAITranslatorService(QObject *parent)
    : TranslatorService(parent)
{
    connect(&m_network, &QNetworkAccessManager::finished,
            this, &OpenAITranslatorService::onReplyFinished);
}

void OpenAITranslatorService::setConfig(const AppConfig &config)
{
    m_config = config.generic;
}

void OpenAITranslatorService::translate(const QString &text, const QString &from, const QString &to)
{
    if (!m_config.enabled) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeOpenAIResult(false, from, to, QString(), "OpenAI-compatible translator is disabled in settings."));
        });
        return;
    }
    if (m_config.baseUrl.trimmed().isEmpty() || m_config.model.trimmed().isEmpty() || m_config.apiKey.trimmed().isEmpty()) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeOpenAIResult(false, from, to, QString(), "Missing OpenAI-compatible Base URL, model, or API key."));
        });
        return;
    }

    m_pendingFrom = from;
    m_pendingTo = to;

    QJsonArray messages;
    messages.append(QJsonObject{
        {"role", "system"},
        {"content", "You are a precise translation engine. Return only the translated text."}
    });
    messages.append(QJsonObject{
        {"role", "user"},
        {"content", promptFor(text, from, to)}
    });

    QJsonObject body;
    body.insert("model", m_config.model);
    body.insert("messages", messages);
    body.insert("temperature", 0.2);

    QNetworkRequest request(endpointUrl());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_config.apiKey).toUtf8());
    m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void OpenAITranslatorService::onReplyFinished(QNetworkReply *reply)
{
    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorString = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError) {
        emit translationFinished(makeOpenAIResult(false, m_pendingFrom, m_pendingTo, QString(), "Network error: " + networkErrorString));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        emit translationFinished(makeOpenAIResult(false, m_pendingFrom, m_pendingTo, QString(), "Invalid response from OpenAI-compatible service."));
        return;
    }

    const QJsonObject object = doc.object();
    if (object.contains("error")) {
        const QJsonObject error = object.value("error").toObject();
        emit translationFinished(makeOpenAIResult(false, m_pendingFrom, m_pendingTo, QString(),
                                                 error.value("message").toString("OpenAI-compatible service returned an error.")));
        return;
    }

    const QJsonArray choices = object.value("choices").toArray();
    if (choices.isEmpty()) {
        emit translationFinished(makeOpenAIResult(false, m_pendingFrom, m_pendingTo, QString(), "OpenAI-compatible service returned no choices."));
        return;
    }

    const QJsonObject message = choices.first().toObject().value("message").toObject();
    const QString content = message.value("content").toString().trimmed();
    if (content.isEmpty()) {
        emit translationFinished(makeOpenAIResult(false, m_pendingFrom, m_pendingTo, QString(), "OpenAI-compatible service returned an empty result."));
        return;
    }

    emit translationFinished(makeOpenAIResult(true, m_pendingFrom, m_pendingTo, content, QString()));
}

QUrl OpenAITranslatorService::endpointUrl() const
{
    QString url = m_config.baseUrl.trimmed();
    while (url.endsWith('/')) {
        url.chop(1);
    }
    if (!url.endsWith("/chat/completions")) {
        url += "/chat/completions";
    }
    return QUrl(url);
}

QString OpenAITranslatorService::promptFor(const QString &text, const QString &from, const QString &to) const
{
    QString prompt = m_config.promptTemplate.trimmed();
    if (prompt.isEmpty()) {
        prompt = "Translate from {from} to {to}:\n{text}";
    }
    prompt.replace("{from}", from);
    prompt.replace("{to}", to);
    prompt.replace("{text}", text);
    return prompt;
}
