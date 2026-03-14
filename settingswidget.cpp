#include "settingswidget.h"
#include "ui_settingswidget.h"
#include "l10n.h"

#include <QComboBox>
#include <QKeySequenceEdit>
#include <QMessageBox>
#include <QSet>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWidget)
    , m_uiLanguage(AppLanguage::SimplifiedChinese)
    , m_isDirty(false)
    , m_isLoading(false)
{
    ui->setupUi(this);
    setupLanguageOptions();
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

    ui->swapShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.swapLanguage,
                                                                    QKeySequence::PortableText));
    ui->pinShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.toggleOnTop,
                                                                  QKeySequence::PortableText));
    ui->settingsShortcutEdit->setKeySequence(QKeySequence::fromString(shortcuts.openSettings,
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

    config.activeProvider = ProviderType::Baidu;
    config.languagePairs = currentPairs();
    config.appLanguage = static_cast<AppLanguage>(ui->appLanguageCombo->currentIndex());

    const ShortcutConfig defaults = defaultShortcutsForCurrentPlatform();
    auto toPortable = [](const QKeySequence &seq) {
        return seq.toString(QKeySequence::PortableText).trimmed();
    };

    config.shortcuts.swapLanguage = toPortable(ui->swapShortcutEdit->keySequence());
    config.shortcuts.toggleOnTop = toPortable(ui->pinShortcutEdit->keySequence());
    config.shortcuts.openSettings = toPortable(ui->settingsShortcutEdit->keySequence());

    if (config.shortcuts.swapLanguage.isEmpty()) {
        config.shortcuts.swapLanguage = defaults.swapLanguage;
    }
    if (config.shortcuts.toggleOnTop.isEmpty()) {
        config.shortcuts.toggleOnTop = defaults.toggleOnTop;
    }
    if (config.shortcuts.openSettings.isEmpty()) {
        config.shortcuts.openSettings = defaults.openSettings;
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

void SettingsWidget::applyLanguage(AppLanguage language)
{
    setWindowTitle(L10n::text(language, "settings.title"));
    ui->appGroup->setTitle(L10n::text(language, "settings.group.app"));
    ui->labelAppLanguage->setText(L10n::text(language, "settings.label.app_language"));

    ui->shortcutsGroup->setTitle(L10n::text(language, "settings.group.shortcuts"));
    ui->labelSwapShortcut->setText(L10n::text(language, "settings.shortcuts.swap"));
    ui->labelPinShortcut->setText(L10n::text(language, "settings.shortcuts.pin"));
    ui->labelSettingsShortcut->setText(L10n::text(language, "settings.shortcuts.settings"));
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
        canonical(shortcuts.openSettings)
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
