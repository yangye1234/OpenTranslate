#include "languagedetector.h"

#include <QChar>

QString LanguageDetector::detect(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return "auto";
    }

    if (containsInRange(trimmed, 0x4E00, 0x9FFF)) {
        return "zh";
    }
    if (containsInRange(trimmed, 0x3040, 0x30FF)) {
        return "jp";
    }
    if (containsInRange(trimmed, 0xAC00, 0xD7AF)) {
        return "kor";
    }

    int latinLetters = 0;
    int asciiLetters = 0;
    for (const QChar ch : trimmed) {
        if (!ch.isLetter()) {
            continue;
        }
        const uint code = ch.unicode();
        if ((code >= 'A' && code <= 'Z') || (code >= 'a' && code <= 'z')) {
            ++asciiLetters;
        }
        if ((code >= 0x0041 && code <= 0x024F) || (code >= 0x1E00 && code <= 0x1EFF)) {
            ++latinLetters;
        }
    }

    if (asciiLetters > 0 && asciiLetters == latinLetters) {
        return "en";
    }
    if (latinLetters > 0) {
        return "auto";
    }

    return "auto";
}

QString LanguageDetector::autoTargetFor(const QString &sourceLanguage, const QString &preferredTargetLanguage)
{
    const QString preferred = preferredTargetLanguage.trimmed().isEmpty() ? QStringLiteral("zh") : preferredTargetLanguage.trimmed();
    if (sourceLanguage == "zh" || sourceLanguage == "zh-CN" || sourceLanguage == "zh-TW") {
        return preferred == "en" ? QStringLiteral("zh") : QStringLiteral("en");
    }
    return preferred;
}

bool LanguageDetector::containsInRange(const QString &text, uint first, uint last)
{
    for (const QChar ch : text) {
        const uint code = ch.unicode();
        if (code >= first && code <= last) {
            return true;
        }
    }
    return false;
}
