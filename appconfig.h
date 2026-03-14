#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QStringList>

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

struct AppConfig {
    BaiduConfig baidu;
    GenericApiConfig generic;
    QStringList languagePairs;
    ProviderType activeProvider = ProviderType::Baidu;
    AppLanguage appLanguage = AppLanguage::SimplifiedChinese;
};

#endif // APPCONFIG_H
