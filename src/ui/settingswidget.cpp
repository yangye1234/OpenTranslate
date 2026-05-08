#include "settingswidget.h"
#include "ui_settingswidget.h"
#include "l10n.h"
#include "translationcachestore.h"
#include "translationhistorystore.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSet>
#include <QToolButton>
#include <QTextEdit>
#include <QVBoxLayout>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWidget)
    , m_uiLanguage(AppLanguage::SimplifiedChinese)
    , m_isDirty(false)
    , m_isLoading(false)
    , m_providerCombo(nullptr)
    , m_defaultPairCombo(nullptr)
    , m_selectionShortcutEdit(nullptr)
    , m_speechShortcutEdit(nullptr)
    , m_screenshotShortcutEdit(nullptr)
    , m_servicesGroup(nullptr)
    , m_serviceButtons(nullptr)
    , m_deepLGroup(nullptr)
    , m_deepLEnabled(nullptr)
    , m_deepLAuthKey(nullptr)
    , m_deepLBaseUrl(nullptr)
    , m_dictionaryGroup(nullptr)
    , m_dictionaryEnabled(nullptr)
    , m_dataGroup(nullptr)
    , m_lowerOnUnpin(nullptr)
    , m_cacheEnabled(nullptr)
    , m_historyEnabled(nullptr)
    , m_historyMaxEntries(nullptr)
    , m_clearCacheButton(nullptr)
    , m_exportCacheButton(nullptr)
    , m_importCacheButton(nullptr)
    , m_showHistoryButton(nullptr)
    , m_clearHistoryButton(nullptr)
    , m_contentLayout(nullptr)
{
    ui->setupUi(this);
    setupScrollableSettingsUi();
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
    m_speechShortcutEdit->setFocusPolicy(Qt::ClickFocus);
    m_screenshotShortcutEdit->setFocusPolicy(Qt::ClickFocus);
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

    m_providerCombo->setCurrentIndex(static_cast<int>(config.activeProvider));
    if (auto *check = serviceEnabledCheck(ProviderType::Baidu)) {
        check->setChecked(config.baidu.enabled);
    }
    if (auto *check = serviceEnabledCheck(ProviderType::OpenAICompatible)) {
        check->setChecked(config.generic.enabled);
    }
    if (auto *check = serviceEnabledCheck(ProviderType::DeepL)) {
        check->setChecked(config.deepL.enabled);
    }
    if (auto *check = serviceEnabledCheck(ProviderType::Dictionary)) {
        check->setChecked(config.dictionary.enabled);
    }
    updateServiceSelectionUi(config.activeProvider);
    refreshLanguagePairs(config.languagePairs, config.defaultLanguagePair);
    m_cacheEnabled->setChecked(config.cache.enabled);
    m_lowerOnUnpin->setChecked(config.windowBehavior.lowerOnUnpin);
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
    if (shortcuts.toggleSpeech.trimmed().isEmpty()) {
        shortcuts.toggleSpeech = defaults.toggleSpeech;
    }
    if (shortcuts.screenshotTranslate.trimmed().isEmpty()) {
        shortcuts.screenshotTranslate = defaults.screenshotTranslate;
    }

    ui->swapShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.swapLanguage,
                                                                    QKeySequence::PortableText));
    ui->pinShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.toggleOnTop,
                                                                  QKeySequence::PortableText));
    ui->settingsShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.openSettings,
                                                                       QKeySequence::PortableText));
    m_selectionShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.translateSelection,
                                                                      QKeySequence::PortableText));
    m_speechShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.toggleSpeech,
                                                                   QKeySequence::PortableText));
    m_screenshotShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.screenshotTranslate,
                                                                       QKeySequence::PortableText));

    applyLanguage(m_uiLanguage);
    ui->pairEdit->clear();

    m_isLoading = false;
    setDirty(false);
    m_defaultPairCombo->setFocus(Qt::OtherFocusReason);
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
    if (pair.isEmpty()) {
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
    updateDefaultPairOptions(m_defaultPairCombo->currentText());
    setDirty(true);
}

void SettingsWidget::onRemovePairClicked()
{
    delete ui->pairList->takeItem(ui->pairList->currentRow());
    if (ui->pairList->count() == 0) {
        ui->pairList->addItem("zh <> en");
    }
    updateDefaultPairOptions(m_defaultPairCombo->currentText());
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
    if (pair.isEmpty()) {
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
    updateDefaultPairOptions(m_defaultPairCombo->currentText());
    setDirty(true);
}

void SettingsWidget::onSaveClicked()
{
    AppConfig config;
    config.baidu.enabled = serviceEnabledCheck(ProviderType::Baidu)->isChecked();
    config.baidu.appId = ui->baiduAppId->text().trimmed();
    config.baidu.appKey = ui->baiduAppKey->text().trimmed();

    config.generic.enabled = serviceEnabledCheck(ProviderType::OpenAICompatible)->isChecked();
    config.generic.baseUrl = ui->genericBaseUrl->text().trimmed();
    config.generic.model = ui->genericModel->text().trimmed();
    config.generic.apiKey = ui->genericApiKey->text().trimmed();
    config.generic.promptTemplate = ui->genericPrompt->toPlainText().trimmed();

    config.deepL.enabled = serviceEnabledCheck(ProviderType::DeepL)->isChecked();
    config.deepL.authKey = m_deepLAuthKey->text().trimmed();
    config.deepL.baseUrl = m_deepLBaseUrl->text().trimmed();

    config.dictionary.enabled = serviceEnabledCheck(ProviderType::Dictionary)->isChecked();
    config.dictionary.appKey.clear();
    config.dictionary.appSecret.clear();

    const int activeService = m_serviceButtons->checkedId();
    config.activeProvider = activeService >= 0 ? static_cast<ProviderType>(activeService) : ProviderType::Baidu;
    config.languagePairs = currentPairs();
    config.defaultLanguagePair = m_defaultPairCombo->currentText().trimmed();
    if (config.defaultLanguagePair.isEmpty()) {
        config.defaultLanguagePair = config.languagePairs.value(0, "zh <> en");
    }
    config.cache.enabled = m_cacheEnabled->isChecked();
    config.windowBehavior.lowerOnUnpin = m_lowerOnUnpin->isChecked();
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
    config.shortcuts.toggleSpeech = toPortable(m_speechShortcutEdit->keySequence());
    config.shortcuts.screenshotTranslate = toPortable(m_screenshotShortcutEdit->keySequence());

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
    if (config.shortcuts.toggleSpeech.isEmpty()) {
        config.shortcuts.toggleSpeech = defaults.toggleSpeech;
    }
    if (config.shortcuts.screenshotTranslate.isEmpty()) {
        config.shortcuts.screenshotTranslate = defaults.screenshotTranslate;
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
        if (!fixed.isEmpty() && !normalized.contains(fixed)) {
            normalized << fixed;
        }
    }
    if (normalized.isEmpty()) {
        normalized << "zh <> en";
    }
    ui->pairList->addItems(normalized);
}

QStringList SettingsWidget::currentPairs() const
{
    QStringList pairs;
    for (int i = 0; i < ui->pairList->count(); ++i) {
        const QString pair = normalizePair(ui->pairList->item(i)->text());
        if (!pair.isEmpty() && !pairs.contains(pair)) {
            pairs << pair;
        }
    }
    if (pairs.isEmpty()) {
        pairs << "zh <> en";
    }
    return pairs;
}

void SettingsWidget::createExtendedSettingsUi()
{
    m_providerCombo = new QComboBox(this);
    m_providerCombo->addItems({"Baidu", "OpenAI-compatible", "DeepL", "Dictionary"});
    m_providerCombo->hide();

    m_servicesGroup = new QGroupBox(this);
    auto *servicesLayout = new QVBoxLayout(m_servicesGroup);
    servicesLayout->setContentsMargins(12, 14, 12, 14);
    servicesLayout->setSpacing(8);
    m_serviceButtons = new QButtonGroup(m_servicesGroup);
    m_serviceButtons->setExclusive(true);
    m_serviceEnabledChecks.resize(4);
    m_serviceDetailWidgets.resize(4);

    const int serviceIndex = m_contentLayout->indexOf(ui->baiduGroup);
    m_contentLayout->insertWidget(serviceIndex < 0 ? 0 : serviceIndex, m_servicesGroup);

    m_contentLayout->removeWidget(ui->baiduGroup);
    m_contentLayout->removeWidget(ui->genericGroup);
    ui->baiduEnabled->hide();
    ui->genericEnabled->hide();

    m_deepLGroup = new QGroupBox(this);
    auto *deepLLayout = new QFormLayout(m_deepLGroup);
    m_deepLEnabled = new QCheckBox(m_deepLGroup);
    m_deepLEnabled->hide();
    m_deepLAuthKey = new QLineEdit(m_deepLGroup);
    m_deepLAuthKey->setEchoMode(QLineEdit::Password);
    m_deepLBaseUrl = new QLineEdit(m_deepLGroup);
    deepLLayout->addRow(m_deepLEnabled);
    deepLLayout->addRow(new QLabel("Auth Key", m_deepLGroup), m_deepLAuthKey);
    deepLLayout->addRow(new QLabel("Base URL", m_deepLGroup), m_deepLBaseUrl);

    m_dictionaryGroup = new QGroupBox(this);
    auto *dictionaryLayout = new QVBoxLayout(m_dictionaryGroup);
    m_dictionaryEnabled = new QCheckBox(m_dictionaryGroup);
    m_dictionaryEnabled->hide();
    auto *dictionaryNote = new QLabel(m_dictionaryGroup);
    dictionaryNote->setObjectName("labelDictionaryNoApi");
    dictionaryNote->setWordWrap(true);
    dictionaryLayout->addWidget(m_dictionaryEnabled);
    dictionaryLayout->addWidget(dictionaryNote);

    auto addSectionLabel = [servicesLayout, this](const QString &objectName, const QString &text) {
        auto *label = new QLabel(text, m_servicesGroup);
        label->setObjectName(objectName);
        label->setStyleSheet("font-weight: 600; color: palette(mid); margin-top: 4px;");
        servicesLayout->addWidget(label);
    };

    auto addServiceRow = [this, servicesLayout](ProviderType provider,
                                                const QString &name,
                                                const QString &description,
                                                QWidget *detailWidget) {
        auto *container = new QWidget(m_servicesGroup);
        auto *containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(6);

        auto *header = new QFrame(container);
        header->setFrameShape(QFrame::StyledPanel);
        header->setObjectName(QString("serviceHeader%1").arg(static_cast<int>(provider)));

        auto *rowLayout = new QHBoxLayout(header);
        rowLayout->setContentsMargins(12, 10, 12, 10);
        rowLayout->setSpacing(10);

        auto *radio = new QRadioButton(name, header);
        radio->setObjectName(QString("serviceRadio%1").arg(static_cast<int>(provider)));
        radio->setToolTip(description);
        m_serviceButtons->addButton(radio, static_cast<int>(provider));

        auto *detail = new QLabel(description, header);
        detail->setObjectName(QString("serviceDescription%1").arg(static_cast<int>(provider)));
        detail->setWordWrap(true);
        detail->setStyleSheet("color: palette(mid);");

        auto *textLayout = new QVBoxLayout();
        textLayout->setContentsMargins(0, 0, 0, 0);
        textLayout->setSpacing(2);
        textLayout->addWidget(radio);
        textLayout->addWidget(detail);

        auto *enabled = new QCheckBox(L10n::text(m_uiLanguage, "settings.service.enabled"), header);
        enabled->setObjectName(QString("serviceEnabled%1").arg(static_cast<int>(provider)));
        m_serviceEnabledChecks[static_cast<int>(provider)] = enabled;

        auto *configureButton = new QToolButton(header);
        configureButton->setObjectName(QString("serviceConfigure%1").arg(static_cast<int>(provider)));
        configureButton->setText(L10n::text(m_uiLanguage, "settings.service.configure"));
        configureButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

        rowLayout->addLayout(textLayout, 1);
        rowLayout->addWidget(enabled);
        rowLayout->addWidget(configureButton);
        containerLayout->addWidget(header);

        auto *detailContainer = new QWidget(container);
        auto *detailLayout = new QVBoxLayout(detailContainer);
        detailLayout->setContentsMargins(18, 0, 18, 8);
        detailLayout->addWidget(detailWidget);
        detailContainer->setVisible(false);
        m_serviceDetailWidgets[static_cast<int>(provider)] = detailContainer;
        containerLayout->addWidget(detailContainer);
        servicesLayout->addWidget(container);

        connect(radio, &QRadioButton::clicked, this, [this, provider]() {
            if (auto *check = serviceEnabledCheck(provider)) {
                check->setChecked(true);
            }
            m_providerCombo->setCurrentIndex(static_cast<int>(provider));
            updateServiceSelectionUi(provider);
            onAnySettingChanged();
        });
        connect(enabled, &QCheckBox::toggled, this, [this, provider](bool checked) {
            if (provider == ProviderType::Baidu) {
                ui->baiduEnabled->setChecked(checked);
            } else if (provider == ProviderType::OpenAICompatible) {
                ui->genericEnabled->setChecked(checked);
            } else if (provider == ProviderType::DeepL) {
                m_deepLEnabled->setChecked(checked);
            } else if (provider == ProviderType::Dictionary) {
                m_dictionaryEnabled->setChecked(checked);
            }
            onAnySettingChanged();
        });
        connect(configureButton, &QToolButton::clicked, this, [this, provider]() {
            toggleServiceDetails(provider);
        });
    };

    addSectionLabel("labelServiceSectionFree", L10n::text(m_uiLanguage, "settings.service.section.free"));
    addServiceRow(ProviderType::Dictionary,
                  L10n::text(m_uiLanguage, "settings.service.dictionary"),
                  L10n::text(m_uiLanguage, "settings.service.dictionary.desc"),
                  m_dictionaryGroup);
    addSectionLabel("labelServiceSectionApiKey", L10n::text(m_uiLanguage, "settings.service.section.api_key"));
    addServiceRow(ProviderType::Baidu,
                  L10n::text(m_uiLanguage, "settings.service.baidu"),
                  L10n::text(m_uiLanguage, "settings.service.baidu.desc"),
                  ui->baiduGroup);
    addServiceRow(ProviderType::OpenAICompatible,
                  L10n::text(m_uiLanguage, "settings.service.openai"),
                  L10n::text(m_uiLanguage, "settings.service.openai.desc"),
                  ui->genericGroup);
    addServiceRow(ProviderType::DeepL,
                  L10n::text(m_uiLanguage, "settings.service.deepl"),
                  L10n::text(m_uiLanguage, "settings.service.deepl.desc"),
                  m_deepLGroup);

    m_defaultPairCombo = new QComboBox(this);
    auto *labelTargets = new QLabel(this);
    labelTargets->setObjectName("labelTargets");
    ui->formLayoutApp->addRow(labelTargets, m_defaultPairCombo);

    m_lowerOnUnpin = new QCheckBox(this);
    m_lowerOnUnpin->setObjectName("lowerOnUnpin");
    ui->formLayoutApp->addRow(QString(), m_lowerOnUnpin);

    auto *labelSelectionShortcut = new QLabel(this);
    labelSelectionShortcut->setObjectName("labelSelectionShortcut");
    m_selectionShortcutEdit = new QKeySequenceEdit(this);
    ui->formLayoutShortcuts->insertRow(3, labelSelectionShortcut, m_selectionShortcutEdit);

    auto *labelSpeechShortcut = new QLabel(this);
    labelSpeechShortcut->setObjectName("labelSpeechShortcut");
    m_speechShortcutEdit = new QKeySequenceEdit(this);
    ui->formLayoutShortcuts->insertRow(4, labelSpeechShortcut, m_speechShortcutEdit);

    auto *labelScreenshotShortcut = new QLabel(this);
    labelScreenshotShortcut->setObjectName("labelScreenshotShortcut");
    m_screenshotShortcutEdit = new QKeySequenceEdit(this);
    ui->formLayoutShortcuts->insertRow(5, labelScreenshotShortcut, m_screenshotShortcutEdit);

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
    m_contentLayout->insertWidget(m_contentLayout->indexOf(ui->pairGroup), m_dataGroup);

    connect(m_clearCacheButton, &QPushButton::clicked, this, &SettingsWidget::onClearCacheClicked);
    connect(m_exportCacheButton, &QPushButton::clicked, this, &SettingsWidget::onExportCacheClicked);
    connect(m_importCacheButton, &QPushButton::clicked, this, &SettingsWidget::onImportCacheClicked);
    connect(m_showHistoryButton, &QPushButton::clicked, this, &SettingsWidget::onShowHistoryClicked);
    connect(m_clearHistoryButton, &QPushButton::clicked, this, &SettingsWidget::onClearHistoryClicked);
}

void SettingsWidget::setupScrollableSettingsUi()
{
    setMinimumSize(560, 420);
    resize(680, 720);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget(scrollArea);
    m_contentLayout = new QVBoxLayout(content);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(ui->verticalLayout->spacing());

    const QList<QWidget *> groups = {
        ui->appGroup,
        ui->shortcutsGroup,
        ui->baiduGroup,
        ui->genericGroup,
        ui->pairGroup
    };
    for (QWidget *group : groups) {
        ui->verticalLayout->removeWidget(group);
        m_contentLayout->addWidget(group);
    }
    m_contentLayout->addStretch(1);

    scrollArea->setWidget(content);
    ui->verticalLayout->insertWidget(0, scrollArea, 1);
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
    refreshPairList(normalized);
    updateDefaultPairOptions(defaultPair);
}

void SettingsWidget::updateDefaultPairOptions(const QString &selected)
{
    const QString current = selected.trimmed().isEmpty() ? m_defaultPairCombo->currentText() : normalizePair(selected);
    m_defaultPairCombo->clear();
    m_defaultPairCombo->addItems(currentPairs());
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
    if (auto *label = findChild<QLabel *>("labelTargets")) {
        label->setText(L10n::text(language, "settings.label.pairs_default"));
    }

    m_servicesGroup->setTitle(L10n::text(language, "settings.group.services"));
    if (auto *label = findChild<QLabel *>("labelServiceSectionFree")) {
        label->setText(L10n::text(language, "settings.service.section.free"));
    }
    if (auto *label = findChild<QLabel *>("labelServiceSectionApiKey")) {
        label->setText(L10n::text(language, "settings.service.section.api_key"));
    }
    auto setServiceText = [this, language](ProviderType provider,
                                           const QString &nameKey,
                                           const QString &descriptionKey) {
        const int id = static_cast<int>(provider);
        const QString description = L10n::text(language, descriptionKey);
        if (auto *radio = findChild<QRadioButton *>(QString("serviceRadio%1").arg(id))) {
            radio->setText(L10n::text(language, nameKey));
            radio->setToolTip(description);
        }
        if (auto *label = findChild<QLabel *>(QString("serviceDescription%1").arg(id))) {
            label->setText(description);
        }
        if (auto *check = serviceEnabledCheck(provider)) {
            check->setText(L10n::text(language, "settings.service.enabled"));
        }
        if (auto *button = findChild<QToolButton *>(QString("serviceConfigure%1").arg(id))) {
            button->setText(L10n::text(language, "settings.service.configure"));
        }
    };
    setServiceText(ProviderType::Dictionary, "settings.service.dictionary", "settings.service.dictionary.desc");
    setServiceText(ProviderType::Baidu, "settings.service.baidu", "settings.service.baidu.desc");
    setServiceText(ProviderType::OpenAICompatible, "settings.service.openai", "settings.service.openai.desc");
    setServiceText(ProviderType::DeepL, "settings.service.deepl", "settings.service.deepl.desc");

    ui->shortcutsGroup->setTitle(L10n::text(language, "settings.group.shortcuts"));
    ui->labelSwapShortcut->setText(L10n::text(language, "settings.shortcuts.swap"));
    ui->labelPinShortcut->setText(L10n::text(language, "settings.shortcuts.pin"));
    ui->labelSettingsShortcut->setText(L10n::text(language, "settings.shortcuts.settings"));
    if (auto *label = findChild<QLabel *>("labelSelectionShortcut")) {
        label->setText(L10n::text(language, "settings.shortcuts.selection"));
    }
    if (auto *label = findChild<QLabel *>("labelSpeechShortcut")) {
        label->setText(L10n::text(language, "settings.shortcuts.speech"));
    }
    if (auto *label = findChild<QLabel *>("labelScreenshotShortcut")) {
        label->setText(L10n::text(language, "settings.shortcuts.screenshot"));
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
    if (auto *label = findChild<QLabel *>("labelDictionaryNoApi")) {
        label->setText(L10n::text(language, "settings.dictionary.no_api"));
    }

    m_dataGroup->setTitle(L10n::text(language, "settings.group.data"));
    m_lowerOnUnpin->setText(L10n::text(language, "settings.window.lower_on_unpin"));
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
    connect(m_lowerOnUnpin, &QCheckBox::toggled, this, &SettingsWidget::onAnySettingChanged);
    connect(m_deepLEnabled, &QCheckBox::toggled, this, &SettingsWidget::onAnySettingChanged);
    connect(m_deepLAuthKey, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(m_deepLBaseUrl, &QLineEdit::textChanged, this, &SettingsWidget::onAnySettingChanged);
    connect(m_dictionaryEnabled, &QCheckBox::toggled, this, &SettingsWidget::onAnySettingChanged);
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
    connect(m_speechShortcutEdit,
            &QKeySequenceEdit::keySequenceChanged,
            this,
            &SettingsWidget::onAnySettingChanged);
    connect(m_screenshotShortcutEdit,
            &QKeySequenceEdit::keySequenceChanged,
            this,
            &SettingsWidget::onAnySettingChanged);
}

void SettingsWidget::updateServiceSelectionUi(ProviderType provider)
{
    if (!m_serviceButtons) {
        return;
    }

    if (auto *button = m_serviceButtons->button(static_cast<int>(provider))) {
        button->setChecked(true);
    }

    for (int id = 0; id < m_serviceEnabledChecks.size(); ++id) {
        if (auto *row = findChild<QFrame *>(QString("serviceHeader%1").arg(id))) {
            const bool active = id == static_cast<int>(provider);
            row->setStyleSheet(active
                                   ? "QFrame { border: 1px solid #0A84FF; border-radius: 8px; background: #0A84FF; }"
                                     "QFrame QLabel, QFrame QRadioButton, QFrame QCheckBox { border: none; color: white; }"
                                     "QFrame QToolButton { color: white; }"
                                   : "QFrame { border: 1px solid palette(midlight); border-radius: 8px; }"
                                     "QFrame QLabel { border: none; color: palette(mid); }"
                                     "QFrame QRadioButton, QFrame QCheckBox { border: none; }");
        }
    }
}

void SettingsWidget::toggleServiceDetails(ProviderType provider)
{
    const int selected = static_cast<int>(provider);
    if (selected < 0 || selected >= m_serviceDetailWidgets.size()) {
        return;
    }

    QWidget *selectedWidget = m_serviceDetailWidgets.at(selected);
    const bool shouldShow = selectedWidget && !selectedWidget->isVisible();
    for (QWidget *widget : m_serviceDetailWidgets) {
        if (widget) {
            widget->setVisible(false);
        }
    }
    if (selectedWidget) {
        selectedWidget->setVisible(shouldShow);
    }
}

QCheckBox *SettingsWidget::serviceEnabledCheck(ProviderType provider) const
{
    const int index = static_cast<int>(provider);
    if (index < 0 || index >= m_serviceEnabledChecks.size()) {
        return nullptr;
    }
    return m_serviceEnabledChecks.at(index);
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
        canonical(shortcuts.translateSelection),
        canonical(shortcuts.toggleSpeech),
        canonical(shortcuts.screenshotTranslate)
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
