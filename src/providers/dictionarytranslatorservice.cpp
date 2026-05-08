#include "dictionarytranslatorservice.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {
constexpr const char *kYoudaoDictUrl = "https://dict.youdao.com";
constexpr const char *kYoudaoWebDictKey = "Mk6hqtUp33DGGtoS63tTJbMUYjRrG1Lu";

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

QString md5Hex(const QString &text)
{
    return QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5).toHex());
}

bool isChineseLanguage(const QString &language)
{
    const QString code = language.toLower();
    return code == "zh" || code == "zh-cn" || code == "zh-tw" || code == "zh-hans" || code == "zh-hant";
}

QString youdaoForeignLanguage(const QString &from, const QString &to)
{
    auto normalizeForeign = [](const QString &language) -> QString {
        const QString code = language.toLower();
        if (code == "en" || code.startsWith("en-")) {
            return "en";
        }
        if (code == "ja" || code == "jp" || code.startsWith("ja-")) {
            return "ja";
        }
        if (code == "ko" || code == "kr" || code.startsWith("ko-")) {
            return "ko";
        }
        if (code == "fr" || code.startsWith("fr-")) {
            return "fr";
        }
        return {};
    };

    if (isChineseLanguage(from)) {
        return normalizeForeign(to);
    }
    if (isChineseLanguage(to)) {
        return normalizeForeign(from);
    }
    return normalizeForeign(from);
}

QString youdaoAudioUrl(const QString &audio, const QString &language)
{
    if (audio.trimmed().isEmpty()) {
        return {};
    }

    QString url = QString::fromLatin1(kYoudaoDictUrl) + "/dictvoice?audio=" + audio.trimmed();
    if (!language.trimmed().isEmpty()) {
        url += "&le=" + language.trimmed();
    }
    return url;
}

void appendEcResult(const QJsonObject &ec, QStringList &lines, QString &phonetic, QString &audioUrl)
{
    const QJsonObject word = ec.value("word").toObject();
    if (word.isEmpty()) {
        return;
    }

    const QString usphone = word.value("usphone").toString().trimmed();
    const QString ukphone = word.value("ukphone").toString().trimmed();
    QStringList phonetics;
    if (!usphone.isEmpty()) {
        phonetics << "US [" + usphone + "]";
    }
    if (!ukphone.isEmpty()) {
        phonetics << "UK [" + ukphone + "]";
    }
    if (!phonetics.isEmpty()) {
        phonetic = phonetics.join(" ");
    }

    audioUrl = youdaoAudioUrl(word.value("usspeech").toString(), "en");
    if (audioUrl.isEmpty()) {
        audioUrl = youdaoAudioUrl(word.value("ukspeech").toString(), "en");
    }

    const QJsonArray trs = word.value("trs").toArray();
    for (const QJsonValue &value : trs) {
        const QJsonObject item = value.toObject();
        const QString pos = item.value("pos").toString().trimmed();
        const QString tran = item.value("tran").toString().trimmed();
        if (!tran.isEmpty()) {
            lines << (pos.isEmpty() ? tran : QString("%1. %2").arg(pos, tran));
        }
    }
}

void appendCeResult(const QJsonObject &ce, QStringList &lines, QString &phonetic)
{
    const QJsonObject word = ce.value("word").toObject();
    if (word.isEmpty()) {
        return;
    }

    const QString phone = word.value("phone").toString().trimmed();
    if (!phone.isEmpty()) {
        phonetic = phone;
    }

    const QJsonArray trs = word.value("trs").toArray();
    for (const QJsonValue &value : trs) {
        if (lines.size() >= 3) {
            break;
        }
        const QJsonObject item = value.toObject();
        const QString text = item.value("#text").toString(item.value("text").toString()).trimmed();
        const QString tran = item.value("tran").toString().trimmed();
        if (!text.isEmpty() && !tran.isEmpty()) {
            lines << QString("%1: %2").arg(text, tran);
        } else if (!text.isEmpty()) {
            lines << text;
        } else if (!tran.isEmpty()) {
            lines << tran;
        }
    }
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
    if (!m_config.enabled) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeDictionaryResult(false, from, to, QString(), QString(), QString(), "Youdao dictionary is disabled in settings."));
        });
        return;
    }

    const QString query = text.trimmed();
    if (query.isEmpty()) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeDictionaryResult(false, from, to, QString(), QString(), QString(), "Translation text is empty."));
        });
        return;
    }

    const QString foreignLanguage = youdaoForeignLanguage(from, to);
    if (foreignLanguage.isEmpty()) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeDictionaryResult(false, from, to, QString(), QString(), QString(), "Youdao dictionary supports Chinese with English, Japanese, Korean, or French."));
        });
        return;
    }

    m_pendingFrom = from;
    m_pendingTo = to;

    const QString ww = query + "webdict";
    const QString time = QString::number(ww.size() % 10);
    const QString salt = md5Hex(ww);
    const QString sign = md5Hex("web" + query + time + kYoudaoWebDictKey + salt);

    QUrl url(QString::fromLatin1(kYoudaoDictUrl) + "/jsonapi_s");
    QUrlQuery requestQuery;
    requestQuery.addQueryItem("doctype", "json");
    requestQuery.addQueryItem("jsonversion", "4");
    url.setQuery(requestQuery);

    QUrlQuery body;
    body.addQueryItem("q", query);
    body.addQueryItem("le", foreignLanguage);
    body.addQueryItem("client", "web");
    body.addQueryItem("t", time);
    body.addQueryItem("sign", sign);
    body.addQueryItem("keyfrom", "webdict");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("User-Agent", "Mozilla/5.0 OpenTranslate");
    request.setRawHeader("Referer", "https://fanyi.youdao.com");
    request.setRawHeader("Cookie", "OUTFOX_SEARCH_USER_ID=1796239350@10.110.96.157;");
    m_network.post(request, body.toString(QUrl::FullyEncoded).toUtf8());
}

void DictionaryTranslatorService::onReplyFinished(QNetworkReply *reply)
{
    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorString = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError) {
        emit translationFinished(makeDictionaryResult(false, m_pendingFrom, m_pendingTo, QString(), QString(), QString(), "Youdao dictionary network error: " + networkErrorString));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        emit translationFinished(makeDictionaryResult(false, m_pendingFrom, m_pendingTo, QString(), QString(), QString(), "Youdao dictionary returned an invalid response."));
        return;
    }

    const QJsonObject root = doc.object();
    QStringList lines;
    QString phonetic;
    QString audioUrl;

    appendEcResult(root.value("ec").toObject(), lines, phonetic, audioUrl);
    appendCeResult(root.value("ce").toObject(), lines, phonetic);

    const QString fanyi = root.value("fanyi").toObject().value("tran").toString().trimmed();
    if (lines.isEmpty() && !fanyi.isEmpty()) {
        lines << fanyi;
    }

    if (lines.isEmpty()) {
        emit translationFinished(makeDictionaryResult(false, m_pendingFrom, m_pendingTo, QString(), phonetic, audioUrl, "有道词典未返回可显示结果。"));
        return;
    }

    emit translationFinished(makeDictionaryResult(true, m_pendingFrom, m_pendingTo, lines.join("\n"), phonetic, audioUrl, QString()));
}
