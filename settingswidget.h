#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>

#include "appconfig.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class SettingsWidget;
}
QT_END_NAMESPACE

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    ~SettingsWidget() override;

    void setConfig(const AppConfig &config);
    void setHotkeyStatusMessage(const QString &message);

signals:
    void configSaved(const AppConfig &config);

private slots:
    void onAddPairClicked();
    void onRemovePairClicked();
    void onLanguagePairEdited();
    void onSaveClicked();
    void onAppLanguageChanged(int index);
    void onAnySettingChanged();

private:
    static QString normalizePair(const QString &pair);
    void refreshPairList(const QStringList &pairs);
    QStringList currentPairs() const;
    void applyLanguage(AppLanguage language);
    void setupLanguageOptions();
    void setDirty(bool dirty);
    void updateSaveButtonText();
    void setupDirtyTracking();
    static bool hasShortcutConflict(const ShortcutConfig &shortcuts);

private:
    Ui::SettingsWidget *ui;
    AppLanguage m_uiLanguage;
    bool m_isDirty;
    bool m_isLoading;
    QString m_hotkeyStatusMessage;
};

#endif // SETTINGSWIDGET_H
