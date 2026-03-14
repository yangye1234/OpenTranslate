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
class BaiduTranslatorService;
class QHotkey;

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
    void triggerTranslate();
    void onTranslationFinished(bool success, const QString &translatedText, const QString &errorMessage);
    void swapLanguagePair();
private:
    void reloadLanguagePairs();
    bool parseLanguagePair(const QString &pair, QString &from, QString &to) const;
    void applyLanguage(AppLanguage language);
    void applyDialogStyle();
    void applyShortcuts(const ShortcutConfig &shortcuts);
    void unregisterGlobalHotkeys();
    void registerGlobalHotkeys(const ShortcutConfig &shortcuts);

    Ui::Translate *ui;
    QPoint m_dragPosition;
    AppConfig m_config;
    SettingsWidget *m_settingsWidget;
    BaiduTranslatorService *m_baiduService;
    QHotkey *m_swapHotkey;
    QHotkey *m_pinHotkey;
    QHotkey *m_settingsHotkey;
    bool m_isTranslating;
};
#endif // TRANSLATE_H
