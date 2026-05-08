#include "ocrservice.h"

#include <QImage>
#include <QMetaObject>

#import <Foundation/Foundation.h>
#import <Vision/Vision.h>

namespace {
NSString *visionLanguageCode(const QString &language)
{
    const QString code = language.trimmed().toLower();
    if (code.isEmpty() || code == "auto") {
        return nil;
    }
    if (code == "zh" || code == "zh-cn" || code == "zh-hans") {
        return @"zh-Hans";
    }
    if (code == "zh-tw" || code == "zh-hant" || code == "cht") {
        return @"zh-Hant";
    }
    if (code == "en") {
        return @"en-US";
    }
    if (code == "ja" || code == "jp") {
        return @"ja-JP";
    }
    if (code == "ko" || code == "kor") {
        return @"ko-KR";
    }
    if (code == "fr" || code == "fra") {
        return @"fr-FR";
    }
    if (code == "es" || code == "spa") {
        return @"es-ES";
    }
    if (code == "pt") {
        return @"pt-PT";
    }
    if (code == "de") {
        return @"de-DE";
    }
    if (code == "it") {
        return @"it-IT";
    }
    if (code == "ru") {
        return @"ru-RU";
    }
    if (code == "uk") {
        return @"uk-UA";
    }
    return nil;
}

NSArray<NSString *> *visionLanguageHints(const QStringList &languageHints)
{
    NSMutableArray<NSString *> *languages = [NSMutableArray array];
    for (const QString &hint : languageHints) {
        NSString *language = visionLanguageCode(hint);
        if (language && ![languages containsObject:language]) {
            [languages addObject:language];
        }
    }
    return languages;
}

CGImageRef createCgImage(const QImage &image)
{
    QImage rgba = image.convertToFormat(QImage::Format_RGBA8888).copy();
    if (rgba.isNull()) {
        return nullptr;
    }

    QByteArray *bytes = new QByteArray(reinterpret_cast<const char *>(rgba.constBits()),
                                       qsizetype(rgba.sizeInBytes()));
    CGDataProviderRef provider = CGDataProviderCreateWithData(bytes,
                                                              bytes->constData(),
                                                              size_t(bytes->size()),
                                                              [](void *info, const void *, size_t) {
                                                                  delete static_cast<QByteArray *>(info);
                                                              });
    if (!provider) {
        delete bytes;
        return nullptr;
    }

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGImageRef cgImage = CGImageCreate(size_t(rgba.width()),
                                       size_t(rgba.height()),
                                       8,
                                       32,
                                       size_t(rgba.bytesPerLine()),
                                       colorSpace,
                                       kCGBitmapByteOrder32Big | kCGImageAlphaLast,
                                       provider,
                                       nullptr,
                                       false,
                                       kCGRenderingIntentDefault);
    CGColorSpaceRelease(colorSpace);
    CGDataProviderRelease(provider);
    return cgImage;
}
}

class MacVisionOcrService : public OcrService
{
public:
    explicit MacVisionOcrService(QObject *parent = nullptr)
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
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
            @autoreleasepool {
                CGImageRef cgImage = createCgImage(capturedImage);
                if (!cgImage) {
                    QMetaObject::invokeMethod(this, [this]() {
                        emit recognitionFinished(false,
                                                 QString(),
                                                 QStringLiteral("Failed to prepare screenshot for OCR."));
                    }, Qt::QueuedConnection);
                    return;
                }

                VNRecognizeTextRequest *request = [[VNRecognizeTextRequest alloc] init];
                request.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
                request.usesLanguageCorrection = YES;
                NSArray<NSString *> *languages = visionLanguageHints(hints);
                if (languages.count > 0) {
                    request.recognitionLanguages = languages;
                }
                if (@available(macOS 11.0, *)) {
                    request.automaticallyDetectsLanguage = YES;
                }

                VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCGImage:cgImage
                                                                                        options:@{}];
                NSError *error = nil;
                BOOL success = [handler performRequests:@[request] error:&error];
                CGImageRelease(cgImage);

                if (!success || error) {
                    QString message = error ? QString::fromNSString(error.localizedDescription)
                                            : QStringLiteral("Apple Vision OCR failed.");
                    QMetaObject::invokeMethod(this, [this, message]() {
                        emit recognitionFinished(false, QString(), message);
                    }, Qt::QueuedConnection);
                    return;
                }

                NSMutableArray<NSString *> *lines = [NSMutableArray array];
                for (VNRecognizedTextObservation *observation in request.results) {
                    NSArray<VNRecognizedText *> *candidates = [observation topCandidates:1];
                    VNRecognizedText *candidate = candidates.firstObject;
                    if (candidate.string.length > 0) {
                        [lines addObject:candidate.string];
                    }
                }

                const QString text = QString::fromNSString([lines componentsJoinedByString:@"\n"]).trimmed();
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
            }
        });
    }
};

OcrService *OcrService::create(QObject *parent)
{
    return new MacVisionOcrService(parent);
}
