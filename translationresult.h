#ifndef TRANSLATIONRESULT_H
#define TRANSLATIONRESULT_H

#include <QString>

struct TranslationResult {
    bool success = false;
    QString provider;
    QString sourceLanguage;
    QString targetLanguage;
    QString translatedText;
    QString phoneticText;
    QString errorMessage;
};

#endif // TRANSLATIONRESULT_H
