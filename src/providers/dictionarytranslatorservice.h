#ifndef DICTIONARYTRANSLATORSERVICE_H
#define DICTIONARYTRANSLATORSERVICE_H

#include <QNetworkAccessManager>

#include "translatorservice.h"

class DictionaryTranslatorService : public TranslatorService
{
    Q_OBJECT

public:
    explicit DictionaryTranslatorService(QObject *parent = nullptr);

    void setConfig(const AppConfig &config) override;
    void translate(const QString &text, const QString &from, const QString &to) override;

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager m_network;
    DictionaryConfig m_config;
    QString m_pendingFrom;
    QString m_pendingTo;
};

#endif // DICTIONARYTRANSLATORSERVICE_H
