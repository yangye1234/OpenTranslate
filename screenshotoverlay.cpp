#include "screenshotoverlay.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

#if defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#endif

ScreenshotOverlay::ScreenshotOverlay(QWidget *parent)
    : QWidget(parent)
    , m_screen(nullptr)
    , m_selecting(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void ScreenshotOverlay::begin()
{
#if defined(Q_OS_MACOS)
    if (!CGPreflightScreenCaptureAccess()) {
        CGRequestScreenCaptureAccess();
        if (!CGPreflightScreenCaptureAccess()) {
            emit captureFailed(QStringLiteral("dialog.error.screen_recording_permission"));
            deleteLater();
            return;
        }
    }
#endif

    m_screen = QGuiApplication::screenAt(QCursor::pos());
    if (!m_screen) {
        m_screen = QGuiApplication::primaryScreen();
    }
    if (!m_screen) {
        emit captureFailed(QStringLiteral("No screen is available for screenshot capture."));
        deleteLater();
        return;
    }

    m_screenPixmap = m_screen->grabWindow(0);
    if (m_screenPixmap.isNull()) {
        emit captureFailed(QStringLiteral("dialog.error.screen_capture_failed"));
        deleteLater();
        return;
    }

    setGeometry(m_screen->geometry());
    showFullScreen();
    raise();
    activateWindow();
    setFocus(Qt::OtherFocusReason);
}

void ScreenshotOverlay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit captureCancelled();
        close();
        deleteLater();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ScreenshotOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        emit captureCancelled();
        close();
        deleteLater();
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }
    m_selecting = true;
    m_startPos = event->pos();
    m_currentPos = event->pos();
    update();
}

void ScreenshotOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_selecting) {
        return;
    }
    m_currentPos = event->pos();
    update();
}

void ScreenshotOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_selecting) {
        return;
    }
    m_selecting = false;
    m_currentPos = event->pos();

    const QRect selection = normalizedSelection();
    if (selection.width() < 6 || selection.height() < 6) {
        emit captureCancelled();
        close();
        deleteLater();
        return;
    }

    const QImage image = croppedSelection();
    if (image.isNull()) {
        emit captureFailed(QStringLiteral("Screenshot crop failed."));
    } else {
        emit captureFinished(image);
    }
    close();
    deleteLater();
}

void ScreenshotOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.drawPixmap(rect(), m_screenPixmap);
    painter.fillRect(rect(), QColor(0, 0, 0, 90));

    const QRect selection = normalizedSelection();
    if (selection.isEmpty()) {
        return;
    }

    painter.drawPixmap(selection,
                       m_screenPixmap,
                       QRectF(selection.x(),
                              selection.y(),
                              selection.width(),
                              selection.height()));
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(QColor(65, 145, 255), 2));
    painter.drawRect(selection.adjusted(0, 0, -1, -1));
}

QRect ScreenshotOverlay::normalizedSelection() const
{
    return QRect(m_startPos, m_currentPos).normalized().intersected(rect());
}

QImage ScreenshotOverlay::croppedSelection() const
{
    const QRect selection = normalizedSelection();
    if (selection.isEmpty()) {
        return {};
    }

    const qreal dpr = m_screenPixmap.devicePixelRatio();
    const QRect pixelRect = QRect(qRound(selection.x() * dpr),
                                  qRound(selection.y() * dpr),
                                  qRound(selection.width() * dpr),
                                  qRound(selection.height() * dpr))
                                .intersected(m_screenPixmap.rect());
    if (pixelRect.isEmpty()) {
        return {};
    }
    return m_screenPixmap.copy(pixelRect).toImage();
}
