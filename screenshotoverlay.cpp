#include "screenshotoverlay.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QVector>

#if defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#include <dlfcn.h>
#endif

namespace {
#if defined(Q_OS_MACOS)
bool closeTo(qreal a, qreal b)
{
    return qAbs(a - b) < 2.0;
}

CGDirectDisplayID displayIdForScreen(const QScreen *screen)
{
    if (!screen) {
        return CGMainDisplayID();
    }

    uint32_t displayCount = 0;
    if (CGGetActiveDisplayList(0, nullptr, &displayCount) != kCGErrorSuccess || displayCount == 0) {
        return CGMainDisplayID();
    }

    QVector<CGDirectDisplayID> displays;
    displays.resize(int(displayCount));
    if (CGGetActiveDisplayList(displayCount, displays.data(), &displayCount) != kCGErrorSuccess) {
        return CGMainDisplayID();
    }

    const QRect geometry = screen->geometry();
    const qreal dpr = screen->devicePixelRatio();
    for (CGDirectDisplayID display : displays) {
        const CGRect bounds = CGDisplayBounds(display);
        const QRectF logicalBounds(bounds.origin.x / dpr,
                                   bounds.origin.y / dpr,
                                   bounds.size.width / dpr,
                                   bounds.size.height / dpr);
        if (closeTo(logicalBounds.x(), geometry.x())
            && closeTo(logicalBounds.y(), geometry.y())
            && closeTo(logicalBounds.width(), geometry.width())
            && closeTo(logicalBounds.height(), geometry.height())) {
            return display;
        }

        const QRectF unscaledBounds(bounds.origin.x,
                                    bounds.origin.y,
                                    bounds.size.width,
                                    bounds.size.height);
        if (closeTo(unscaledBounds.x(), geometry.x())
            && closeTo(unscaledBounds.y(), geometry.y())
            && closeTo(unscaledBounds.width(), geometry.width())
            && closeTo(unscaledBounds.height(), geometry.height())) {
            return display;
        }
    }

    return CGMainDisplayID();
}

QImage qImageFromCgImage(CGImageRef cgImage)
{
    if (!cgImage) {
        return {};
    }

    const int width = int(CGImageGetWidth(cgImage));
    const int height = int(CGImageGetHeight(cgImage));
    if (width <= 0 || height <= 0) {
        return {};
    }

    QImage image(width, height, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(image.bits(),
                                                 size_t(width),
                                                 size_t(height),
                                                 8,
                                                 size_t(image.bytesPerLine()),
                                                 colorSpace,
                                                 kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast);
    CGColorSpaceRelease(colorSpace);
    if (!context) {
        return {};
    }

    CGContextTranslateCTM(context, 0, height);
    CGContextScaleCTM(context, 1, -1);
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
    CGContextRelease(context);
    return image;
}

CGImageRef createDisplayImage(CGDirectDisplayID displayId)
{
    using CreateImageFn = CGImageRef (*)(CGDirectDisplayID);
    static CreateImageFn createImage = reinterpret_cast<CreateImageFn>(dlsym(RTLD_DEFAULT, "CGDisplayCreateImage"));
    if (!createImage) {
        return nullptr;
    }
    return createImage(displayId);
}
#endif
}

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

QImage ScreenshotOverlay::captureSelection(const QRect &selection) const
{
#if defined(Q_OS_MACOS)
    if (!m_screen || selection.isEmpty()) {
        return {};
    }

    const qreal scaleFactor = m_screen->devicePixelRatio();
    const CGRect cropRect = CGRectIntegral(CGRectMake(selection.x() * scaleFactor,
                                                      selection.y() * scaleFactor,
                                                      selection.width() * scaleFactor,
                                                      selection.height() * scaleFactor));
    const CGDirectDisplayID displayId = displayIdForScreen(m_screen);
    CGImageRef displayImage = createDisplayImage(displayId);
    if (!displayImage) {
        return {};
    }

    CGImageRef croppedImage = CGImageCreateWithImageInRect(displayImage, cropRect);
    CGImageRelease(displayImage);
    if (!croppedImage) {
        return {};
    }

    const QImage image = qImageFromCgImage(croppedImage);
    CGImageRelease(croppedImage);
    return image;
#else
    Q_UNUSED(selection)
    return croppedSelection();
#endif
}
