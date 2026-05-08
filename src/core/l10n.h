#ifndef L10N_H
#define L10N_H

#include <QString>

#include "appconfig.h"

namespace L10n {

QString text(AppLanguage language, const QString &key);

} // namespace L10n

#endif // L10N_H
