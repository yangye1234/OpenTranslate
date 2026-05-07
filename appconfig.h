#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QStringList>
#include <QtGlobal>

enum class ProviderType {
    Baidu = 0,
    OpenAICompatible = 1,
    DeepL = 2,
    Dictionary = 3
};

enum class AppLanguage {
    English = 0,
    SimplifiedChinese = 1,
    TraditionalChinese = 2
};

struct BaiduConfig {
    QString appId;
    QString appKey;
    bool enabled = true;
};

struct GenericApiConfig {
    QString baseUrl;
    QString model;
    QString apiKey;
    QString promptTemplate;
    bool enabled = false;
};

struct DeepLConfig {
    QString authKey;
    QString baseUrl;
    bool enabled = false;
};

struct DictionaryConfig {
    QString appKey;
    QString appSecret;
    bool enabled = false;
};

struct CacheConfig {
    bool enabled = true;
};

struct HistoryConfig {
    bool enabled = true;
    int maxEntries = 200;
};

struct WindowBehaviorConfig {
    bool lowerOnUnpin = false;
};

struct ShortcutConfig {
    QString swapLanguage;
    QString toggleOnTop;
    QString openSettings;
    QString translateSelection;
    QString toggleSpeech;
};

inline ShortcutConfig defaultShortcutsForCurrentPlatform()
{
#if defined(Q_OS_MACOS)
    return {"Ctrl+Meta+T", "Ctrl+Meta+F", "Ctrl+,", "Ctrl+Meta+D", "Ctrl+Meta+Space"};
#elif defined(Q_OS_WIN)
    return {"Ctrl+Alt+T", "Ctrl+Alt+F", "Ctrl+Alt+,", "Ctrl+Alt+D", "Ctrl+Alt+Space"};
#else
    return {"Ctrl+Alt+T", "Ctrl+Alt+F", "Ctrl+Alt+,", "Ctrl+Alt+D", "Ctrl+Alt+Space"};
#endif
}

struct AppConfig {
    BaiduConfig baidu;
    GenericApiConfig generic;
    DeepLConfig deepL;
    DictionaryConfig dictionary;
    CacheConfig cache;
    HistoryConfig history;
    WindowBehaviorConfig windowBehavior;
    ShortcutConfig shortcuts = defaultShortcutsForCurrentPlatform();
    QStringList languagePairs;
    QString defaultLanguagePair = "zh <> en";
    ProviderType activeProvider = ProviderType::Baidu;
    AppLanguage appLanguage = AppLanguage::SimplifiedChinese;
};

#endif // APPCONFIG_H
