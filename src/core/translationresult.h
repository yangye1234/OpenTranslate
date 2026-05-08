#ifndef TRANSLATIONRESULT_H
#define TRANSLATIONRESULT_H

#include <QList>
#include <QString>
#include <QStringList>

struct DictionaryPhonetic {
    QString label;
    QString value;
    QString audioUrl;
};

struct DictionarySection {
    QString title;
    QStringList lines;
};

struct TranslationResult {
    bool success = false;
    QString provider;
    QString queryText;
    QString sourceLanguage;
    QString targetLanguage;
    QString translatedText;
    QString phoneticText;
    QString audioUrl;
    QList<DictionaryPhonetic> dictionaryPhonetics;
    QList<DictionarySection> dictionarySections;
    QString errorMessage;
};

#endif // TRANSLATIONRESULT_H
