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
#include <utility>
#include <vector>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>
#endif

namespace {
QStringList windowsLanguageTagsForHint(const QString &language)
{
    const QString code = language.trimmed().toLower();
    if (code.isEmpty() || code == "auto") {
        return {};
    }
    if (code == "zh" || code == "zh-cn" || code == "zh-hans") {
        return {QStringLiteral("zh-Hans-CN"), QStringLiteral("zh-CN"), QStringLiteral("zh-Hans")};
    }
    if (code == "zh-tw" || code == "zh-hant" || code == "cht") {
        return {QStringLiteral("zh-Hant-TW"), QStringLiteral("zh-TW"), QStringLiteral("zh-Hant")};
    }
    if (code == "en") {
        return {QStringLiteral("en-US"), QStringLiteral("en")};
    }
    if (code == "ja" || code == "jp") {
        return {QStringLiteral("ja-JP"), QStringLiteral("ja")};
    }
    if (code == "ko" || code == "kor") {
        return {QStringLiteral("ko-KR"), QStringLiteral("ko")};
    }
    if (code == "fr" || code == "fra") {
        return {QStringLiteral("fr-FR"), QStringLiteral("fr")};
    }
    if (code == "es" || code == "spa") {
        return {QStringLiteral("es-ES"), QStringLiteral("es")};
    }
    if (code == "de") {
        return {QStringLiteral("de-DE"), QStringLiteral("de")};
    }
    if (code == "it") {
        return {QStringLiteral("it-IT"), QStringLiteral("it")};
    }
    if (code == "pt") {
        return {QStringLiteral("pt-PT"), QStringLiteral("pt")};
    }
    if (code == "ru") {
        return {QStringLiteral("ru-RU"), QStringLiteral("ru")};
    }
    return {};
}

#if defined(OPENTRANSLATE_HAS_CPPWINRT)
using namespace winrt;
using namespace Windows::Globalization;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Ocr;
using namespace Windows::Storage::Streams;

int scriptScore(const QString &text, const QString &languageTag)
{
    int cjk = 0;
    int kana = 0;
    int hangul = 0;
    int latin = 0;
    for (const QChar ch : text) {
        const ushort u = ch.unicode();
        if ((u >= 0x4E00 && u <= 0x9FFF) || (u >= 0x3400 && u <= 0x4DBF)) {
            ++cjk;
        } else if ((u >= 0x3040 && u <= 0x30FF) || (u >= 0x31F0 && u <= 0x31FF)) {
            ++kana;
        } else if (u >= 0xAC00 && u <= 0xD7AF) {
            ++hangul;
        } else if ((u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z')) {
            ++latin;
        }
    }

    const QString tag = languageTag.toLower();
    int score = text.trimmed().size();
    if (tag.startsWith(QStringLiteral("zh"))) {
        score += cjk * 8 + latin;
    } else if (tag.startsWith(QStringLiteral("ja"))) {
        score += (kana + cjk) * 8 + latin;
    } else if (tag.startsWith(QStringLiteral("ko"))) {
        score += hangul * 8 + latin;
    } else if (tag.startsWith(QStringLiteral("en"))) {
        score += latin * 4;
    } else {
        score += latin + cjk + kana + hangul;
    }
    return score;
}

std::vector<std::pair<QString, OcrEngine>> createEngines(const QStringList &languageHints)
{
    std::vector<std::pair<QString, OcrEngine>> engines;
    QStringList triedTags;

    auto addEngine = [&engines, &triedTags](const QString &tag) {
        if (tag.isEmpty()) {
            return;
        }
        if (triedTags.contains(tag, Qt::CaseInsensitive)) {
            return;
        }
        triedTags << tag;
        try {
            Language language(tag.toStdWString());
            OcrEngine engine = OcrEngine::TryCreateFromLanguage(language);
            if (engine) {
                engines.emplace_back(tag, engine);
            }
        } catch (...) {
        }
    };

    for (const QString &hint : languageHints) {
        for (const QString &tag : windowsLanguageTagsForHint(hint)) {
            addEngine(tag);
        }
    }

    try {
        for (const Language &language : OcrEngine::AvailableRecognizerLanguages()) {
            addEngine(QString::fromWCharArray(language.LanguageTag().c_str()));
        }
    } catch (...) {
    }

    OcrEngine engine = OcrEngine::TryCreateFromUserProfileLanguages();
    if (engine) {
        engines.emplace_back(QStringLiteral("user"), engine);
    }
    if (engines.empty()) {
        try {
            OcrEngine fallback = OcrEngine::TryCreateFromLanguage(Language(L"en-US"));
            if (fallback) {
                engines.emplace_back(QStringLiteral("en-US"), fallback);
            }
        } catch (...) {
        }
    }
    return engines;
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
                const auto engines = createEngines(hints);
                if (engines.empty()) {
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

                QString bestText;
                int bestScore = -1;
                for (const auto &candidate : engines) {
                    OcrResult result = candidate.second.RecognizeAsync(bitmap).get();
                    const QString text = textFromResult(result);
                    if (text.isEmpty()) {
                        continue;
                    }
                    const int score = scriptScore(text, candidate.first);
                    if (score > bestScore) {
                        bestScore = score;
                        bestText = text;
                    }
                }

                if (bestText.isEmpty()) {
                    QMetaObject::invokeMethod(this, [this]() {
                        emit recognitionFinished(false,
                                                 QString(),
                                                 QStringLiteral("No text was recognized in the screenshot."));
                    }, Qt::QueuedConnection);
                    return;
                }

                QMetaObject::invokeMethod(this, [this, bestText]() {
                    emit recognitionFinished(true, bestText, QString());
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
function TextFromOcrResult($Result) {
    $lines = @()
    foreach ($line in $Result.Lines) {
        if (-not [string]::IsNullOrWhiteSpace($line.Text)) {
            $lines += $line.Text
        }
    }
    return (($lines -join [Environment]::NewLine).Trim())
}
function GetScriptScore([string]$Text, [string]$LanguageTag) {
    $cjk = 0
    $kana = 0
    $hangul = 0
    $latin = 0
    foreach ($ch in $Text.ToCharArray()) {
        $code = [int][char]$ch
        if (($code -ge 0x4E00 -and $code -le 0x9FFF) -or ($code -ge 0x3400 -and $code -le 0x4DBF)) {
            $cjk++
        } elseif (($code -ge 0x3040 -and $code -le 0x30FF) -or ($code -ge 0x31F0 -and $code -le 0x31FF)) {
            $kana++
        } elseif ($code -ge 0xAC00 -and $code -le 0xD7AF) {
            $hangul++
        } elseif (($code -ge 65 -and $code -le 90) -or ($code -ge 97 -and $code -le 122)) {
            $latin++
        }
    }
    $tag = $LanguageTag.ToLowerInvariant()
    $score = $Text.Trim().Length
    if ($tag.StartsWith('zh')) {
        $score += $cjk * 8 + $latin
    } elseif ($tag.StartsWith('ja')) {
        $score += ($kana + $cjk) * 8 + $latin
    } elseif ($tag.StartsWith('ko')) {
        $score += $hangul * 8 + $latin
    } elseif ($tag.StartsWith('en')) {
        $score += $latin * 4
    } else {
        $score += $latin + $cjk + $kana + $hangul
    }
    return $score
}
$engineCandidates = @()
function AddOcrEngineCandidate([string]$Tag) {
    if ([string]::IsNullOrWhiteSpace($Tag)) { return }
    foreach ($candidate in $script:engineCandidates) {
        if ($candidate.Tag -ieq $Tag) { return }
    }
    try {
        $language = [Windows.Globalization.Language]::new($Tag)
        $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage($language)
        if ($null -ne $engine) {
            $script:engineCandidates += [PSCustomObject]@{ Tag = $Tag; Engine = $engine }
        }
    } catch {
    }
}
$requestedTags = @()
foreach ($tag in $LanguageTags) {
    if ([string]::IsNullOrWhiteSpace($tag)) { continue }
    $requestedTags += $tag
    AddOcrEngineCandidate $tag
}
$availableRecognizerTags = @()
try {
    foreach ($language in [Windows.Media.Ocr.OcrEngine]::AvailableRecognizerLanguages) {
        if ($null -ne $language -and -not [string]::IsNullOrWhiteSpace($language.LanguageTag)) {
            $availableRecognizerTags += $language.LanguageTag
            AddOcrEngineCandidate $language.LanguageTag
        }
    }
} catch {
}
try {
    $userEngine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages()
    if ($null -ne $userEngine) {
        $engineCandidates += [PSCustomObject]@{ Tag = 'user'; Engine = $userEngine }
    }
} catch {
}
if ($engineCandidates.Count -eq 0) {
    throw 'No Windows OCR language is available. Install OCR language features in Windows Settings.'
}
$ImagePath = [System.IO.Path]::GetFullPath($ImagePath)
$file = AwaitOperation ([Windows.Storage.StorageFile]::GetFileFromPathAsync($ImagePath)) ([Windows.Storage.StorageFile])
$stream = AwaitOperation ($file.OpenReadAsync()) ([Windows.Storage.Streams.IRandomAccessStreamWithContentType])
$decoder = AwaitOperation ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
$bitmap = AwaitOperation ($decoder.GetSoftwareBitmapAsync([Windows.Graphics.Imaging.BitmapPixelFormat]::Bgra8, [Windows.Graphics.Imaging.BitmapAlphaMode]::Premultiplied)) ([Windows.Graphics.Imaging.SoftwareBitmap])
$bestText = ''
$bestScore = -1
foreach ($candidate in $engineCandidates) {
    try {
        $result = AwaitOperation ($candidate.Engine.RecognizeAsync($bitmap)) ([Windows.Media.Ocr.OcrResult])
        $text = TextFromOcrResult $result
        if ([string]::IsNullOrWhiteSpace($text)) { continue }
        $score = GetScriptScore $text $candidate.Tag
        if ($score -gt $bestScore) {
            $bestScore = $score
            $bestText = $text
        }
    } catch {
    }
}
if ([string]::IsNullOrWhiteSpace($bestText)) {
    $requested = (($requestedTags | Select-Object -Unique) -join ', ')
    $available = (($availableRecognizerTags | Select-Object -Unique) -join ', ')
    if ([string]::IsNullOrWhiteSpace($available)) { $available = 'none' }
    throw "No text was recognized in the screenshot. Requested OCR languages: $requested. Installed OCR languages: $available."
}
Write-Output $bestText
)ps1");
}

QStringList windowsOcrLanguageTags(const QStringList &languageHints)
{
    QStringList tags;
    for (const QString &hint : languageHints) {
        for (const QString &tag : windowsLanguageTagsForHint(hint)) {
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
