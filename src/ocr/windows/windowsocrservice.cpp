#include "ocrservice.h"

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QMetaObject>
#include <QStringList>

#if defined(OPENTRANSLATE_HAS_CPPWINRT)
#include <thread>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>
#endif

namespace {
#if defined(OPENTRANSLATE_HAS_CPPWINRT)
using namespace winrt;
using namespace Windows::Globalization;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Ocr;
using namespace Windows::Storage::Streams;

QString windowsLanguageTag(const QString &language)
{
    const QString code = language.trimmed().toLower();
    if (code.isEmpty() || code == "auto") {
        return {};
    }
    if (code == "zh" || code == "zh-cn" || code == "zh-hans") {
        return QStringLiteral("zh-Hans");
    }
    if (code == "zh-tw" || code == "zh-hant" || code == "cht") {
        return QStringLiteral("zh-Hant");
    }
    if (code == "en") {
        return QStringLiteral("en-US");
    }
    if (code == "ja" || code == "jp") {
        return QStringLiteral("ja-JP");
    }
    if (code == "ko" || code == "kor") {
        return QStringLiteral("ko-KR");
    }
    if (code == "fr" || code == "fra") {
        return QStringLiteral("fr-FR");
    }
    if (code == "es" || code == "spa") {
        return QStringLiteral("es-ES");
    }
    if (code == "de") {
        return QStringLiteral("de-DE");
    }
    if (code == "it") {
        return QStringLiteral("it-IT");
    }
    if (code == "pt") {
        return QStringLiteral("pt-PT");
    }
    if (code == "ru") {
        return QStringLiteral("ru-RU");
    }
    return {};
}

OcrEngine createEngine(const QStringList &languageHints)
{
    for (const QString &hint : languageHints) {
        const QString tag = windowsLanguageTag(hint);
        if (tag.isEmpty()) {
            continue;
        }
        try {
            Language language(tag.toStdWString());
            OcrEngine engine = OcrEngine::TryCreateFromLanguage(language);
            if (engine) {
                return engine;
            }
        } catch (...) {
        }
    }

    OcrEngine engine = OcrEngine::TryCreateFromUserProfileLanguages();
    if (engine) {
        return engine;
    }
    return OcrEngine::TryCreateFromLanguage(Language(L"en-US"));
}

SoftwareBitmap bitmapFromImage(const QImage &image)
{
    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();
    if (pngBytes.isEmpty()) {
        return nullptr;
    }

    InMemoryRandomAccessStream stream;
    DataWriter writer(stream.GetOutputStreamAt(0));
    array_view<const uint8_t> bytes(reinterpret_cast<const uint8_t *>(pngBytes.constData()),
                                    reinterpret_cast<const uint8_t *>(pngBytes.constData() + pngBytes.size()));
    writer.WriteBytes(bytes);
    writer.StoreAsync().get();
    writer.FlushAsync().get();
    writer.DetachStream();
    stream.Seek(0);

    BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();
    SoftwareBitmap bitmap = decoder.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8,
                                                           BitmapAlphaMode::Premultiplied).get();
    return bitmap;
}

QString textFromResult(const OcrResult &result)
{
    QStringList lines;
    for (const OcrLine &line : result.Lines()) {
        const QString text = QString::fromWCharArray(line.Text().c_str());
        if (!text.trimmed().isEmpty()) {
            lines << text;
        }
    }
    return lines.join('\n').trimmed();
}

QString exceptionMessage(const winrt::hresult_error &error)
{
    const QString message = QString::fromWCharArray(error.message().c_str());
    return message.isEmpty() ? QStringLiteral("Windows OCR failed.") : message;
}

class WindowsOcrService : public OcrService
{
public:
    explicit WindowsOcrService(QObject *parent = nullptr)
        : OcrService(parent)
    {
    }

    void recognizeText(const QImage &image, const QStringList &languageHints) override
    {
        if (image.isNull()) {
            emit recognitionFinished(false, QString(), QStringLiteral("Screenshot image is empty."));
            return;
        }

        QImage capturedImage = image.copy();
        QStringList hints = languageHints;
        std::thread([this, capturedImage, hints]() {
            init_apartment(apartment_type::multi_threaded);
            try {
                OcrEngine engine = createEngine(hints);
                if (!engine) {
                    QMetaObject::invokeMethod(this, [this]() {
                        emit recognitionFinished(false,
                                                 QString(),
                                                 QStringLiteral("No Windows OCR language is available. Install OCR language features in Windows Settings."));
                    }, Qt::QueuedConnection);
                    return;
                }

                SoftwareBitmap bitmap = bitmapFromImage(capturedImage);
                if (!bitmap) {
                    QMetaObject::invokeMethod(this, [this]() {
                        emit recognitionFinished(false,
                                                 QString(),
                                                 QStringLiteral("Failed to prepare screenshot for Windows OCR."));
                    }, Qt::QueuedConnection);
                    return;
                }

                OcrResult result = engine.RecognizeAsync(bitmap).get();
                const QString text = textFromResult(result);
                if (text.isEmpty()) {
                    QMetaObject::invokeMethod(this, [this]() {
                        emit recognitionFinished(false,
                                                 QString(),
                                                 QStringLiteral("No text was recognized in the screenshot."));
                    }, Qt::QueuedConnection);
                    return;
                }

                QMetaObject::invokeMethod(this, [this, text]() {
                    emit recognitionFinished(true, text, QString());
                }, Qt::QueuedConnection);
            } catch (const winrt::hresult_error &error) {
                const QString message = exceptionMessage(error);
                QMetaObject::invokeMethod(this, [this, message]() {
                    emit recognitionFinished(false, QString(), message);
                }, Qt::QueuedConnection);
            } catch (...) {
                QMetaObject::invokeMethod(this, [this]() {
                    emit recognitionFinished(false, QString(), QStringLiteral("Windows OCR failed."));
                }, Qt::QueuedConnection);
            }
        }).detach();
    }
};
#else
class WindowsOcrService : public OcrService
{
public:
    explicit WindowsOcrService(QObject *parent = nullptr)
        : OcrService(parent)
    {
    }

    void recognizeText(const QImage &image, const QStringList &languageHints) override
    {
        Q_UNUSED(image)
        Q_UNUSED(languageHints)
        emit recognitionFinished(false,
                                 QString(),
                                 QStringLiteral("Windows screenshot OCR is disabled because this build environment does not provide C++/WinRT headers."));
    }
};
#endif
}

OcrService *OcrService::create(QObject *parent)
{
    return new WindowsOcrService(parent);
}
