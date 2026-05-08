#ifndef SCREENSHOTOVERLAY_H
#define SCREENSHOTOVERLAY_H

#include <QImage>
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
    QRect imageRectForSelection(const QRect &selection) const;
    QImage captureSelection(const QRect &selection) const;

    QScreen *m_screen;
    QImage m_screenImage;
    qreal m_devicePixelRatio;
    QPoint m_startPos;
    QPoint m_currentPos;
    bool m_selecting;
};

#endif // SCREENSHOTOVERLAY_H
