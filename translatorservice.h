#ifndef TRANSLATORSERVICE_H
#define TRANSLATORSERVICE_H

#include <QObject>

#include "appconfig.h"
#include "translationresult.h"

class TranslatorService : public QObject
{
    Q_OBJECT

public:
    explicit TranslatorService(QObject *parent = nullptr) : QObject(parent) {}
    ~TranslatorService() override = default;

    virtual void setConfig(const AppConfig &config) = 0;
    virtual void translate(const QString &text, const QString &from, const QString &to) = 0;

signals:
    void translationFinished(const TranslationResult &result);
};

#endif // TRANSLATORSERVICE_H
