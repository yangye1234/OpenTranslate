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
                                       const QString &query,
                                       const QString &from,
                                       const QString &to,
                                       const QString &text,
                                       const QString &phonetic,
                                       const QString &audioUrl,
                                       const QList<DictionaryPhonetic> &phonetics,
                                       const QList<DictionarySection> &sections,
                                       const QString &error)
{
    TranslationResult result;
    result.success = success;
    result.provider = "dictionary";
    result.queryText = query;
    result.sourceLanguage = from;
    result.targetLanguage = to;
    result.translatedText = text;
    result.phoneticText = phonetic;
    result.audioUrl = audioUrl;
    result.dictionaryPhonetics = phonetics;
    result.dictionarySections = sections;
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

QStringList jsonStringArray(const QJsonValue &value)
{
    QStringList items;
    const QJsonArray array = value.toArray();
    for (const QJsonValue &item : array) {
        const QString text = item.toString().trimmed();
        if (!text.isEmpty()) {
            items << text;
        }
    }
    return items;
}

void addSection(QList<DictionarySection> &sections, const QString &title, const QStringList &lines)
{
    QStringList cleaned;
    for (const QString &line : lines) {
        const QString text = line.trimmed();
        if (!text.isEmpty() && !cleaned.contains(text)) {
            cleaned << text;
        }
    }
    if (cleaned.isEmpty()) {
        return;
    }

    DictionarySection section;
    section.title = title;
    section.lines = cleaned;
    sections << section;
}

void appendEcResult(const QJsonObject &ec,
                    QStringList &lines,
                    QString &phonetic,
                    QString &audioUrl,
                    QList<DictionaryPhonetic> &phonetics,
                    QList<DictionarySection> &sections)
{
    const QJsonObject word = ec.value("word").toObject();
    if (word.isEmpty()) {
        return;
    }

    const QString usphone = word.value("usphone").toString().trimmed();
    const QString ukphone = word.value("ukphone").toString().trimmed();
    QStringList phoneticTexts;
    if (!usphone.isEmpty()) {
        phoneticTexts << "US [" + usphone + "]";
    }
    if (!ukphone.isEmpty()) {
        phoneticTexts << "UK [" + ukphone + "]";
    }
    if (!phoneticTexts.isEmpty()) {
        phonetic = phoneticTexts.join(" ");
    }

    const QString usAudioUrl = youdaoAudioUrl(word.value("usspeech").toString(), "en");
    const QString ukAudioUrl = youdaoAudioUrl(word.value("ukspeech").toString(), "en");
    if (!usphone.isEmpty()) {
        phonetics << DictionaryPhonetic{"US", usphone, usAudioUrl};
    }
    if (!ukphone.isEmpty()) {
        phonetics << DictionaryPhonetic{"UK", ukphone, ukAudioUrl};
    }

    audioUrl = usAudioUrl;
    if (audioUrl.isEmpty()) {
        audioUrl = ukAudioUrl;
    }

    QStringList definitionLines;
    const QJsonArray trs = word.value("trs").toArray();
    for (const QJsonValue &value : trs) {
        const QJsonObject item = value.toObject();
        const QString pos = item.value("pos").toString().trimmed();
        const QString tran = item.value("tran").toString().trimmed();
        if (!tran.isEmpty()) {
            const QString line = pos.isEmpty() ? tran : QString("%1. %2").arg(pos, tran);
            lines << line;
            definitionLines << line;
        }
    }
    addSection(sections, "definitions", definitionLines);

    QStringList exchangeLines;
    const QJsonArray wfs = word.value("wfs").toArray();
    for (const QJsonValue &value : wfs) {
        const QJsonObject wf = value.toObject().value("wf").toObject();
        const QString name = wf.value("name").toString().trimmed();
        const QString wordValue = wf.value("value").toString().trimmed();
        if (!name.isEmpty() && !wordValue.isEmpty()) {
            exchangeLines << QString("%1: %2").arg(name, wordValue);
        }
    }
    addSection(sections, "forms", exchangeLines);

    const QStringList tags = jsonStringArray(ec.value("exam_type"));
    addSection(sections, "tags", tags);
}

void appendCeResult(const QJsonObject &ce,
                    QStringList &lines,
                    QString &phonetic,
                    QList<DictionaryPhonetic> &phonetics,
                    QList<DictionarySection> &sections)
{
    const QJsonObject word = ce.value("word").toObject();
    if (word.isEmpty()) {
        return;
    }

    const QString phone = word.value("phone").toString().trimmed();
    if (!phone.isEmpty()) {
        phonetic = phone;
        phonetics << DictionaryPhonetic{"phonetic", phone, QString()};
    }

    QStringList definitionLines;
    const QJsonArray trs = word.value("trs").toArray();
    for (const QJsonValue &value : trs) {
        if (lines.size() >= 3) {
            break;
        }
        const QJsonObject item = value.toObject();
        const QString text = item.value("#text").toString(item.value("text").toString()).trimmed();
        const QString tran = item.value("tran").toString().trimmed();
        if (!text.isEmpty() && !tran.isEmpty()) {
            const QString line = QString("%1: %2").arg(text, tran);
            lines << line;
            definitionLines << line;
        } else if (!text.isEmpty()) {
            lines << text;
            definitionLines << text;
        } else if (!tran.isEmpty()) {
            lines << tran;
            definitionLines << tran;
        }
    }
    addSection(sections, "definitions", definitionLines);
}

void appendWebTranslations(const QJsonObject &root, QList<DictionarySection> &sections)
{
    const QJsonObject webTrans = root.value("web_trans").toObject();
    const QJsonArray webTranslations = webTrans.value("web-translation").toArray();
    QStringList lines;
    for (const QJsonValue &value : webTranslations) {
        if (lines.size() >= 5) {
            break;
        }
        const QJsonObject item = value.toObject();
        const QString key = item.value("key").toString().trimmed();
        QStringList translations;
        const QJsonArray trans = item.value("trans").toArray();
        for (const QJsonValue &transValue : trans) {
            if (translations.size() >= 3) {
                break;
            }
            const QString translation = transValue.toObject().value("value").toString().trimmed();
            if (!translation.isEmpty()) {
                translations << translation;
            }
        }
        if (!key.isEmpty() && !translations.isEmpty()) {
            lines << QString("%1: %2").arg(key, translations.join("; "));
        }
    }
    addSection(sections, "usage", lines);
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
        QTimer::singleShot(0, this, [this, text, from, to]() {
            emit translationFinished(makeDictionaryResult(false, text.trimmed(), from, to, QString(), QString(), QString(), {}, {}, "Youdao dictionary is disabled in settings."));
        });
        return;
    }

    const QString query = text.trimmed();
    if (query.isEmpty()) {
        QTimer::singleShot(0, this, [this, from, to]() {
            emit translationFinished(makeDictionaryResult(false, QString(), from, to, QString(), QString(), QString(), {}, {}, "Translation text is empty."));
        });
        return;
    }

    const QString foreignLanguage = youdaoForeignLanguage(from, to);
    if (foreignLanguage.isEmpty()) {
        QTimer::singleShot(0, this, [this, query, from, to]() {
            emit translationFinished(makeDictionaryResult(false, query, from, to, QString(), QString(), QString(), {}, {}, "Youdao dictionary supports Chinese with English, Japanese, Korean, or French."));
        });
        return;
    }

    m_pendingFrom = from;
    m_pendingTo = to;
    m_pendingQuery = query;

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
        emit translationFinished(makeDictionaryResult(false, m_pendingQuery, m_pendingFrom, m_pendingTo, QString(), QString(), QString(), {}, {}, "Youdao dictionary network error: " + networkErrorString));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        emit translationFinished(makeDictionaryResult(false, m_pendingQuery, m_pendingFrom, m_pendingTo, QString(), QString(), QString(), {}, {}, "Youdao dictionary returned an invalid response."));
        return;
    }

    const QJsonObject root = doc.object();
    QStringList lines;
    QString phonetic;
    QString audioUrl;
    QList<DictionaryPhonetic> phonetics;
    QList<DictionarySection> sections;

    appendEcResult(root.value("ec").toObject(), lines, phonetic, audioUrl, phonetics, sections);
    appendCeResult(root.value("ce").toObject(), lines, phonetic, phonetics, sections);
    appendWebTranslations(root, sections);

    const QString fanyi = root.value("fanyi").toObject().value("tran").toString().trimmed();
    if (lines.isEmpty() && !fanyi.isEmpty()) {
        lines << fanyi;
    }

    if (lines.isEmpty()) {
        emit translationFinished(makeDictionaryResult(false, m_pendingQuery, m_pendingFrom, m_pendingTo, QString(), phonetic, audioUrl, phonetics, sections, "有道词典未返回可显示结果。"));
        return;
    }

    emit translationFinished(makeDictionaryResult(true, m_pendingQuery, m_pendingFrom, m_pendingTo, lines.join("\n"), phonetic, audioUrl, phonetics, sections, QString()));
}
