#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <QDialog>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QGuiApplication>

#include "appconfig.h"
#include "translationcachestore.h"
#include "translationresult.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Translate;
}
QT_END_NAMESPACE

class SettingsWidget;
class BaiduTranslatorService;
class DeepLTranslatorService;
class DictionaryTranslatorService;
class OpenAITranslatorService;
class TranslatorService;
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
    void onTranslationFinished(const TranslationResult &result);
    void swapLanguagePair();
private:
    void reloadLanguagePairs();
    bool parseLanguagePair(const QString &pair, QString &from, QString &to) const;
    QString activeProviderKey() const;
    TranslatorService *activeTranslatorService() const;
    void applyLanguage(AppLanguage language);
    void applyDialogStyle();
    void applyShortcuts(const ShortcutConfig &shortcuts);
    void unregisterGlobalHotkeys();
    void registerGlobalHotkeys(const ShortcutConfig &shortcuts);
    bool hasRegisteredHotkeys() const;

    Ui::Translate *ui;
    QPoint m_dragPosition;
    AppConfig m_config;
    SettingsWidget *m_settingsWidget;
    BaiduTranslatorService *m_baiduService;
    OpenAITranslatorService *m_openAIService;
    DeepLTranslatorService *m_deepLService;
    DictionaryTranslatorService *m_dictionaryService;
    QHotkey *m_swapHotkey;
    QHotkey *m_pinHotkey;
    QHotkey *m_settingsHotkey;
    bool m_isTranslating;
    QString m_hotkeyStatusMessage;
    TranslationCacheStore m_translationCache;
    QString m_pendingSourceText;
    QString m_pendingFrom;
    QString m_pendingTo;
    QString m_pendingProvider;
};
#endif // TRANSLATE_H
