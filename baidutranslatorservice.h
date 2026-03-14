#ifndef BAIDUTRANSLATORSERVICE_H
#define BAIDUTRANSLATORSERVICE_H

#include <QNetworkAccessManager>

#include "translatorservice.h"

class BaiduTranslatorService : public TranslatorService
{
    Q_OBJECT

public:
    explicit BaiduTranslatorService(QObject *parent = nullptr);

    void setConfig(const AppConfig &config) override;
    void translate(const QString &text, const QString &from, const QString &to) override;

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    static QString generateSign(const QString &appId,
                                const QString &query,
                                const QString &salt,
                                const QString &appKey);

private:
    QNetworkAccessManager m_network;
    BaiduConfig m_baiduConfig;
};

#endif // BAIDUTRANSLATORSERVICE_H
