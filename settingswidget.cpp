#include "settingswidget.h"
#include "ui_settingswidget.h"

#include <QMessageBox>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWidget)
{
    ui->setupUi(this);
    setWindowTitle("Settings");

    connect(ui->addPairButton, &QPushButton::clicked, this, &SettingsWidget::onAddPairClicked);
    connect(ui->removePairButton, &QPushButton::clicked, this, &SettingsWidget::onRemovePairClicked);
    connect(ui->pairEdit, &QLineEdit::returnPressed, this, &SettingsWidget::onLanguagePairEdited);
    connect(ui->editPairButton, &QPushButton::clicked, this, &SettingsWidget::onLanguagePairEdited);
    connect(ui->saveButton, &QPushButton::clicked, this, &SettingsWidget::onSaveClicked);
}

SettingsWidget::~SettingsWidget()
{
    delete ui;
}

void SettingsWidget::setConfig(const AppConfig &config)
{
    ui->baiduEnabled->setChecked(config.baidu.enabled);
    ui->baiduAppId->setText(config.baidu.appId);
    ui->baiduAppKey->setText(config.baidu.appKey);

    ui->genericEnabled->setChecked(config.generic.enabled);
    ui->genericBaseUrl->setText(config.generic.baseUrl);
    ui->genericModel->setText(config.generic.model);
    ui->genericApiKey->setText(config.generic.apiKey);
    ui->genericPrompt->setPlainText(config.generic.promptTemplate);

    refreshPairList(config.languagePairs);
    ui->pairEdit->clear();
}

void SettingsWidget::onAddPairClicked()
{
    const QString pair = normalizePair(ui->pairEdit->text());
    if (pair.isEmpty() || !pair.contains("->")) {
        QMessageBox::warning(this, "Invalid language pair", "Use format like en->zh.");
        return;
    }

    for (int i = 0; i < ui->pairList->count(); ++i) {
        if (ui->pairList->item(i)->text() == pair) {
            QMessageBox::information(this, "Duplicate", "This language pair already exists.");
            return;
        }
    }

    ui->pairList->addItem(pair);
    ui->pairEdit->clear();
}

void SettingsWidget::onRemovePairClicked()
{
    delete ui->pairList->takeItem(ui->pairList->currentRow());
    if (ui->pairList->count() == 0) {
        ui->pairList->addItem("en->zh");
        ui->pairList->addItem("zh->en");
    }
}

void SettingsWidget::onLanguagePairEdited()
{
    QListWidgetItem *item = ui->pairList->currentItem();
    if (!item) {
        onAddPairClicked();
        return;
    }

    const QString pair = normalizePair(ui->pairEdit->text());
    if (pair.isEmpty() || !pair.contains("->")) {
        QMessageBox::warning(this, "Invalid language pair", "Use format like en->zh.");
        return;
    }

    for (int i = 0; i < ui->pairList->count(); ++i) {
        if (ui->pairList->item(i) != item && ui->pairList->item(i)->text() == pair) {
            QMessageBox::information(this, "Duplicate", "This language pair already exists.");
            return;
        }
    }

    item->setText(pair);
    ui->pairEdit->clear();
}

void SettingsWidget::onSaveClicked()
{
    AppConfig config;
    config.baidu.enabled = ui->baiduEnabled->isChecked();
    config.baidu.appId = ui->baiduAppId->text().trimmed();
    config.baidu.appKey = ui->baiduAppKey->text().trimmed();

    config.generic.enabled = ui->genericEnabled->isChecked();
    config.generic.baseUrl = ui->genericBaseUrl->text().trimmed();
    config.generic.model = ui->genericModel->text().trimmed();
    config.generic.apiKey = ui->genericApiKey->text().trimmed();
    config.generic.promptTemplate = ui->genericPrompt->toPlainText().trimmed();

    config.activeProvider = ProviderType::Baidu;
    config.languagePairs = currentPairs();

    emit configSaved(config);
}

QString SettingsWidget::normalizePair(const QString &pair)
{
    QString normalized = pair.trimmed();
    normalized.replace("-->", "->");
    normalized.remove(' ');
    return normalized;
}

void SettingsWidget::refreshPairList(const QStringList &pairs)
{
    ui->pairList->clear();
    QStringList normalized;
    for (const QString &pair : pairs) {
        const QString fixed = normalizePair(pair);
        if (!fixed.isEmpty() && fixed.contains("->") && !normalized.contains(fixed)) {
            normalized << fixed;
        }
    }
    if (normalized.isEmpty()) {
        normalized << "en->zh" << "zh->en";
    }
    ui->pairList->addItems(normalized);
}

QStringList SettingsWidget::currentPairs() const
{
    QStringList pairs;
    for (int i = 0; i < ui->pairList->count(); ++i) {
        const QString pair = normalizePair(ui->pairList->item(i)->text());
        if (!pair.isEmpty() && pair.contains("->") && !pairs.contains(pair)) {
            pairs << pair;
        }
    }
    if (pairs.isEmpty()) {
        pairs << "en->zh" << "zh->en";
    }
    return pairs;
}
