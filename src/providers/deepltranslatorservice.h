#ifndef DEEPLTRANSLATORSERVICE_H
#define DEEPLTRANSLATORSERVICE_H

#include <QNetworkAccessManager>

#include "translatorservice.h"

class DeepLTranslatorService : public TranslatorService
{
    Q_OBJECT

public:
    explicit DeepLTranslatorService(QObject *parent = nullptr);

    void setConfig(const AppConfig &config) override;
    void translate(const QString &text, const QString &from, const QString &to) override;

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QString deepLLanguageCode(const QString &language) const;

private:
    QNetworkAccessManager m_network;
    DeepLConfig m_config;
    QString m_pendingFrom;
    QString m_pendingTo;
};

#endif // DEEPLTRANSLATORSERVICE_H
