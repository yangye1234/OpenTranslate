#include "baidutranslatorservice.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTimer>
#include <QUrlQuery>

namespace {
TranslationResult makeResult(bool success,
                             const QString &from,
                             const QString &to,
                             const QString &translatedText,
                             const QString &errorMessage)
{
    TranslationResult result;
    result.success = success;
    result.provider = "baidu";
    result.sourceLanguage = from;
    result.targetLanguage = to;
    result.translatedText = translatedText;
    result.errorMessage = errorMessage;
    return result;
}
}

BaiduTranslatorService::BaiduTranslatorService(QObject *parent)
    : TranslatorService(parent)
{
    connect(&m_network, &QNetworkAccessManager::finished,
            this, &BaiduTranslatorService::onReplyFinished);
}

void BaiduTranslatorService::setConfig(const AppConfig &config)
{
    m_baiduConfig = config.baidu;
}

void BaiduTranslatorService::translate(const QString &text, const QString &from, const QString &to)
{
    if (!m_baiduConfig.enabled) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeResult(false, from, to, QString(), "Baidu translator is disabled in settings."));
        });
        return;
    }

    if (m_baiduConfig.appId.isEmpty() || m_baiduConfig.appKey.isEmpty()) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeResult(false, from, to, QString(), "Missing Baidu AppID or AppKey."));
        });
        return;
    }

    m_pendingFrom = from;
    m_pendingTo = to;

    const QString salt = QString::number(QDateTime::currentMSecsSinceEpoch());
    const QString sign = generateSign(m_baiduConfig.appId, text, salt, m_baiduConfig.appKey);

    QUrl url("https://fanyi-api.baidu.com/api/trans/vip/translate");
    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", from);
    query.addQueryItem("to", to);
    query.addQueryItem("appid", m_baiduConfig.appId);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);
    url.setQuery(query);

    QNetworkRequest request(url);
    m_network.get(request);
}

void BaiduTranslatorService::onReplyFinished(QNetworkReply *reply)
{
    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorString = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError) {
        emit translationFinished(makeResult(false, m_pendingFrom, m_pendingTo, QString(), "Network error: " + networkErrorString));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        emit translationFinished(makeResult(false, m_pendingFrom, m_pendingTo, QString(), "Invalid response from translation service."));
        return;
    }

    const QJsonObject obj = doc.object();
    if (obj.contains("error_code")) {
        const QString errorCode = obj.value("error_code").toString();
        const QString errorMsg = obj.value("error_msg").toString("Unknown error");
        emit translationFinished(makeResult(false, m_pendingFrom, m_pendingTo, QString(), QString("Baidu error %1: %2").arg(errorCode, errorMsg)));
        return;
    }

    const QJsonArray results = obj.value("trans_result").toArray();
    QStringList lines;
    for (const QJsonValue &item : results) {
        const QJsonObject result = item.toObject();
        lines << result.value("dst").toString();
    }

    if (lines.isEmpty()) {
        emit translationFinished(makeResult(false, m_pendingFrom, m_pendingTo, QString(), "No translation result returned."));
        return;
    }

    emit translationFinished(makeResult(true, m_pendingFrom, m_pendingTo, lines.join("\n"), QString()));
}

QString BaiduTranslatorService::generateSign(const QString &appId,
                                             const QString &query,
                                             const QString &salt,
                                             const QString &appKey)
{
    const QByteArray raw = (appId + query + salt + appKey).toUtf8();
    return QString(QCryptographicHash::hash(raw, QCryptographicHash::Md5).toHex());
}
