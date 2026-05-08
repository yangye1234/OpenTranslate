#include "ocrservice.h"

OcrService::OcrService(QObject *parent)
    : QObject(parent)
{
}

OcrService::~OcrService() = default;

#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
namespace {
class UnsupportedOcrService : public OcrService
{
    Q_OBJECT

public:
    explicit UnsupportedOcrService(QObject *parent = nullptr)
        : OcrService(parent)
    {
    }

    void recognizeText(const QImage &image, const QStringList &languageHints) override
    {
        Q_UNUSED(image)
        Q_UNUSED(languageHints)
        emit recognitionFinished(false,
                                 QString(),
                                 QStringLiteral("Screenshot OCR is not available on this platform yet."));
    }
};
}

OcrService *OcrService::create(QObject *parent)
{
    return new UnsupportedOcrService(parent);
}

#include "ocrservice.moc"
#endif
