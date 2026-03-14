#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include "appconfig.h"

class ConfigStore
{
public:
    static AppConfig load();
    static void save(const AppConfig &config);

private:
    static QStringList normalizedPairs(QStringList pairs);
};

#endif // CONFIGSTORE_H
