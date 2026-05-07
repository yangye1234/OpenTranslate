#include "settingswidget.h"
#include "ui_settingswidget.h"
#include "l10n.h"
#include "translationcachestore.h"
#include "translationhistorystore.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QTextEdit>
#include <QVBoxLayout>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWidget)
    , m_uiLanguage(AppLanguage::SimplifiedChinese)
    , m_isDirty(false)
    , m_isLoading(false)
    , m_providerCombo(nullptr)
    , m_languagePairsEdit(nullptr)
    , m_defaultPairCombo(nullptr)
    , m_selectionShortcutEdit(nullptr)
    , m_deepLGroup(nullptr)
    , m_deepLEnabled(nullptr)
    , m_deepLAuthKey(nullptr)
    , m_deepLBaseUrl(nullptr)
    , m_dictionaryGroup(nullptr)
    , m_dictionaryEnabled(nullptr)
    , m_dictionaryAppKey(nullptr)
    , m_dictionaryAppSecret(nullptr)
    , m_dataGroup(nullptr)
    , m_cacheEnabled(nullptr)
    , m_historyEnabled(nullptr)
    , m_historyMaxEntries(nullptr)
    , m_clearCacheButton(nullptr)
    , m_exportCacheButton(nullptr)
    , m_importCacheButton(nullptr)
    , m_showHistoryButton(nullptr)
    , m_clearHistoryButton(nullptr)
{
    ui->setupUi(this);
    setupLanguageOptions();
    createExtendedSettingsUi();
    applyLanguage(m_uiLanguage);

    connect(ui->addPairButton, &QPushButton::clicked, this, &SettingsWidget::onAddPairClicked);
    connect(ui->removePairButton, &QPushButton::clicked, this, &SettingsWidget::onRemovePairClicked);
    connect(ui->pairEdit, &QLineEdit::returnPressed, this, &SettingsWidget::onLanguagePairEdited);
    connect(ui->editPairButton, &QPushButton::clicked, this, &SettingsWidget::onLanguagePairEdited);
    connect(ui->saveButton, &QPushButton::clicked, this, &SettingsWidget::onSaveClicked);
    connect(ui->appLanguageCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &SettingsWidget::onAppLanguageChanged);

    ui->swapShortcutEdit->setFocusPolicy(Qt::ClickFocus);
    ui->pinShortcutEdit->setFocusPolicy(Qt::ClickFocus);
    ui->settingsShortcutEdit->setFocusPolicy(Qt::ClickFocus);
    m_selectionShortcutEdit->setFocusPolicy(Qt::ClickFocus);
    ui->hotkeyStatusLabel->setVisible(false);

    setupDirtyTracking();
    setDirty(false);
}

SettingsWidget::~SettingsWidget()
{
    delete ui;
}

void SettingsWidget::setConfig(const AppConfig &config)
{
    m_isLoading = true;

    ui->baiduEnabled->setChecked(config.baidu.enabled);
    ui->baiduAppId->setText(config.baidu.appId);
    ui->baiduAppKey->setText(config.baidu.appKey);

    ui->genericEnabled->setChecked(config.generic.enabled);
    ui->genericBaseUrl->setText(config.generic.baseUrl);
    ui->genericModel->setText(config.generic.model);
    ui->genericApiKey->setText(config.generic.apiKey);
    ui->genericPrompt->setPlainText(config.generic.promptTemplate);

    m_deepLEnabled->setChecked(config.deepL.enabled);
    m_deepLAuthKey->setText(config.deepL.authKey);
    m_deepLBaseUrl->setText(config.deepL.baseUrl);

    m_dictionaryEnabled->setChecked(config.dictionary.enabled);
    m_dictionaryAppKey->setText(config.dictionary.appKey);
    m_dictionaryAppSecret->setText(config.dictionary.appSecret);

    m_providerCombo->setCurrentIndex(static_cast<int>(config.activeProvider));
    refreshLanguagePairs(config.languagePairs, config.defaultLanguagePair);
    m_cacheEnabled->setChecked(config.cache.enabled);
    m_historyEnabled->setChecked(config.history.enabled);
    m_historyMaxEntries->setText(QString::number(config.history.maxEntries));

    ui->appLanguageCombo->setCurrentIndex(static_cast<int>(config.appLanguage));
    m_uiLanguage = config.appLanguage;

    ShortcutConfig shortcuts = config.shortcuts;
    const ShortcutConfig defaults = defaultShortcutsForCurrentPlatform();
    if (shortcuts.swapLanguage.trimmed().isEmpty()) {
        shortcuts.swapLanguage = defaults.swapLanguage;
    }
    if (shortcuts.toggleOnTop.trimmed().isEmpty()) {
        shortcuts.toggleOnTop = defaults.toggleOnTop;
    }
    if (shortcuts.openSettings.trimmed().isEmpty()) {
        shortcuts.openSettings = defaults.openSettings;
    }
    if (shortcuts.translateSelection.trimmed().isEmpty()) {
        shortcuts.translateSelection = defaults.translateSelection;
    }

    ui->swapShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.swapLanguage,
                                                                    QKeySequence::PortableText));
    ui->pinShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.toggleOnTop,
                                                                  QKeySequence::PortableText));
    ui->settingsShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.openSettings,
                                                                       QKeySequence::PortableText));
    m_selectionShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.translateSelection,
                                                                      QKeySequence::PortableText));

    applyLanguage(m_uiLanguage);
    refreshPairList(config.languagePairs);
    ui->pairEdit->clear();

    m_isLoading = false;
    setDirty(false);
    ui->baiduAppId->setFocus(Qt::OtherFocusReason);
}

void SettingsWidget::setHotkeyStatusMessage(const QString &message)
{
    m_hotkeyStatusMessage = message.trimmed();
    ui->hotkeyStatusLabel->setText(m_hotkeyStatusMessage);
    ui->hotkeyStatusLabel->setVisible(!m_hotkeyStatusMessage.isEmpty());
}

void SettingsWidget::onAddPairClicked()
{
    const QString pair = normalizePair(ui->pairEdit->text());
    if (pair.isEmpty() || !pair.contains("->")) {
        QMessageBox::warning(this,
                             L10n::text(m_uiLanguage, "settings.error.invalid_pair.title"),
                             L10n::text(m_uiLanguage, "settings.error.invalid_pair.body"));
        return;
    }

    for (int i = 0; i < ui->pairList->count(); ++i) {
        if (ui->pairList->item(i)->text() == pair) {
            QMessageBox::information(this,
                                     L10n::text(m_uiLanguage, "settings.error.duplicate.title"),
                                     L10n::text(m_uiLanguage, "settings.error.duplicate.body"));
            return;
        }
    }

    ui->pairList->addItem(pair);
    ui->pairEdit->clear();
    setDirty(true);
}

void SettingsWidget::onRemovePairClicked()
{
    delete ui->pairList->takeItem(ui->pairList->currentRow());
    if (ui->pairList->count() == 0) {
        ui->pairList->addItem("en->zh");
        ui->pairList->addItem("zh->en");
    }
    setDirty(true);
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
        QMessageBox::warning(this,
                             L10n::text(m_uiLanguage, "settings.error.invalid_pair.title"),
                             L10n::text(m_uiLanguage, "settings.error.invalid_pair.body"));
        return;
    }

    for (int i = 0; i < ui->pairList->count(); ++i) {
        if (ui->pairList->item(i) != item && ui->pairList->item(i)->text() == pair) {
            QMessageBox::information(this,
                                     L10n::text(m_uiLanguage, "settings.error.duplicate.title"),
                                     L10n::text(m_uiLanguage, "settings.error.duplicate.body"));
            return;
        }
    }

    item->setText(pair);
    ui->pairEdit->clear();
    setDirty(true);
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

    config.deepL.enabled = m_deepLEnabled->isChecked();
    config.deepL.authKey = m_deepLAuthKey->text().trimmed();
    config.deepL.baseUrl = m_deepLBaseUrl->text().trimmed();

    config.dictionary.enabled = m_dictionaryEnabled->isChecked();
    config.dictionary.appKey = m_dictionaryAppKey->text().trimmed();
    config.dictionary.appSecret = m_dictionaryAppSecret->text().trimmed();

    config.activeProvider = static_cast<ProviderType>(m_providerCombo->currentIndex());
    config.languagePairs = currentLanguagePairsFromEdit();
    config.defaultLanguagePair = m_defaultPairCombo->currentText().trimmed();
    if (config.defaultLanguagePair.isEmpty()) {
        config.defaultLanguagePair = config.languagePairs.value(0, "zh <> en");
    }
    config.cache.enabled = m_cacheEnabled->isChecked();
    config.history.enabled = m_historyEnabled->isChecked();
    config.history.maxEntries = m_historyMaxEntries->text().trimmed().toInt();
    if (config.history.maxEntries <= 0) {
        config.history.maxEntries = 200;
    }
    config.appLanguage = static_cast<AppLanguage>(ui->appLanguageCombo->currentIndex());

    const ShortcutConfig defaults = defaultShortcutsForCurrentPlatform();
    auto toPortable = [](const QKeySequence &seq) {
        return seq.toString(QKeySequence::PortableText).trimmed();
    };

    config.shortcuts.swapLanguage = toPortable(ui->swapShortcutEdit->keySequence());
    config.shortcuts.toggleOnTop = toPortable(ui->pinShortcutEdit->keySequence());
    config.shortcuts.openSettings = toPortable(ui->settingsShortcutEdit->keySequence());
    config.shortcuts.translateSelection = toPortable(m_selectionShortcutEdit->keySequence());

    if (config.shortcuts.swapLanguage.isEmpty()) {
        config.shortcuts.swapLanguage = defaults.swapLanguage;
    }
    if (config.shortcuts.toggleOnTop.isEmpty()) {
        config.shortcuts.toggleOnTop = defaults.toggleOnTop;
    }
    if (config.shortcuts.openSettings.isEmpty()) {
        config.shortcuts.openSettings = defaults.openSettings;
    }
    if (config.shortcuts.translateSelection.isEmpty()) {
        config.shortcuts.translateSelection = defaults.translateSelection;
    }

    if (hasShortcutConflict(config.shortcuts)) {
        QMessageBox::warning(this,
                             L10n::text(m_uiLanguage, "settings.error.shortcut_conflict.title"),
                             L10n::text(m_uiLanguage, "settings.error.shortcut_conflict.body"));
        return;
    }

    emit configSaved(config);
    setDirty(false);
}

void SettingsWidget::onAppLanguageChanged(int index)
{
    m_uiLanguage = static_cast<AppLanguage>(index);
    applyLanguage(m_uiLanguage);
    onAnySettingChanged();
}

void SettingsWidget::onAnySettingChanged()
{
    if (m_isLoading) {
        return;
    }
    setDirty(true);
}

void SettingsWidget::onLanguagePairsEdited()
{
    updateDefaultPairOptions(m_defaultPairCombo->currentText());
    onAnySettingChanged();
}

void SettingsWidget::onClearCacheClicked()
{
    if (QMessageBox::question(this,
                              L10n::text(m_uiLanguage, "settings.cache.clear.title"),
                              L10n::text(m_uiLanguage, "settings.cache.clear.body"))
        != QMessageBox::Yes) {
        return;
    }
    const bool ok = TranslationCacheStore().clear();
    QMessageBox::information(this,
                             L10n::text(m_uiLanguage, ok ? "settings.operation.done" : "settings.operation.failed"),
                             ok ? L10n::text(m_uiLanguage, "settings.cache.clear.done")
                                : L10n::text(m_uiLanguage, "settings.cache.clear.failed"));
}

void SettingsWidget::onExportCacheClicked()
{
    const QString filePath = QFileDialog::getSaveFileName(this,
                                                          L10n::text(m_uiLanguage, "settings.cache.export"),
                                                          "translations_cache.json",
                                                          "JSON (*.json)");
    if (filePath.isEmpty()) {
        return;
    }
    const bool ok = TranslationCacheStore().exportTo(filePath);
    QMessageBox::information(this,
                             L10n::text(m_uiLanguage, ok ? "settings.operation.done" : "settings.operation.failed"),
                             ok ? L10n::text(m_uiLanguage, "settings.cache.export.done")
                                : L10n::text(m_uiLanguage, "settings.cache.export.failed"));
}

void SettingsWidget::onImportCacheClicked()
{
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          L10n::text(m_uiLanguage, "settings.cache.import"),
                                                          QString(),
                                                          "JSON (*.json)");
    if (filePath.isEmpty()) {
        return;
    }
    const bool ok = TranslationCacheStore().importFrom(filePath);
    QMessageBox::information(this,
                             L10n::text(m_uiLanguage, ok ? "settings.operation.done" : "settings.operation.failed"),
                             ok ? L10n::text(m_uiLanguage, "settings.cache.import.done")
                                : L10n::text(m_uiLanguage, "settings.cache.import.failed"));
}

void SettingsWidget::onShowHistoryClicked()
{
    const QList<TranslationHistoryEntry> entries = TranslationHistoryStore().entries();
    QStringList lines;
    for (const TranslationHistoryEntry &entry : entries) {
        lines << QString("[%1] %2 %3->%4\n%5\n=> %6\n")
                     .arg(entry.createdAt,
                          entry.provider,
                          entry.sourceLanguage,
                          entry.targetLanguage,
                          entry.sourceText,
                          entry.translatedText);
    }

    QDialog dialog(this);
    dialog.setWindowTitle(L10n::text(m_uiLanguage, "settings.history.view"));
    dialog.resize(720, 480);
    auto *layout = new QVBoxLayout(&dialog);
    auto *textEdit = new QTextEdit(&dialog);
    textEdit->setReadOnly(true);
    textEdit->setPlainText(lines.isEmpty() ? L10n::text(m_uiLanguage, "settings.history.empty") : lines.join("\n"));
    layout->addWidget(textEdit);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto *copyButton = buttons->addButton(L10n::text(m_uiLanguage, "settings.history.copy_all"), QDialogButtonBox::ActionRole);
    connect(copyButton, &QPushButton::clicked, textEdit, &QTextEdit::selectAll);
    connect(copyButton, &QPushButton::clicked, textEdit, &QTextEdit::copy);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}

void SettingsWidget::onClearHistoryClicked()
{
    if (QMessageBox::question(this,
                              L10n::text(m_uiLanguage, "settings.history.clear.title"),
                              L10n::text(m_uiLanguage, "settings.history.clear.body"))
        != QMessageBox::Yes) {
        return;
    }
    const bool ok = TranslationHistoryStore().clear();
    QMessageBox::information(this,
                             L10n::text(m_uiLanguage, ok ? "settings.operation.done" : "settings.operation.failed"),
                             ok ? L10n::text(m_uiLanguage, "settings.history.clear.done")
                                : L10n::text(m_uiLanguage, "settings.history.clear.failed"));
}

QString SettingsWidget::normalizePair(const QString &pair)
{
    QString normalized = pair.trimmed();
    normalized.replace("-->", "->");
    normalized.replace("<->", "<>");
    normalized.replace("<=>", "<>");
    normalized.remove(' ');
    QStringList parts;
    if (normalized.contains("<>")) {
        parts = normalized.split("<>", Qt::SkipEmptyParts);
    } else if (normalized.contains("->")) {
        parts = normalized.split("->", Qt::SkipEmptyParts);
    }
    if (parts.size() != 2) {
        return {};
    }
    const QString left = parts.at(0).trimmed();
    const QString right = parts.at(1).trimmed();
    if (left.isEmpty() || right.isEmpty() || left == right) {
        return {};
    }
    return left < right ? left + " <> " + right : right + " <> " + left;
}

QString SettingsWidget::normalizeLanguageCode(const QString &code)
{
    return code.trimmed();
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

void SettingsWidget::createExtendedSettingsUi()
{
    m_providerCombo = new QComboBox(this);
    m_providerCombo->addItems({"Baidu", "OpenAI-compatible", "DeepL", "Dictionary"});
    auto *labelProvider = new QLabel(this);
    labelProvider->setObjectName("labelProvider");
    labelProvider->setText(L10n::text(m_uiLanguage, "settings.label.provider"));
    ui->formLayoutApp->addRow(labelProvider, m_providerCombo);

    m_languagePairsEdit = new QLineEdit(this);
    m_defaultPairCombo = new QComboBox(this);
    auto *targetWrapper = new QWidget(this);
    auto *targetLayout = new QHBoxLayout(targetWrapper);
    targetLayout->setContentsMargins(0, 0, 0, 0);
    targetLayout->addWidget(m_languagePairsEdit, 2);
    targetLayout->addWidget(m_defaultPairCombo, 1);
    auto *labelTargets = new QLabel(this);
    labelTargets->setObjectName("labelTargets");
    ui->formLayoutApp->addRow(labelTargets, targetWrapper);

    auto *labelSelectionShortcut = new QLabel(this);
    labelSelectionShortcut->setObjectName("labelSelectionShortcut");
    m_selectionShortcutEdit = new QKeySequenceEdit(this);
    ui->formLayoutShortcuts->insertRow(3, labelSelectionShortcut, m_selectionShortcutEdit);

    m_deepLGroup = new QGroupBox(this);
    auto *deepLLayout = new QFormLayout(m_deepLGroup);
    m_deepLEnabled = new QCheckBox(m_deepLGroup);
    m_deepLAuthKey = new QLineEdit(m_deepLGroup);
    m_deepLAuthKey->setEchoMode(QLineEdit::Password);
    m_deepLBaseUrl = new QLineEdit(m_deepLGroup);
    deepLLayout->addRow(m_deepLEnabled);
    deepLLayout->addRow(new QLabel("Auth Key", m_deepLGroup), m_deepLAuthKey);
    deepLLayout->addRow(new QLabel("Base URL", m_deepLGroup), m_deepLBaseUrl);
    ui->verticalLayout->insertWidget(ui->verticalLayout->indexOf(ui->pairGroup), m_deepLGroup);

    m_dictionaryGroup = new QGroupBox(this);
    auto *dictionaryLayout = new QFormLayout(m_dictionaryGroup);
    m_dictionaryEnabled = new QCheckBox(m_dictionaryGroup);
    m_dictionaryAppKey = new QLineEdit(m_dictionaryGroup);
    m_dictionaryAppSecret = new QLineEdit(m_dictionaryGroup);
    m_dictionaryAppSecret->setEchoMode(QLineEdit::Password);
    dictionaryLayout->addRow(m_dictionaryEnabled);
    dictionaryLayout->addRow(new QLabel("App Key", m_dictionaryGroup), m_dictionaryAppKey);
    dictionaryLayout->addRow(new QLabel("App Secret", m_dictionaryGroup), m_dictionaryAppSecret);
    ui->verticalLayout->insertWidget(ui->verticalLayout->indexOf(ui->pairGroup), m_dictionaryGroup);

    m_dataGroup = new QGroupBox(this);
    auto *dataLayout = new QVBoxLayout(m_dataGroup);
    m_cacheEnabled = new QCheckBox(m_dataGroup);
    m_historyEnabled = new QCheckBox(m_dataGroup);
    m_historyMaxEntries = new QLineEdit(m_dataGroup);
    m_historyMaxEntries->setPlaceholderText("200");
    dataLayout->addWidget(m_cacheEnabled);
    dataLayout->addWidget(m_historyEnabled);
    auto *historyLimitLayout = new QHBoxLayout();
    auto *historyLimitLabel = new QLabel(m_dataGroup);
    historyLimitLabel->setObjectName("labelHistoryLimit");
    historyLimitLayout->addWidget(historyLimitLabel);
    historyLimitLayout->addWidget(m_historyMaxEntries);
    dataLayout->addLayout(historyLimitLayout);

    auto *cacheButtons = new QHBoxLayout();
    m_clearCacheButton = new QPushButton(m_dataGroup);
    m_exportCacheButton = new QPushButton(m_dataGroup);
    m_importCacheButton = new QPushButton(m_dataGroup);
    cacheButtons->addWidget(m_clearCacheButton);
    cacheButtons->addWidget(m_exportCacheButton);
    cacheButtons->addWidget(m_importCacheButton);
    dataLayout->addLayout(cacheButtons);

    auto *historyButtons = new QHBoxLayout();
    m_showHistoryButton = new QPushButton(m_dataGroup);
    m_clearHistoryButton = new QPushButton(m_dataGroup);
    historyButtons->addWidget(m_showHistoryButton);
    historyButtons->addWidget(m_clearHistoryButton);
    dataLayout->addLayout(historyButtons);
    ui->verticalLayout->insertWidget(ui->verticalLayout->indexOf(ui->pairGroup), m_dataGroup);

    connect(m_languagePairsEdit, &QLineEdit::textChanged, this, &SettingsWidget::onLanguagePairsEdited);
    connect(m_clearCacheButton, &QPushButton::clicked, this, &SettingsWidget::onClearCacheClicked);
    connect(m_exportCacheButton, &QPushButton::clicked, this, &SettingsWidget::onExportCacheClicked);
    connect(m_importCacheButton, &QPushButton::clicked, this, &SettingsWidget::onImportCacheClicked);
    connect(m_showHistoryButton, &QPushButton::clicked, this, &SettingsWidget::onShowHistoryClicked);
    connect(m_clearHistoryButton, &QPushButton::clicked, this, &SettingsWidget::onClearHistoryClicked);
}

void SettingsWidget::refreshLanguagePairs(const QStringList &pairs, const QString &defaultPair)
{
    QStringList normalized;
    for (const QString &pair : pairs) {
        const QString code = normalizePair(pair);
        if (!code.isEmpty() && !normalized.contains(code)) {
            normalized << code;
        }
    }
    if (normalized.isEmpty()) {
        normalized << "zh <> en";
    }
    m_languagePairsEdit->setText(normalized.join(", "));
    updateDefaultPairOptions(defaultPair);
}

QStringList SettingsWidget::currentLanguagePairsFromEdit() const
{
    QStringList out;
    const QStringList parts = m_languagePairsEdit->text().split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString code = normalizePair(part);
        if (!code.isEmpty() && !out.contains(code)) {
            out << code;
        }
    }
    if (out.isEmpty()) {
        out << "zh <> en";
    }
    return out;
}

void SettingsWidget::updateDefaultPairOptions(const QString &selected)
{
    const QString current = selected.trimmed().isEmpty() ? m_defaultPairCombo->currentText() : normalizePair(selected);
    m_defaultPairCombo->clear();
    m_defaultPairCombo->addItems(currentLanguagePairsFromEdit());
    int index = m_defaultPairCombo->findText(current);
    if (index < 0) {
        index = 0;
    }
    m_defaultPairCombo->setCurrentIndex(index);
}

void SettingsWidget::applyLanguage(AppLanguage language)
{
    setWindowTitle(L10n::text(language, "settings.title"));
    ui->appGroup->setTitle(L10n::text(language, "settings.group.app"));
    ui->labelAppLanguage->setText(L10n::text(language, "settings.label.app_language"));
    if (auto *label = findChild<QLabel *>("labelProvider")) {
        label->setText(L10n::text(language, "settings.label.provider"));
    }
    if (auto *label = findChild<QLabel *>("labelTargets")) {
        label->setText(L10n::text(language, "settings.label.pairs_default"));
    }

    ui->shortcutsGroup->setTitle(L10n::text(language, "settings.group.shortcuts"));
    ui->labelSwapShortcut->setText(L10n::text(language, "settings.shortcuts.swap"));
    ui->labelPinShortcut->setText(L10n::text(language, "settings.shortcuts.pin"));
    ui->labelSettingsShortcut->setText(L10n::text(language, "settings.shortcuts.settings"));
    if (auto *label = findChild<QLabel *>("labelSelectionShortcut")) {
        label->setText(L10n::text(language, "settings.shortcuts.selection"));
    }
    ui->hotkeyStatusLabel->setText(m_hotkeyStatusMessage);
    ui->hotkeyStatusLabel->setVisible(!m_hotkeyStatusMessage.isEmpty());

    ui->baiduGroup->setTitle(L10n::text(language, "settings.group.baidu"));
    ui->baiduEnabled->setText(L10n::text(language, "settings.baidu.enabled"));
    ui->labelBaiduAppId->setText(L10n::text(language, "settings.baidu.app_id"));
    ui->labelBaiduAppKey->setText(L10n::text(language, "settings.baidu.app_key"));

    ui->genericGroup->setTitle(L10n::text(language, "settings.group.generic"));
    ui->genericEnabled->setText(L10n::text(language, "settings.generic.enabled"));
    ui->labelGenericBaseUrl->setText(L10n::text(language, "settings.generic.base_url"));
    ui->labelGenericModel->setText(L10n::text(language, "settings.generic.model"));
    ui->labelGenericApiKey->setText(L10n::text(language, "settings.generic.api_key"));
    ui->labelGenericPrompt->setText(L10n::text(language, "settings.generic.prompt"));

    m_deepLGroup->setTitle(L10n::text(language, "settings.group.deepl"));
    m_deepLEnabled->setText(L10n::text(language, "settings.deepl.enabled"));

    m_dictionaryGroup->setTitle(L10n::text(language, "settings.group.dictionary"));
    m_dictionaryEnabled->setText(L10n::text(language, "settings.dictionary.enabled"));

    m_dataGroup->setTitle(L10n::text(language, "settings.group.data"));
    m_cacheEnabled->setText(L10n::text(language, "settings.cache.enabled"));
    m_historyEnabled->setText(L10n::text(language, "settings.history.enabled"));
    if (auto *label = findChild<QLabel *>("labelHistoryLimit")) {
        label->setText(L10n::text(language, "settings.history.max_entries"));
    }
    m_clearCacheButton->setText(L10n::text(language, "settings.cache.clear"));
    m_exportCacheButton->setText(L10n::text(language, "settings.cache.export"));
    m_importCacheButton->setText(L10n::text(language, "settings.cache.import"));
    m_showHistoryButton->setText(L10n::text(language, "settings.history.view"));
    m_clearHistoryButton->setText(L10n::text(language, "settings.history.clear"));

    ui->pairGroup->setTitle(L10n::text(language, "settings.group.pairs"));
    ui->pairEdit->setPlaceholderText(L10n::text(language, "settings.pairs.placeholder"));
    ui->addPairButton->setText(L10n::text(language, "settings.pairs.add"));
    ui->editPairButton->setText(L10n::text(language, "settings.pairs.edit"));
    ui->removePairButton->setText(L10n::text(language, "settings.pairs.remove"));

    updateSaveButtonText();
}

void SettingsWidget::setupLanguageOptions()
{
    ui->appLanguageCombo->clear();
    ui->appLanguageCombo->addItem(L10n::text(AppLanguage::English, "language.english"));
    ui->appLanguageCombo->addItem(L10n::text(AppLanguage::SimplifiedChinese, "language.zh_cn"));
    ui->appLanguageCombo->addItem(L10n::text(AppLanguage::TraditionalChinese, "language.zh_tw"));
}

void SettingsWidget::setDirty(bool dirty)
{
    m_isDirty = dirty;
    updateSaveButtonText();
}

void SettingsWidget::updateSaveButtonText()
{
    ui->saveButton->setText(L10n::text(m_uiLanguage, m_isDirty ? "settings.save" : "settings.saved"));
}

void SettingsWidget::setupDirtyTracking()
{
    connect(ui->baiduEnabled, &QCheckBox::toggled, this, &SettingsWidget::onAnySettingChanged);
    connect(ui->baiduAppId, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(ui->baiduAppKey, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);

    connect(ui->genericEnabled, &QCheckBox::toggled, this, &SettingsWidget::onAnySettingChanged);
    connect(ui->genericBaseUrl, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(ui->genericModel, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(ui->genericApiKey, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(ui->genericPrompt, &QPlainTextEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);

    connect(m_providerCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsWidget::onAnySettingChanged);
    connect(m_defaultPairCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsWidget::onAnySettingChanged);
    connect(m_deepLEnabled, &QCheckBox::toggled, this, &SettingsWidget::onAnySettingChanged);
    connect(m_deepLAuthKey, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(m_deepLBaseUrl, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(m_dictionaryEnabled, &QCheckBox::toggled, this, &SettingsWidget::onAnySettingChanged);
    connect(m_dictionaryAppKey, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(m_dictionaryAppSecret, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(m_cacheEnabled, &QCheckBox::toggled, this, &SettingsWidget::onAnySettingChanged);
    connect(m_historyEnabled, &QCheckBox::toggled, this, &SettingsWidget::onAnySettingChanged);
    connect(m_historyMaxEntries, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);

    connect(ui->pairEdit, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(ui->swapShortcutEdit,
            &QKeySequenceEdit::keySequenceChanged,
            this,
            &SettingsWidget::onAnySettingChanged);
    connect(ui->pinShortcutEdit,
            &QKeySequenceEdit::keySequenceChanged,
            this,
            &SettingsWidget::onAnySettingChanged);
    connect(ui->settingsShortcutEdit,
            &QKeySequenceEdit::keySequenceChanged,
            this,
            &SettingsWidget::onAnySettingChanged);
    connect(m_selectionShortcutEdit,
            &QKeySequenceEdit::keySequenceChanged,
            this,
            &SettingsWidget::onAnySettingChanged);
}

bool SettingsWidget::hasShortcutConflict(const ShortcutConfig &shortcuts)
{
    auto canonical = [](const QString &shortcut) {
        return QKeySequence::fromString(shortcut, QKeySequence::PortableText)
            .toString(QKeySequence::PortableText)
            .trimmed();
    };

    QSet<QString> seen;
    const QStringList values = {
        canonical(shortcuts.swapLanguage),
        canonical(shortcuts.toggleOnTop),
        canonical(shortcuts.openSettings),
        canonical(shortcuts.translateSelection)
    };

    for (const QString &value : values) {
        if (value.isEmpty()) {
            continue;
        }
        if (seen.contains(value)) {
            return true;
        }
        seen.insert(value);
    }
    return false;
}
