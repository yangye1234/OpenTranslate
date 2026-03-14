#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <QDialog>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

#include "appconfig.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Translate;
}
QT_END_NAMESPACE

class SettingsWidget;

class Translate : public QDialog
{
    Q_OBJECT

public:
    Translate(QWidget *parent = nullptr);
    ~Translate();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
private slots:
    void toggleStayOnTop();
    void openSettings();
    void onConfigSaved(const AppConfig &config);
private:
    void reloadLanguagePairs();

    Ui::Translate *ui;
    QPoint m_dragPosition;
    AppConfig m_config;
    SettingsWidget *m_settingsWidget;
};
#endif // TRANSLATE_H
