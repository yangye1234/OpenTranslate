#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QVector>

#include "appconfig.h"

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QKeySequenceEdit;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

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
    void setupScrollableSettingsUi();
    void refreshLanguagePairs(const QStringList &pairs, const QString &defaultPair);
    void updateDefaultPairOptions(const QString &selected);
    void toggleServiceDetails(ProviderType provider);
    void applyLanguage(AppLanguage language);
    void setupLanguageOptions();
    void setDirty(bool dirty);
    void updateSaveButtonText();
    void setupDirtyTracking();
    void updateServiceSelectionUi(ProviderType provider);
    QCheckBox *serviceEnabledCheck(ProviderType provider) const;
    static bool hasShortcutConflict(const ShortcutConfig &shortcuts);

private:
    Ui::SettingsWidget *ui;
    AppLanguage m_uiLanguage;
    bool m_isDirty;
    bool m_isLoading;
    QString m_hotkeyStatusMessage;
    QComboBox *m_providerCombo;
    QComboBox *m_defaultPairCombo;
    QKeySequenceEdit *m_selectionShortcutEdit;
    QGroupBox *m_servicesGroup;
    QButtonGroup *m_serviceButtons;
    QVector<QCheckBox *> m_serviceEnabledChecks;
    QVector<QWidget *> m_serviceDetailWidgets;
    QGroupBox *m_deepLGroup;
    QCheckBox *m_deepLEnabled;
    QLineEdit *m_deepLAuthKey;
    QLineEdit *m_deepLBaseUrl;
    QGroupBox *m_dictionaryGroup;
    QCheckBox *m_dictionaryEnabled;
    QGroupBox *m_dataGroup;
    QCheckBox *m_cacheEnabled;
    QCheckBox *m_historyEnabled;
    QLineEdit *m_historyMaxEntries;
    QPushButton *m_clearCacheButton;
    QPushButton *m_exportCacheButton;
    QPushButton *m_importCacheButton;
    QPushButton *m_showHistoryButton;
    QPushButton *m_clearHistoryButton;
    QVBoxLayout *m_contentLayout;
};

#endif // SETTINGSWIDGET_H
