#ifndef SPEECHPLAYER_H
#define SPEECHPLAYER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QString>

class SpeechPlayer : public QObject
{
    Q_OBJECT

public:
    explicit SpeechPlayer(QObject *parent = nullptr);

    bool isPlaying() const;
    void play(const QString &text, const QString &language, const QString &audioUrl = QString());
    void stop();

signals:
    void playingChanged(bool playing);
    void errorOccurred(const QString &message);

private:
    void fetchDictionaryAudioUrl(const QString &text, const QString &language);
    void playAudioUrl(const QString &audioUrl);
    void playLocalFile(const QString &filePath);
    void pollMciPlayback();
    void closeMciPlayback();
    bool canUseDictionaryFallback(const QString &text, const QString &language) const;
    void setPlaying(bool playing);

private:
    QNetworkAccessManager m_network;
    QProcess m_process;
    QString m_mciAlias;
    bool m_isPlaying;
    bool m_stopRequested;
};

#endif // SPEECHPLAYER_H
