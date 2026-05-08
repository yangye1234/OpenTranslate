#ifndef OCRSERVICE_H
#define OCRSERVICE_H

#include <QObject>
#include <QImage>
#include <QStringList>

class OcrService : public QObject
{
    Q_OBJECT

public:
    explicit OcrService(QObject *parent = nullptr);
    ~OcrService() override;

    static OcrService *create(QObject *parent = nullptr);

    virtual void recognizeText(const QImage &image, const QStringList &languageHints) = 0;

signals:
    void recognitionFinished(bool success, const QString &text, const QString &errorMessage);
};

#endif // OCRSERVICE_H
