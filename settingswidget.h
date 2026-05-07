#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>

#include "appconfig.h"

class QCheckBox;
class QComboBox;
class QGroupBox;
class QKeySequenceEdit;
class QLineEdit;
class QPushButton;

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
    void onTargetLanguagesEdited();
    void onClearCacheClicked();
    void onExportCacheClicked();
    void onImportCacheClicked();
    void onShowHistoryClicked();
    void onClearHistoryClicked();

private:
    static QString normalizePair(const QString &pair);
    static QString normalizeLanguageCode(const QString &code);
    void refreshPairList(const QStringList &pairs);
    QStringList currentPairs() const;
    void createExtendedSettingsUi();
    void refreshTargetLanguages(const QStringList &languages, const QString &defaultLanguage);
    QStringList currentTargetLanguages() const;
    void updateDefaultTargetOptions(const QString &selected);
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
    QComboBox *m_providerCombo;
    QLineEdit *m_targetLanguagesEdit;
    QComboBox *m_defaultTargetCombo;
    QKeySequenceEdit *m_selectionShortcutEdit;
    QGroupBox *m_deepLGroup;
    QCheckBox *m_deepLEnabled;
    QLineEdit *m_deepLAuthKey;
    QLineEdit *m_deepLBaseUrl;
    QGroupBox *m_dictionaryGroup;
    QCheckBox *m_dictionaryEnabled;
    QLineEdit *m_dictionaryAppKey;
    QLineEdit *m_dictionaryAppSecret;
    QGroupBox *m_dataGroup;
    QCheckBox *m_cacheEnabled;
    QCheckBox *m_historyEnabled;
    QLineEdit *m_historyMaxEntries;
    QPushButton *m_clearCacheButton;
    QPushButton *m_exportCacheButton;
    QPushButton *m_importCacheButton;
    QPushButton *m_showHistoryButton;
    QPushButton *m_clearHistoryButton;
};

#endif // SETTINGSWIDGET_H
