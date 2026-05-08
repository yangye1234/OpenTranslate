#include "speechplayer.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>
#include <QtGlobal>

namespace {
QString youdaoTtsLanguageCode(const QString &language)
{
    const QString code = language.toLower();
    if (code == "zh" || code == "zh-cn" || code == "zh-tw" || code == "zh-hans" || code == "zh-hant") {
        return "zh";
    }
    if (code == "en" || code.startsWith("en-")) {
        return "en";
    }
    if (code == "ja" || code == "jp" || code.startsWith("ja-")) {
        return "ja";
    }
    if (code == "ko" || code == "kr" || code.startsWith("ko-")) {
        return "ko";
    }
    if (code == "fr" || code.startsWith("fr-")) {
        return "fr";
    }
    return {};
}

QString youdaoTtsUrl(const QString &text, const QString &language)
{
    const QString ttsLanguage = youdaoTtsLanguageCode(language);
    if (ttsLanguage.isEmpty()) {
        return {};
    }

    QString url = "https://dict.youdao.com/dictvoice?audio=" + QString::fromLatin1(QUrl::toPercentEncoding(text.trimmed()));
    url += "&le=" + ttsLanguage;
    if (ttsLanguage == "en") {
        url += "&type=2";
    }
    return url;
}

QString audioFileSuffix(const QString &audioUrl, const QString &contentType)
{
    const QString type = contentType.toLower();
    if (type.contains("mpeg") || type.contains("mp3")) {
        return QStringLiteral(".mp3");
    }
    if (type.contains("wav") || type.contains("wave")) {
        return QStringLiteral(".wav");
    }
    if (type.contains("aac")) {
        return QStringLiteral(".aac");
    }
    if (type.contains("mp4") || type.contains("m4a")) {
        return QStringLiteral(".m4a");
    }

    const QString path = QUrl(audioUrl).path().toLower();
    if (path.endsWith(".mp3")) {
        return QStringLiteral(".mp3");
    }
    if (path.endsWith(".wav")) {
        return QStringLiteral(".wav");
    }
    if (path.endsWith(".aac")) {
        return QStringLiteral(".aac");
    }
    if (path.endsWith(".m4a") || path.endsWith(".mp4")) {
        return QStringLiteral(".m4a");
    }
    return QStringLiteral(".mp3");
}
}

SpeechPlayer::SpeechPlayer(QObject *parent)
    : QObject(parent)
    , m_isPlaying(false)
    , m_stopRequested(false)
{
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                setPlaying(false);
                if (!m_stopRequested && (exitStatus != QProcess::NormalExit || exitCode != 0)) {
                    QString detail = QString::fromLocal8Bit(m_process.readAllStandardError()).trimmed();
                    if (detail.isEmpty()) {
                        detail = QString::fromLocal8Bit(m_process.readAllStandardOutput()).trimmed();
                    }
                    emit errorOccurred(detail.isEmpty() ? QStringLiteral("Audio playback failed.") : detail);
                }
                m_stopRequested = false;
            });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        setPlaying(false);
        if (!m_stopRequested) {
            emit errorOccurred(m_process.errorString());
        }
    });
}

bool SpeechPlayer::isPlaying() const
{
    return m_isPlaying;
}

void SpeechPlayer::play(const QString &text, const QString &language, const QString &audioUrl)
{
    const QString trimmed = text.trimmed();
    const QString trimmedAudioUrl = audioUrl.trimmed();
    if (trimmed.isEmpty() && trimmedAudioUrl.isEmpty()) {
        return;
    }

    stop();

    if (!trimmedAudioUrl.isEmpty()) {
        playAudioUrl(trimmedAudioUrl);
        return;
    }

    if (canUseDictionaryFallback(trimmed, language)) {
        fetchDictionaryAudioUrl(trimmed, language);
        return;
    }

    emit errorOccurred("No provider or Youdao dictionary audio is available for this text.");
}

void SpeechPlayer::fetchDictionaryAudioUrl(const QString &text, const QString &language)
{
    const QString audioUrl = youdaoTtsUrl(text, language);
    if (audioUrl.isEmpty()) {
        emit errorOccurred("Youdao dictionary audio is unavailable for this language.");
        return;
    }
    playAudioUrl(audioUrl);
}

void SpeechPlayer::playAudioUrl(const QString &audioUrl)
{
    const QUrl url(audioUrl);
    if (!url.isValid()) {
        emit errorOccurred("Audio URL is invalid.");
        return;
    }

    QNetworkReply *reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, audioUrl]() {
        const QByteArray payload = reply->readAll();
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString networkErrorString = reply->errorString();
        const QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        reply->deleteLater();

        if (networkError != QNetworkReply::NoError || payload.isEmpty()) {
            emit errorOccurred("Audio download failed: " + networkErrorString);
            return;
        }

        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        if (tempDir.isEmpty()) {
            tempDir = QDir::tempPath();
        }
        const QByteArray hash = QCryptographicHash::hash(audioUrl.toUtf8(), QCryptographicHash::Md5).toHex();
        const QString suffix = audioFileSuffix(audioUrl, contentType);
        const QString filePath = QDir(tempDir).filePath("opentranslate_audio_" + QString::fromLatin1(hash) + suffix);

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit errorOccurred("Could not write temporary audio file.");
            return;
        }
        file.write(payload);
        file.close();
        playLocalFile(filePath);
    });
}

void SpeechPlayer::playLocalFile(const QString &filePath)
{
    m_stopRequested = false;
#if defined(Q_OS_MACOS)
    m_process.start("afplay", {filePath});
#elif defined(Q_OS_WIN)
    const QString fileUri = QUrl::fromLocalFile(filePath).toString();
    const QString script = QString(
        "$ErrorActionPreference = 'Stop'; "
        "Add-Type -AssemblyName PresentationCore; "
        "$p = New-Object System.Windows.Media.MediaPlayer; "
        "$p.Open([Uri]::new($args[0])); "
        "$openDeadline = (Get-Date).AddSeconds(10); "
        "while (-not $p.NaturalDuration.HasTimeSpan -and (Get-Date) -lt $openDeadline) { Start-Sleep -Milliseconds 100 }; "
        "if (-not $p.NaturalDuration.HasTimeSpan) { $p.Close(); Write-Error 'Audio file could not be opened.'; exit 2 }; "
        "$p.Play(); "
        "$duration = $p.NaturalDuration.TimeSpan; "
        "$playDeadline = (Get-Date).Add($duration).AddSeconds(2); "
        "while ($p.Position -lt $duration -and (Get-Date) -lt $playDeadline) { Start-Sleep -Milliseconds 100 }; "
        "$p.Close();");
    m_process.start("powershell", {"-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", script, fileUri});
#else
    Q_UNUSED(filePath);
    emit errorOccurred("Audio playback is not supported on this platform yet.");
    return;
#endif

    if (!m_process.waitForStarted(1000)) {
        emit errorOccurred(m_process.errorString());
        return;
    }
    setPlaying(true);
}

bool SpeechPlayer::canUseDictionaryFallback(const QString &text, const QString &language) const
{
    const QString trimmed = text.trimmed();
    return !trimmed.isEmpty() && trimmed.size() <= 600 && !youdaoTtsLanguageCode(language).isEmpty();
}

void SpeechPlayer::stop()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_stopRequested = true;
        m_process.kill();
        m_process.waitForFinished(500);
    }
    setPlaying(false);
}

void SpeechPlayer::setPlaying(bool playing)
{
    if (m_isPlaying == playing) {
        return;
    }
    m_isPlaying = playing;
    emit playingChanged(m_isPlaying);
}
