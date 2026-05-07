#ifndef SCREENSHOTOVERLAY_H
#define SCREENSHOTOVERLAY_H

#include <QPixmap>
#include <QWidget>

class QScreen;

class ScreenshotOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenshotOverlay(QWidget *parent = nullptr);

    void begin();

signals:
    void captureFinished(const QImage &image);
    void captureCancelled();
    void captureFailed(const QString &message);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QRect normalizedSelection() const;
    QImage croppedSelection() const;

    QScreen *m_screen;
    QPixmap m_screenPixmap;
    QPoint m_startPos;
    QPoint m_currentPos;
    bool m_selecting;
};

#endif // SCREENSHOTOVERLAY_H
