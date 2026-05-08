#include "ocrservice.h"
#include "tempfilemanager.h"

#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QMetaObject>
#include <QProcess>
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

#if defined(OPENTRANSLATE_HAS_CPPWINRT)
using namespace winrt;
using namespace Windows::Globalization;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Ocr;
using namespace Windows::Storage::Streams;

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
QString powerShellOcrScript()
{
    return QStringLiteral(R"ps1(param([string]$ImagePath, [string[]]$LanguageTags)
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Add-Type -AssemblyName System.Runtime.WindowsRuntime
[void][System.Reflection.Assembly]::LoadWithPartialName('System.Runtime.WindowsRuntime')
$null = [Windows.Storage.StorageFile, Windows.Storage, ContentType=WindowsRuntime]
$null = [Windows.Storage.Streams.IRandomAccessStreamWithContentType, Windows.Storage.Streams, ContentType=WindowsRuntime]
$null = [Windows.Graphics.Imaging.BitmapDecoder, Windows.Graphics.Imaging, ContentType=WindowsRuntime]
$null = [Windows.Graphics.Imaging.BitmapPixelFormat, Windows.Graphics.Imaging, ContentType=WindowsRuntime]
$null = [Windows.Graphics.Imaging.BitmapAlphaMode, Windows.Graphics.Imaging, ContentType=WindowsRuntime]
$null = [Windows.Graphics.Imaging.SoftwareBitmap, Windows.Graphics.Imaging, ContentType=WindowsRuntime]
$null = [Windows.Media.Ocr.OcrEngine, Windows.Foundation, ContentType=WindowsRuntime]
$null = [Windows.Media.Ocr.OcrResult, Windows.Foundation, ContentType=WindowsRuntime]
$null = [Windows.Globalization.Language, Windows.Globalization, ContentType=WindowsRuntime]
$asTaskCandidates = [System.WindowsRuntimeSystemExtensions].GetMethods() |
    Where-Object {
        $_.Name -eq 'AsTask' -and
        $_.IsGenericMethodDefinition -and
        $_.GetParameters().Count -eq 1
    }
$asTaskMethod = $asTaskCandidates |
    Where-Object {
        $parameterType = $_.GetParameters()[0].ParameterType
        $parameterType.Namespace -eq 'Windows.Foundation' -and
        ($parameterType.Name -eq 'IAsyncOperation`1' -or $parameterType.FullName -like 'Windows.Foundation.IAsyncOperation`1*')
    } |
    Select-Object -First 1
if ($null -eq $asTaskMethod) {
    $asTaskMethod = $asTaskCandidates |
        Where-Object {
            $parameterType = $_.GetParameters()[0].ParameterType
            $parameterType.Name -like 'IAsyncOperation*' -or $parameterType.FullName -like '*IAsyncOperation*'
        } |
        Select-Object -First 1
}
if ($null -eq $asTaskMethod) {
    $available = ($asTaskCandidates | ForEach-Object { $_.GetParameters()[0].ParameterType.FullName + ' / ' + $_.GetParameters()[0].ParameterType.Name }) -join '; '
    throw "Could not find Windows Runtime AsTask IAsyncOperation overload. Available AsTask overloads: $available"
}
function AwaitOperation($Operation, [Type]$ResultType) {
    if ($null -eq $Operation) {
        throw 'Windows Runtime OCR operation is null.'
    }
    try {
        $task = $asTaskMethod.MakeGenericMethod($ResultType).Invoke($null, [object[]]@($Operation))
        return $task.GetAwaiter().GetResult()
    } catch [System.Reflection.TargetInvocationException] {
        if ($_.Exception.InnerException) {
            throw $_.Exception.InnerException
        }
        throw
    } catch {
        throw "Could not await Windows Runtime OCR operation: $($_.Exception.Message)"
    }
}
$engine = $null
foreach ($tag in $LanguageTags) {
    if ([string]::IsNullOrWhiteSpace($tag)) { continue }
    try {
        $language = [Windows.Globalization.Language]::new($tag)
        $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage($language)
        if ($null -ne $engine) { break }
    } catch {
    }
}
if ($null -eq $engine) {
    $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages()
}
if ($null -eq $engine) {
    throw 'No Windows OCR language is available. Install OCR language features in Windows Settings.'
}
$ImagePath = [System.IO.Path]::GetFullPath($ImagePath)
$file = AwaitOperation ([Windows.Storage.StorageFile]::GetFileFromPathAsync($ImagePath)) ([Windows.Storage.StorageFile])
$stream = AwaitOperation ($file.OpenReadAsync()) ([Windows.Storage.Streams.IRandomAccessStreamWithContentType])
$decoder = AwaitOperation ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
$bitmap = AwaitOperation ($decoder.GetSoftwareBitmapAsync([Windows.Graphics.Imaging.BitmapPixelFormat]::Bgra8, [Windows.Graphics.Imaging.BitmapAlphaMode]::Premultiplied)) ([Windows.Graphics.Imaging.SoftwareBitmap])
$result = AwaitOperation ($engine.RecognizeAsync($bitmap)) ([Windows.Media.Ocr.OcrResult])
$lines = @()
foreach ($line in $result.Lines) {
    if (-not [string]::IsNullOrWhiteSpace($line.Text)) {
        $lines += $line.Text
    }
}
$text = ($lines -join [Environment]::NewLine).Trim()
if ([string]::IsNullOrWhiteSpace($text)) {
    throw 'No text was recognized in the screenshot.'
}
Write-Output $text
)ps1");
}

QStringList windowsOcrLanguageTags(const QStringList &languageHints)
{
    QStringList tags;
    for (const QString &hint : languageHints) {
        const QString tag = windowsLanguageTag(hint);
        if (!tag.isEmpty()) {
            tags << tag;
        }
    }
    tags.removeDuplicates();
    return tags;
}

QString createTempFilePath(const QString &suffix)
{
    return TempFileManager::filePath(QStringLiteral("opentranslate_ocr_"), suffix);
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

        const QString imagePath = createTempFilePath(QStringLiteral(".png"));
        const QString scriptPath = createTempFilePath(QStringLiteral(".ps1"));
        if (imagePath.isEmpty() || scriptPath.isEmpty()) {
            emit recognitionFinished(false, QString(), QStringLiteral("Failed to prepare Windows OCR temporary files."));
            return;
        }
        if (!image.save(imagePath, "PNG")) {
            emit recognitionFinished(false, QString(), QStringLiteral("Failed to prepare screenshot for Windows OCR."));
            return;
        }

        QFile scriptFile(scriptPath);
        if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QFile::remove(imagePath);
            emit recognitionFinished(false, QString(), QStringLiteral("Failed to prepare Windows OCR fallback script."));
            return;
        }
        scriptFile.write(powerShellOcrScript().toUtf8());
        scriptFile.close();

        auto *process = new QProcess(this);
        QStringList arguments {
            QStringLiteral("-NoProfile"),
            QStringLiteral("-ExecutionPolicy"),
            QStringLiteral("Bypass"),
            QStringLiteral("-File"),
            QDir::toNativeSeparators(scriptPath),
            QDir::toNativeSeparators(imagePath),
        };
        arguments.append(windowsOcrLanguageTags(languageHints));

        connect(process, &QProcess::errorOccurred, this, [this, process, imagePath, scriptPath](QProcess::ProcessError error) {
            if (error != QProcess::FailedToStart) {
                return;
            }
            process->setProperty("opentranslateHandled", true);
            QFile::remove(imagePath);
            QFile::remove(scriptPath);
            emit recognitionFinished(false, QString(), QStringLiteral("PowerShell Windows OCR fallback could not be started."));
            process->deleteLater();
        });
        connect(process,
                qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                this,
                [this, process, imagePath, scriptPath](int exitCode, QProcess::ExitStatus exitStatus) {
                    if (process->property("opentranslateHandled").toBool()) {
                        return;
                    }
                    const QString output = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
                    const QString errorOutput = QString::fromUtf8(process->readAllStandardError()).trimmed();
                    QFile::remove(imagePath);
                    QFile::remove(scriptPath);
                    process->deleteLater();

                    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                        emit recognitionFinished(false,
                                                 QString(),
                                                 errorOutput.isEmpty() ? QStringLiteral("Windows OCR failed.") : errorOutput);
                        return;
                    }
                    if (output.isEmpty()) {
                        emit recognitionFinished(false, QString(), QStringLiteral("No text was recognized in the screenshot."));
                        return;
                    }
                    emit recognitionFinished(true, output, QString());
                });
        process->start(QStringLiteral("powershell.exe"), arguments);
    }
};
#endif
}

OcrService *OcrService::create(QObject *parent)
{
    return new WindowsOcrService(parent);
}
