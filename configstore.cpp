#include "configstore.h"

#include <QSettings>

namespace {
QString normalizePair(const QString &pair)
{
    QString normalized = pair.trimmed();
    normalized.replace("-->", "->");
    normalized.remove(' ');
    return normalized;
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

QString normalizedLanguageCode(const QString &code)
{
    return code.trimmed();
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

    config.activeProvider = providerFromInt(settings.value("provider/active", 0).toInt());
    config.appLanguage = static_cast<AppLanguage>(settings.value("app/language", 1).toInt());
    config.shortcuts.swapLanguage = settings.value("shortcuts/swap", config.shortcuts.swapLanguage).toString();
    config.shortcuts.toggleOnTop = settings.value("shortcuts/pin", config.shortcuts.toggleOnTop).toString();
    config.shortcuts.openSettings = settings.value("shortcuts/settings", config.shortcuts.openSettings).toString();
    config.shortcuts.translateSelection = settings.value("shortcuts/translateSelection", config.shortcuts.translateSelection).toString();

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
        config.languagePairs << "en->zh" << "zh->en";
    }

    QStringList targets = settings.value("languages/targets").toStringList();
    if (targets.isEmpty()) {
        for (const QString &pair : config.languagePairs) {
            const QStringList parts = pair.split("->", Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                targets << parts.at(1);
            }
        }
    }
    config.targetLanguages = normalizedLanguageCodes(targets);
    if (config.targetLanguages.isEmpty()) {
        config.targetLanguages << "zh" << "en";
    }

    config.defaultTargetLanguage = normalizedLanguageCode(
        settings.value("languages/defaultTarget", config.targetLanguages.first()).toString());
    if (config.defaultTargetLanguage.isEmpty()) {
        config.defaultTargetLanguage = config.targetLanguages.first();
    }
    if (!config.targetLanguages.contains(config.defaultTargetLanguage)) {
        config.targetLanguages.prepend(config.defaultTargetLanguage);
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

    settings.setValue("provider/active", static_cast<int>(config.activeProvider));
    settings.setValue("app/language", static_cast<int>(config.appLanguage));
    settings.setValue("shortcuts/swap", config.shortcuts.swapLanguage);
    settings.setValue("shortcuts/pin", config.shortcuts.toggleOnTop);
    settings.setValue("shortcuts/settings", config.shortcuts.openSettings);
    settings.setValue("shortcuts/translateSelection", config.shortcuts.translateSelection);
    settings.setValue("languages/pairs", normalizedPairs(config.languagePairs));
    settings.setValue("languages/targets", normalizedLanguageCodes(config.targetLanguages));
    settings.setValue("languages/defaultTarget", normalizedLanguageCode(config.defaultTargetLanguage));
}

QStringList ConfigStore::normalizedPairs(QStringList pairs)
{
    QStringList out;
    for (const QString &pair : pairs) {
        const QString normalized = normalizePair(pair);
        if (normalized.isEmpty() || !normalized.contains("->")) {
            continue;
        }
        if (!out.contains(normalized)) {
            out << normalized;
        }
    }
    return out;
}

QStringList ConfigStore::normalizedLanguageCodes(QStringList codes)
{
    QStringList out;
    for (const QString &code : codes) {
        const QString normalized = normalizedLanguageCode(code);
        if (normalized.isEmpty()) {
            continue;
        }
        if (!out.contains(normalized)) {
            out << normalized;
        }
    }
    return out;
}
