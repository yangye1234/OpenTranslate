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

signals:
    void configSaved(const AppConfig &config);

private slots:
    void onAddPairClicked();
    void onRemovePairClicked();
    void onLanguagePairEdited();
    void onSaveClicked();
    void onAppLanguageChanged(int index);

private:
    static QString normalizePair(const QString &pair);
    void refreshPairList(const QStringList &pairs);
    QStringList currentPairs() const;
    void applyLanguage(AppLanguage language);
    void setupLanguageOptions();

private:
    Ui::SettingsWidget *ui;
    AppLanguage m_uiLanguage;
};

#endif // SETTINGSWIDGET_H
