#ifndef OPENAITRANSLATORSERVICE_H
#define OPENAITRANSLATORSERVICE_H

#include <QNetworkAccessManager>

#include "translatorservice.h"

class OpenAITranslatorService : public TranslatorService
{
    Q_OBJECT

public:
    explicit OpenAITranslatorService(QObject *parent = nullptr);

    void setConfig(const AppConfig &config) override;
    void translate(const QString &text, const QString &from, const QString &to) override;

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QUrl endpointUrl() const;
    QString promptFor(const QString &text, const QString &from, const QString &to) const;

private:
    QNetworkAccessManager m_network;
    GenericApiConfig m_config;
    QString m_pendingFrom;
    QString m_pendingTo;
};

#endif // OPENAITRANSLATORSERVICE_H
