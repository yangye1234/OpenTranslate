#ifndef SPEECHPLAYER_H
#define SPEECHPLAYER_H

#include <QObject>
#include <QProcess>

class SpeechPlayer : public QObject
{
    Q_OBJECT

public:
    explicit SpeechPlayer(QObject *parent = nullptr);

    bool isPlaying() const;
    void play(const QString &text, const QString &language);
    void stop();

signals:
    void playingChanged(bool playing);
    void errorOccurred(const QString &message);

private:
    void setPlaying(bool playing);

private:
    QProcess m_process;
    bool m_isPlaying;
};

#endif // SPEECHPLAYER_H
