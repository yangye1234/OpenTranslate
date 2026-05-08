#ifndef LANGUAGEDETECTOR_H
#define LANGUAGEDETECTOR_H

#include <QString>

class LanguageDetector
{
public:
    static QString detect(const QString &text);
    static QString autoTargetFor(const QString &sourceLanguage, const QString &preferredTargetLanguage);

private:
    static bool containsInRange(const QString &text, uint first, uint last);
};

#endif // LANGUAGEDETECTOR_H
