#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QStringList>
#include <QtGlobal>

enum class ProviderType {
    Baidu = 0,
    Generic = 1
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

struct ShortcutConfig {
    QString swapLanguage;
    QString toggleOnTop;
    QString openSettings;
};

inline ShortcutConfig defaultShortcutsForCurrentPlatform()
{
#if defined(Q_OS_MACOS)
    return {"Ctrl+Meta+T", "Ctrl+Meta+F", "Meta+,"};
#elif defined(Q_OS_WIN)
    return {"Ctrl+Alt+T", "Ctrl+Alt+F", "Ctrl+Alt+,"};
#else
    return {"Ctrl+Alt+T", "Ctrl+Alt+F", "Ctrl+Alt+,"};
#endif
}

struct AppConfig {
    BaiduConfig baidu;
    GenericApiConfig generic;
    ShortcutConfig shortcuts = defaultShortcutsForCurrentPlatform();
    QStringList languagePairs;
    ProviderType activeProvider = ProviderType::Baidu;
    AppLanguage appLanguage = AppLanguage::SimplifiedChinese;
};

#endif // APPCONFIG_H
