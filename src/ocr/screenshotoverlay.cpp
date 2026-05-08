#include "screenshotoverlay.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QTimer>

#if defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#endif

ScreenshotOverlay::ScreenshotOverlay(QWidget *parent)
    : QWidget(parent)
    , m_screen(nullptr)
    , m_devicePixelRatio(1.0)
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

    const QPixmap screenPixmap = m_screen->grabWindow(0);
    if (screenPixmap.isNull()) {
        emit captureFailed(QStringLiteral("dialog.error.screen_capture_failed"));
        deleteLater();
        return;
    }
    m_devicePixelRatio = screenPixmap.devicePixelRatio();
    if (m_devicePixelRatio <= 0) {
        m_devicePixelRatio = m_screen->devicePixelRatio();
    }
    if (m_devicePixelRatio <= 0) {
        m_devicePixelRatio = 1.0;
    }
    m_screenImage = screenPixmap.toImage();
    if (m_screenImage.isNull()) {
        emit captureFailed(QStringLiteral("dialog.error.screen_capture_failed"));
        deleteLater();
        return;
    }

    setGeometry(m_screen->geometry());
    show();
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

    hide();
    QTimer::singleShot(80, this, [this, selection]() {
        const QImage image = captureSelection(selection);
        if (image.isNull()) {
            emit captureFailed(QStringLiteral("Screenshot crop failed."));
        } else {
            emit captureFinished(image);
        }
        close();
        deleteLater();
    });
}

void ScreenshotOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.drawImage(rect(), m_screenImage);
    painter.fillRect(rect(), QColor(0, 0, 0, 90));

    const QRect selection = normalizedSelection();
    if (selection.isEmpty()) {
        return;
    }

    painter.drawImage(selection, m_screenImage, imageRectForSelection(selection));
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(QColor(65, 145, 255), 2));
    painter.drawRect(selection.adjusted(0, 0, -1, -1));
}

QRect ScreenshotOverlay::normalizedSelection() const
{
    return QRect(m_startPos, m_currentPos).normalized().intersected(rect());
}

QRect ScreenshotOverlay::imageRectForSelection(const QRect &selection) const
{
    if (selection.isEmpty()) {
        return {};
    }

    return QRect(qRound(selection.x() * m_devicePixelRatio),
                 qRound(selection.y() * m_devicePixelRatio),
                 qRound(selection.width() * m_devicePixelRatio),
                 qRound(selection.height() * m_devicePixelRatio))
        .intersected(m_screenImage.rect());
}

QImage ScreenshotOverlay::captureSelection(const QRect &selection) const
{
    const QRect imageRect = imageRectForSelection(selection);
    if (imageRect.isEmpty()) {
        return {};
    }

    return m_screenImage.copy(imageRect);
}
