#include "configstore.h"

#include <QSettings>

namespace {
QString normalizePair(const QString &pair)
{
    QString normalized = pair.trimmed();
    normalized.replace("-->", "->");
    normalized.remove(' ');
    normalized.replace("<->", "<>");
    normalized.replace("<=>", "<>");
    return normalized;
}

QString makeBidirectionalPair(const QString &left, const QString &right)
{
    const QString a = left.trimmed();
    const QString b = right.trimmed();
    if (a.isEmpty() || b.isEmpty() || a == b) {
        return {};
    }
    return a < b ? a + " <> " + b : b + " <> " + a;
}

QString normalizedLanguagePair(const QString &pair)
{
    const QString normalized = normalizePair(pair);
    if (normalized.contains("<>")) {
        const QStringList parts = normalized.split("<>", Qt::SkipEmptyParts);
        if (parts.size() == 2) {
            return makeBidirectionalPair(parts.at(0), parts.at(1));
        }
    }
    if (normalized.contains("->")) {
        const QStringList parts = normalized.split("->", Qt::SkipEmptyParts);
        if (parts.size() == 2) {
            return makeBidirectionalPair(parts.at(0), parts.at(1));
        }
    }
    return {};
}

ProviderType providerFromInt(int value)
{
    switch (value) {
    case 0:
        return ProviderType::Baidu;
    case 1:
        return ProviderType::OpenAICompatible;
    case 2:
        return ProviderType::DeepL;
    case 3:
        return ProviderType::Dictionary;
    default:
        return ProviderType::Baidu;
    }
}

}

AppConfig ConfigStore::load()
{
    QSettings settings("OpenTranslate", "OpenTranslate");

    AppConfig config;
    config.shortcuts = defaultShortcutsForCurrentPlatform();
    config.baidu.appId = settings.value("baidu/appId").toString();
    config.baidu.appKey = settings.value("baidu/appKey").toString();
    config.baidu.enabled = settings.value("baidu/enabled", true).toBool();

    config.generic.baseUrl = settings.value("generic/baseUrl").toString();
    config.generic.model = settings.value("generic/model").toString();
    config.generic.apiKey = settings.value("generic/apiKey").toString();
    config.generic.promptTemplate = settings.value("generic/promptTemplate").toString();
    config.generic.enabled = settings.value("generic/enabled", false).toBool();

    config.deepL.authKey = settings.value("deepl/authKey").toString();
    config.deepL.baseUrl = settings.value("deepl/baseUrl", "https://api-free.deepl.com/v2/translate").toString();
    config.deepL.enabled = settings.value("deepl/enabled", false).toBool();

    config.dictionary.appKey = settings.value("dictionary/appKey").toString();
    config.dictionary.appSecret = settings.value("dictionary/appSecret").toString();
    config.dictionary.enabled = settings.value("dictionary/enabled", false).toBool();

    config.cache.enabled = settings.value("cache/enabled", true).toBool();
    config.history.enabled = settings.value("history/enabled", true).toBool();
    config.history.maxEntries = settings.value("history/maxEntries", 200).toInt();
    if (config.history.maxEntries <= 0) {
        config.history.maxEntries = 200;
    }
    config.windowBehavior.lowerOnUnpin = settings.value("window/lowerOnUnpin", false).toBool();

    config.activeProvider = providerFromInt(settings.value("provider/active", 0).toInt());
    config.appLanguage = static_cast<AppLanguage>(settings.value("app/language", 1).toInt());
    config.shortcuts.swapLanguage = settings.value("shortcuts/swap", config.shortcuts.swapLanguage).toString();
    config.shortcuts.toggleOnTop = settings.value("shortcuts/pin", config.shortcuts.toggleOnTop).toString();
    config.shortcuts.openSettings = settings.value("shortcuts/settings", config.shortcuts.openSettings).toString();
    config.shortcuts.translateSelection = settings.value("shortcuts/translateSelection", config.shortcuts.translateSelection).toString();
    config.shortcuts.toggleSpeech = settings.value("shortcuts/toggleSpeech", config.shortcuts.toggleSpeech).toString();

    if (config.shortcuts.swapLanguage.trimmed().isEmpty()) {
        config.shortcuts.swapLanguage = defaultShortcutsForCurrentPlatform().swapLanguage;
    }
    if (config.shortcuts.toggleOnTop.trimmed().isEmpty()) {
        config.shortcuts.toggleOnTop = defaultShortcutsForCurrentPlatform().toggleOnTop;
    }
    if (config.shortcuts.openSettings.trimmed().isEmpty()) {
        config.shortcuts.openSettings = defaultShortcutsForCurrentPlatform().openSettings;
    }
    if (config.shortcuts.translateSelection.trimmed().isEmpty()) {
        config.shortcuts.translateSelection = defaultShortcutsForCurrentPlatform().translateSelection;
    }
    if (config.shortcuts.toggleSpeech.trimmed().isEmpty()) {
        config.shortcuts.toggleSpeech = defaultShortcutsForCurrentPlatform().toggleSpeech;
    }
#if defined(Q_OS_MACOS)
    // Migration: previous versions accidentally used Meta+, which maps to Control key on macOS.
    if (config.shortcuts.openSettings == "Meta+,") {
        config.shortcuts.openSettings = "Ctrl+,";
    }
#endif

    QStringList pairs = settings.value("languages/pairs").toStringList();
    if (pairs.isEmpty()) {
        pairs << "en->zh" << "zh->en";
    }
    config.languagePairs = normalizedPairs(pairs);
    if (config.languagePairs.isEmpty()) {
        config.languagePairs << "zh <> en";
    }

    config.defaultLanguagePair = normalizedPair(
        settings.value("languages/defaultPair", settings.value("languages/defaultTarget", "zh <> en")).toString());
    if (config.defaultLanguagePair.isEmpty()) {
        config.defaultLanguagePair = config.languagePairs.first();
    }
    if (!config.languagePairs.contains(config.defaultLanguagePair)) {
        config.languagePairs.prepend(config.defaultLanguagePair);
    }

    return config;
}

void ConfigStore::save(const AppConfig &config)
{
    QSettings settings("OpenTranslate", "OpenTranslate");
    settings.setValue("baidu/appId", config.baidu.appId);
    settings.setValue("baidu/appKey", config.baidu.appKey);
    settings.setValue("baidu/enabled", config.baidu.enabled);

    settings.setValue("generic/baseUrl", config.generic.baseUrl);
    settings.setValue("generic/model", config.generic.model);
    settings.setValue("generic/apiKey", config.generic.apiKey);
    settings.setValue("generic/promptTemplate", config.generic.promptTemplate);
    settings.setValue("generic/enabled", config.generic.enabled);

    settings.setValue("deepl/authKey", config.deepL.authKey);
    settings.setValue("deepl/baseUrl", config.deepL.baseUrl);
    settings.setValue("deepl/enabled", config.deepL.enabled);

    settings.setValue("dictionary/appKey", config.dictionary.appKey);
    settings.setValue("dictionary/appSecret", config.dictionary.appSecret);
    settings.setValue("dictionary/enabled", config.dictionary.enabled);

    settings.setValue("cache/enabled", config.cache.enabled);
    settings.setValue("history/enabled", config.history.enabled);
    settings.setValue("history/maxEntries", config.history.maxEntries);
    settings.setValue("window/lowerOnUnpin", config.windowBehavior.lowerOnUnpin);

    settings.setValue("provider/active", static_cast<int>(config.activeProvider));
    settings.setValue("app/language", static_cast<int>(config.appLanguage));
    settings.setValue("shortcuts/swap", config.shortcuts.swapLanguage);
    settings.setValue("shortcuts/pin", config.shortcuts.toggleOnTop);
    settings.setValue("shortcuts/settings", config.shortcuts.openSettings);
    settings.setValue("shortcuts/translateSelection", config.shortcuts.translateSelection);
    settings.setValue("shortcuts/toggleSpeech", config.shortcuts.toggleSpeech);
    settings.setValue("languages/pairs", normalizedPairs(config.languagePairs));
    settings.setValue("languages/defaultPair", normalizedPair(config.defaultLanguagePair));
}

QStringList ConfigStore::normalizedPairs(QStringList pairs)
{
    QStringList out;
    for (const QString &pair : pairs) {
        const QString normalized = normalizedPair(pair);
        if (normalized.isEmpty()) {
            continue;
        }
        if (!out.contains(normalized)) {
            out << normalized;
        }
    }
    return out;
}

QString ConfigStore::normalizedPair(const QString &pair)
{
    return normalizedLanguagePair(pair);
}
