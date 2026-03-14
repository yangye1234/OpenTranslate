#ifndef TRANSLATORSERVICE_H
#define TRANSLATORSERVICE_H

#include <QObject>

#include "appconfig.h"

class TranslatorService : public QObject
{
    Q_OBJECT

public:
    explicit TranslatorService(QObject *parent = nullptr) : QObject(parent) {}
    ~TranslatorService() override = default;

    virtual void setConfig(const AppConfig &config) = 0;
    virtual void translate(const QString &text, const QString &from, const QString &to) = 0;

signals:
    void translationFinished(bool success, const QString &translatedText, const QString &errorMessage);
};

#endif // TRANSLATORSERVICE_H
