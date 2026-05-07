#include "speechplayer.h"

#include <QTimer>
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

void SpeechPlayer::play(const QString &text, const QString &language)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    stop();

#if defined(Q_OS_MACOS)
    QStringList args;
    if (language == "en") {
        args << "-r" << "150";
    }
    args << trimmed;
    m_process.start("say", args);
#elif defined(Q_OS_WIN)
    const QString escaped = trimmed;
    const QString script = QString(
        "Add-Type -AssemblyName System.Speech; "
        "$s = New-Object System.Speech.Synthesis.SpeechSynthesizer; "
        "$s.Speak(%1);")
        .arg(QString("\"%1\"").arg(escaped.toHtmlEscaped().replace("\"", "`\"")));
    m_process.start("powershell", {"-NoProfile", "-Command", script});
#else
    Q_UNUSED(language);
    emit errorOccurred("Text-to-speech is not supported on this platform yet.");
    return;
#endif

    if (!m_process.waitForStarted(1000)) {
        emit errorOccurred(m_process.errorString());
        return;
    }
    setPlaying(true);
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
