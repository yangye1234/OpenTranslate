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

    config.activeProvider = static_cast<ProviderType>(settings.value("provider/active", 0).toInt());
    config.appLanguage = static_cast<AppLanguage>(settings.value("app/language", 1).toInt());
    config.shortcuts.swapLanguage = settings.value("shortcuts/swap", config.shortcuts.swapLanguage).toString();
    config.shortcuts.toggleOnTop = settings.value("shortcuts/pin", config.shortcuts.toggleOnTop).toString();
    config.shortcuts.openSettings = settings.value("shortcuts/settings", config.shortcuts.openSettings).toString();

    if (config.shortcuts.swapLanguage.trimmed().isEmpty()) {
        config.shortcuts.swapLanguage = defaultShortcutsForCurrentPlatform().swapLanguage;
    }
    if (config.shortcuts.toggleOnTop.trimmed().isEmpty()) {
        config.shortcuts.toggleOnTop = defaultShortcutsForCurrentPlatform().toggleOnTop;
    }
    if (config.shortcuts.openSettings.trimmed().isEmpty()) {
        config.shortcuts.openSettings = defaultShortcutsForCurrentPlatform().openSettings;
    }

    QStringList pairs = settings.value("languages/pairs").toStringList();
    if (pairs.isEmpty()) {
        pairs << "en->zh" << "zh->en";
    }
    config.languagePairs = normalizedPairs(pairs);
    if (config.languagePairs.isEmpty()) {
        config.languagePairs << "en->zh" << "zh->en";
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

    settings.setValue("provider/active", static_cast<int>(config.activeProvider));
    settings.setValue("app/language", static_cast<int>(config.appLanguage));
    settings.setValue("shortcuts/swap", config.shortcuts.swapLanguage);
    settings.setValue("shortcuts/pin", config.shortcuts.toggleOnTop);
    settings.setValue("shortcuts/settings", config.shortcuts.openSettings);
    settings.setValue("languages/pairs", normalizedPairs(config.languagePairs));
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
