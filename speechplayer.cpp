#include "speechplayer.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QtGlobal>

SpeechPlayer::SpeechPlayer(QObject *parent)
    : QObject(parent)
    , m_isPlaying(false)
{
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int, QProcess::ExitStatus) {
                setPlaying(false);
            });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        setPlaying(false);
        emit errorOccurred(m_process.errorString());
    });
}

bool SpeechPlayer::isPlaying() const
{
    return m_isPlaying;
}

void SpeechPlayer::play(const QString &text, const QString &language, const QString &audioUrl)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    stop();

    if (!audioUrl.trimmed().isEmpty()) {
        playAudioUrl(audioUrl.trimmed());
        return;
    }

    if (language == "en" && canUseDictionaryFallback(trimmed)) {
        fetchDictionaryAudioUrl(trimmed);
        return;
    }

    emit errorOccurred("No provider or dictionary audio is available for this text.");
}

void SpeechPlayer::fetchDictionaryAudioUrl(const QString &text)
{
    const QUrl url("https://api.dictionaryapi.dev/api/v2/entries/en/" + QUrl::toPercentEncoding(text.trimmed()));
    QNetworkReply *reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray payload = reply->readAll();
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString networkErrorString = reply->errorString();
        reply->deleteLater();

        if (networkError != QNetworkReply::NoError) {
            emit errorOccurred("Dictionary audio lookup failed: " + networkErrorString);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (!doc.isArray() || doc.array().isEmpty()) {
            emit errorOccurred("Dictionary returned no audio.");
            return;
        }

        const QJsonArray phonetics = doc.array().first().toObject().value("phonetics").toArray();
        for (const QJsonValue &value : phonetics) {
            const QString audioUrl = value.toObject().value("audio").toString().trimmed();
            if (!audioUrl.isEmpty()) {
                playAudioUrl(audioUrl);
                return;
            }
        }
        emit errorOccurred("Dictionary returned no audio.");
    });
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
        const QString suffix = audioUrl.contains(".mp3", Qt::CaseInsensitive) ? ".mp3" : ".audio";
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
#if defined(Q_OS_MACOS)
    m_process.start("afplay", {filePath});
#elif defined(Q_OS_WIN)
    QString windowsPath = filePath;
    windowsPath.replace("\\", "/").replace("'", "''");
    const QString script = QString(
        "Add-Type -AssemblyName PresentationCore; "
        "$p = New-Object System.Windows.Media.MediaPlayer; "
        "$p.Open([Uri](%1)); "
        "$p.Play(); "
        "Start-Sleep -Milliseconds 300; "
        "while ($p.NaturalDuration.HasTimeSpan -and $p.Position -lt $p.NaturalDuration.TimeSpan) { Start-Sleep -Milliseconds 100 };")
        .arg(QString("'file:///%1'").arg(windowsPath));
    m_process.start("powershell", {"-NoProfile", "-Command", script});
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

bool SpeechPlayer::canUseDictionaryFallback(const QString &text) const
{
    static const QRegularExpression singleEnglishWord("^[A-Za-z][A-Za-z'-]*$");
    return singleEnglishWord.match(text.trimmed()).hasMatch();
}

void SpeechPlayer::stop()
{
    if (m_process.state() != QProcess::NotRunning) {
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
