#include "deepltranslatorservice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTimer>
#include <QUrlQuery>

namespace {
TranslationResult makeDeepLResult(bool success,
                                  const QString &from,
                                  const QString &to,
                                  const QString &text,
                                  const QString &error)
{
    TranslationResult result;
    result.success = success;
    result.provider = "deepl";
    result.sourceLanguage = from;
    result.targetLanguage = to;
    result.translatedText = text;
    result.errorMessage = error;
    return result;
}
}

DeepLTranslatorService::DeepLTranslatorService(QObject *parent)
    : TranslatorService(parent)
{
    connect(&m_network, &QNetworkAccessManager::finished,
            this, &DeepLTranslatorService::onReplyFinished);
}

void DeepLTranslatorService::setConfig(const AppConfig &config)
{
    m_config = config.deepL;
}

void DeepLTranslatorService::translate(const QString &text, const QString &from, const QString &to)
{
    if (!m_config.enabled) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeDeepLResult(false, from, to, QString(), "DeepL translator is disabled in settings."));
        });
        return;
    }
    if (m_config.authKey.trimmed().isEmpty()) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeDeepLResult(false, from, to, QString(), "Missing DeepL Auth Key."));
        });
        return;
    }

    const QString targetCode = deepLLanguageCode(to);
    if (targetCode.isEmpty() || targetCode == "AUTO") {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeDeepLResult(false, from, to, QString(), "DeepL target language is not supported."));
        });
        return;
    }

    m_pendingFrom = from;
    m_pendingTo = to;

    QUrlQuery form;
    form.addQueryItem("auth_key", m_config.authKey);
    form.addQueryItem("text", text);
    form.addQueryItem("target_lang", targetCode);
    const QString sourceCode = deepLLanguageCode(from);
    if (!sourceCode.isEmpty() && sourceCode != "AUTO") {
        form.addQueryItem("source_lang", sourceCode);
    }

    const QUrl url(m_config.baseUrl.trimmed().isEmpty()
                       ? QStringLiteral("https://api-free.deepl.com/v2/translate")
                       : m_config.baseUrl.trimmed());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    m_network.post(request, form.toString(QUrl::FullyEncoded).toUtf8());
}

void DeepLTranslatorService::onReplyFinished(QNetworkReply *reply)
{
    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorString = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError) {
        emit translationFinished(makeDeepLResult(false, m_pendingFrom, m_pendingTo, QString(), "Network error: " + networkErrorString));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        emit translationFinished(makeDeepLResult(false, m_pendingFrom, m_pendingTo, QString(), "Invalid response from DeepL."));
        return;
    }

    const QJsonArray translations = doc.object().value("translations").toArray();
    if (translations.isEmpty()) {
        emit translationFinished(makeDeepLResult(false, m_pendingFrom, m_pendingTo, QString(), "DeepL returned no translation."));
        return;
    }

    const QJsonObject item = translations.first().toObject();
    const QString translated = item.value("text").toString().trimmed();
    if (translated.isEmpty()) {
        emit translationFinished(makeDeepLResult(false, m_pendingFrom, m_pendingTo, QString(), "DeepL returned an empty result."));
        return;
    }

    TranslationResult result = makeDeepLResult(true, m_pendingFrom, m_pendingTo, translated, QString());
    const QString detected = item.value("detected_source_language").toString().toLower();
    if (!detected.isEmpty()) {
        result.sourceLanguage = detected;
    }
    emit translationFinished(result);
}

QString DeepLTranslatorService::deepLLanguageCode(const QString &language) const
{
    const QString lang = language.trimmed().toLower();
    if (lang.isEmpty() || lang == "auto") return "AUTO";
    if (lang == "zh" || lang == "zh-cn" || lang == "zh-hans") return "ZH";
    if (lang == "zh-tw" || lang == "zh-hant") return "ZH-HANT";
    if (lang == "en") return "EN";
    if (lang == "jp" || lang == "ja") return "JA";
    if (lang == "kor" || lang == "ko") return "KO";
    return lang.toUpper();
}
