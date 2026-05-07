#include "translate.h"
#include "./ui_translate.h"
#include "baidutranslatorservice.h"
#include "configstore.h"
#include "deepltranslatorservice.h"
#include "dictionarytranslatorservice.h"
#include "languagedetector.h"
#include "l10n.h"
#include "openaitranslatorservice.h"
#include "settingswidget.h"
#include "speechplayer.h"
#include "translationhistorystore.h"

#include <QHotkey>
#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QLinearGradient>
#include <QListView>
#include <QMimeData>
#include <QProcess>
#include <QPushButton>
#include <QTimer>

#if defined(Q_OS_MACOS)
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace {
QMimeData *cloneMimeData(const QMimeData *source)
{
    auto *copy = new QMimeData();
    if (!source) {
        return copy;
    }
    for (const QString &format : source->formats()) {
        copy->setData(format, source->data(format));
    }
    if (source->hasText()) {
        copy->setText(source->text());
    }
    if (source->hasHtml()) {
        copy->setHtml(source->html());
    }
    if (source->hasUrls()) {
        copy->setUrls(source->urls());
    }
    if (source->hasImage()) {
        copy->setImageData(source->imageData());
    }
    return copy;
}

#if defined(Q_OS_MACOS)
void setupMacNativeHotkeyMappings()
{
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    // QHotkey may fail mapping these on some macOS input sources, so provide native mapping explicitly.
    QHotkey::addGlobalMapping(QKeySequence::fromString("Ctrl+Meta+T", QKeySequence::PortableText),
                              QHotkey::NativeShortcut(kVK_ANSI_T, cmdKey | controlKey));
    QHotkey::addGlobalMapping(QKeySequence::fromString("Ctrl+Meta+F", QKeySequence::PortableText),
                              QHotkey::NativeShortcut(kVK_ANSI_F, cmdKey | controlKey));
    QHotkey::addGlobalMapping(QKeySequence::fromString("Ctrl+,", QKeySequence::PortableText),
                              QHotkey::NativeShortcut(kVK_ANSI_Comma, cmdKey));
    QHotkey::addGlobalMapping(QKeySequence::fromString("Ctrl+Meta+D", QKeySequence::PortableText),
                              QHotkey::NativeShortcut(kVK_ANSI_D, cmdKey | controlKey));
}

bool sendNativeCopyShortcut()
{
    if (!AXIsProcessTrusted()) {
        return false;
    }

    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    CGEventRef keyDown = CGEventCreateKeyboardEvent(source, kVK_ANSI_C, true);
    CGEventRef keyUp = CGEventCreateKeyboardEvent(source, kVK_ANSI_C, false);
    CGEventSetFlags(keyDown, kCGEventFlagMaskCommand);
    CGEventSetFlags(keyUp, kCGEventFlagMaskCommand);
    CGEventPost(kCGHIDEventTap, keyDown);
    CGEventPost(kCGHIDEventTap, keyUp);
    CFRelease(keyDown);
    CFRelease(keyUp);
    CFRelease(source);
    return true;
}
#elif defined(Q_OS_WIN)
bool sendNativeCopyShortcut()
{
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'C';
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'C';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(4, inputs, sizeof(INPUT)) == 4;
}
#else
bool sendNativeCopyShortcut()
{
    return false;
}
#endif
}

Translate::Translate(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Translate)
    , m_dragPosition(0, 0)
    , m_settingsWidget(nullptr)
    , m_baiduService(new BaiduTranslatorService(this))
    , m_openAIService(new OpenAITranslatorService(this))
    , m_deepLService(new DeepLTranslatorService(this))
    , m_dictionaryService(new DictionaryTranslatorService(this))
    , m_speechPlayer(new SpeechPlayer(this))
    , m_swapHotkey(nullptr)
    , m_pinHotkey(nullptr)
    , m_settingsHotkey(nullptr)
    , m_selectionHotkey(nullptr)
    , m_isTranslating(false)
{
    ui->setupUi(this);
    
    // 设置窗口为无边框
    setWindowFlags(Qt::FramelessWindowHint);
    
    // 设置窗口背景透明
    setAttribute(Qt::WA_TranslucentBackground);
    
    resize(460, 138);
    applyDialogStyle();
    ui->Translation->setReadOnly(true);
    ui->Convert->setAutoDefault(false);
    ui->Convert->setDefault(false);
    ui->Fixed->setAutoDefault(false);
    ui->Fixed->setDefault(false);
    ui->PlayAudio->setAutoDefault(false);
    ui->PlayAudio->setDefault(false);
    ui->Settings->setAutoDefault(false);
    ui->Settings->setDefault(false);

    auto *languageView = new QListView(ui->SelectLanguage);
    languageView->setFrameShape(QFrame::NoFrame);
    languageView->setStyleSheet(
        "QListView {"
        "  background: rgba(40, 44, 50, 0.98);"
        "  color: #FFFFFF;"
        "  border: none;"
        "  outline: 0;"
        "}"
        "QListView::item {"
        "  color: #FFFFFF;"
        "  padding: 4px 8px;"
        "}"
        "QListView::item:selected {"
        "  color: #FFFFFF;"
        "  background: rgba(98, 107, 121, 0.95);"
        "}"
    );
    ui->SelectLanguage->setView(languageView);

    connect(ui->Fixed,&QPushButton::clicked,this,&Translate::toggleStayOnTop);
    connect(ui->PlayAudio, &QPushButton::clicked, this, &Translate::toggleSpeech);
    connect(ui->Settings, &QPushButton::clicked, this, &Translate::openSettings);
    connect(ui->Convert, &QPushButton::clicked, this, &Translate::swapLanguagePair);
    connect(ui->OriginalText, &QLineEdit::returnPressed, this, &Translate::triggerTranslate);
    connect(m_baiduService, &BaiduTranslatorService::translationFinished,
            this, &Translate::onTranslationFinished);
    connect(m_openAIService, &OpenAITranslatorService::translationFinished,
            this, &Translate::onTranslationFinished);
    connect(m_deepLService, &DeepLTranslatorService::translationFinished,
            this, &Translate::onTranslationFinished);
    connect(m_dictionaryService, &DictionaryTranslatorService::translationFinished,
            this, &Translate::onTranslationFinished);
    connect(m_speechPlayer, &SpeechPlayer::playingChanged,
            this, &Translate::onSpeechPlayingChanged);

    m_config = ConfigStore::load();
    applyLanguage(m_config.appLanguage);
    reloadLanguagePairs();
    m_baiduService->setConfig(m_config);
    m_openAIService->setConfig(m_config);
    m_deepLService->setConfig(m_config);
    m_dictionaryService->setConfig(m_config);
#if defined(Q_OS_MACOS)
    setupMacNativeHotkeyMappings();
#endif
    // Register global hotkeys after the event loop starts.
    // Some platforms (notably macOS) may fail registrations before app.exec().
    QTimer::singleShot(0, this, [this]() {
        applyShortcuts(m_config.shortcuts);
    });
    // Startup fallback: retry once if registration is still not ready.
    QTimer::singleShot(1000, this, [this]() {
        if (!hasRegisteredHotkeys()) {
            applyShortcuts(m_config.shortcuts);
        }
    });
    // When app becomes active, retry registration if none is currently active.
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive && !hasRegisteredHotkeys()) {
            applyShortcuts(m_config.shortcuts);
        }
    });
}

Translate::~Translate()
{
    unregisterGlobalHotkeys();
    delete ui;
}

void Translate::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    QPainterPath path;
    QRectF rect = this->rect().adjusted(1, 1, -1, -1);
    path.addRoundedRect(rect, 22, 22);

    // Use the previous 3bdd0f7 backdrop style.
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    gradient.setColorAt(0.0, QColor(17, 33, 56, 168));
    gradient.setColorAt(1.0, QColor(35, 53, 83, 168));
    painter.fillPath(path, gradient);
    painter.setPen(QPen(QColor(120, 150, 190, 120), 1));
    painter.drawPath(path);
}

void Translate::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void Translate::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void Translate::toggleStayOnTop() {
    bool isOnTop = windowFlags() & Qt::WindowStaysOnTopHint;

    if (isOnTop) {
        // 如果已经在顶层，则移除置顶标志
        setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowStaysOnTopHint);
    } else {
        // 如果不在顶层，则添加置顶标志
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }

    // 显示窗口并激活
    show();
    activateWindow();
    raise();
}

void Translate::openSettings()
{
    if (!m_settingsWidget) {
        m_settingsWidget = new SettingsWidget();
        connect(m_settingsWidget, &SettingsWidget::configSaved, this, &Translate::onConfigSaved);
    }

    m_settingsWidget->setConfig(m_config);
    m_settingsWidget->setHotkeyStatusMessage(m_hotkeyStatusMessage);
    m_settingsWidget->show();
    m_settingsWidget->raise();
    m_settingsWidget->activateWindow();
}

void Translate::onConfigSaved(const AppConfig &config)
{
    m_config = config;
    ConfigStore::save(m_config);
    applyLanguage(m_config.appLanguage);
    reloadLanguagePairs();
    m_baiduService->setConfig(m_config);
    m_openAIService->setConfig(m_config);
    m_deepLService->setConfig(m_config);
    m_dictionaryService->setConfig(m_config);
    applyShortcuts(m_config.shortcuts);
}

void Translate::reloadLanguagePairs()
{
    const QString current = ui->SelectLanguage->currentText();
    ui->SelectLanguage->clear();

    QStringList pairs = m_config.languagePairs;
    if (pairs.isEmpty()) {
        pairs << "zh <> en";
    }
    ui->SelectLanguage->addItem(L10n::text(m_config.appLanguage, "dialog.language.auto"));
    ui->SelectLanguage->addItems(pairs);
    for (int i = 0; i < ui->SelectLanguage->count(); ++i) {
        ui->SelectLanguage->setItemData(i, QColor("#FFFFFF"), Qt::ForegroundRole);
    }

    int index = ui->SelectLanguage->findText(current);
    if (index < 0) {
        index = 0;
    }
    ui->SelectLanguage->setCurrentIndex(index);
}

void Translate::triggerTranslate()
{
    if (m_isTranslating) {
        return;
    }

    const QString sourceText = ui->OriginalText->text().trimmed();
    if (sourceText.isEmpty()) {
        return;
    }

    m_speechPlayer->stop();
    QString from;
    QString to;
    if (!resolveLanguageDirection(sourceText, from, to)) {
        ui->Translation->setText(L10n::text(m_config.appLanguage, "dialog.error.invalid_pair"));
        return;
    }

    const QString provider = activeProviderKey();
    const auto cached = m_config.cache.enabled ? m_translationCache.find(provider, from, to, sourceText) : std::nullopt;
    if (cached.has_value()) {
        m_pendingSourceText.clear();
        m_pendingFrom.clear();
        m_pendingTo.clear();
        m_pendingProvider.clear();
        ui->Translation->setText(cached.value());
        ui->Translation->setFocus();
        ui->Translation->selectAll();
        return;
    }

    m_pendingSourceText = sourceText;
    m_pendingFrom = from;
    m_pendingTo = to;
    m_pendingProvider = provider;

    m_isTranslating = true;
    ui->OriginalText->setEnabled(false);
    ui->Translation->setText(L10n::text(m_config.appLanguage, "dialog.status.translating"));
    activeTranslatorService()->translate(sourceText, from, to);
}

void Translate::onTranslationFinished(const TranslationResult &result)
{
    m_isTranslating = false;
    ui->OriginalText->setEnabled(true);

    if (result.success) {
        if (m_config.cache.enabled && !m_pendingProvider.isEmpty() && !m_pendingSourceText.isEmpty()) {
            m_translationCache.upsert(m_pendingProvider,
                                      result.sourceLanguage.isEmpty() ? m_pendingFrom : result.sourceLanguage,
                                      result.targetLanguage.isEmpty() ? m_pendingTo : result.targetLanguage,
                                      m_pendingSourceText,
                                      result.translatedText);
        }
        const QString displayText = result.phoneticText.isEmpty()
                                        ? result.translatedText
                                        : QString("%1\n%2").arg(result.phoneticText, result.translatedText);
        ui->Translation->setText(displayText);
        m_lastTranslatedText = result.translatedText;
        m_lastTargetLanguage = result.targetLanguage.isEmpty() ? m_pendingTo : result.targetLanguage;
        if (m_config.history.enabled) {
            TranslationHistoryEntry entry;
            entry.sourceText = m_pendingSourceText;
            entry.translatedText = result.translatedText;
            entry.phoneticText = result.phoneticText;
            entry.provider = result.provider;
            entry.sourceLanguage = result.sourceLanguage.isEmpty() ? m_pendingFrom : result.sourceLanguage;
            entry.targetLanguage = m_lastTargetLanguage;
            TranslationHistoryStore().add(entry, m_config.history.maxEntries);
        }
        ui->Translation->setFocus();
        ui->Translation->selectAll();
        m_pendingSourceText.clear();
        m_pendingFrom.clear();
        m_pendingTo.clear();
        m_pendingProvider.clear();
        return;
    }

    if (m_config.cache.enabled && !m_pendingProvider.isEmpty() && !m_pendingSourceText.isEmpty()) {
        const auto fallback = m_translationCache.find(m_pendingProvider,
                                                      m_pendingFrom,
                                                      m_pendingTo,
                                                      m_pendingSourceText);
        if (fallback.has_value()) {
            ui->Translation->setText(fallback.value());
            ui->Translation->setFocus();
            ui->Translation->selectAll();
            m_pendingSourceText.clear();
            m_pendingFrom.clear();
            m_pendingTo.clear();
            m_pendingProvider.clear();
            return;
        }
    }

    ui->Translation->setText(result.errorMessage);
    m_lastTranslatedText.clear();
    m_lastTargetLanguage.clear();
    m_pendingSourceText.clear();
    m_pendingFrom.clear();
    m_pendingTo.clear();
    m_pendingProvider.clear();
}

QString Translate::activeProviderKey() const
{
    switch (m_config.activeProvider) {
    case ProviderType::OpenAICompatible:
        return "openai";
    case ProviderType::DeepL:
        return "deepl";
    case ProviderType::Dictionary:
        return "dictionary";
    case ProviderType::Baidu:
    default:
        return "baidu";
    }
}

TranslatorService *Translate::activeTranslatorService() const
{
    switch (m_config.activeProvider) {
    case ProviderType::OpenAICompatible:
        return m_openAIService;
    case ProviderType::DeepL:
        return m_deepLService;
    case ProviderType::Dictionary:
        return m_dictionaryService;
    case ProviderType::Baidu:
    default:
        return m_baiduService;
    }
}

bool Translate::parseLanguagePair(const QString &pair, QString &from, QString &to) const
{
    QString normalized = pair.trimmed();
    normalized.replace("-->", "->");
    normalized.replace("<->", "<>");
    normalized.replace("<=>", "<>");
    normalized.remove(' ');

    QString separator = normalized.contains("<>") ? "<>" : "->";
    const QStringList parts = normalized.split(separator, Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        return false;
    }

    from = parts.at(0);
    to = parts.at(1);
    return !from.isEmpty() && !to.isEmpty();
}

void Translate::swapLanguagePair()
{
    QString from;
    QString to;
    if (!parseLanguagePair(selectedLanguagePair(), from, to)) {
        return;
    }
    const QString reversed = to + " <> " + from;
    const QString canonical = from < to ? from + " <> " + to : to + " <> " + from;
    int index = ui->SelectLanguage->findText(canonical);
    if (index >= 0) {
        ui->SelectLanguage->setCurrentIndex(index);
    }
}

void Translate::toggleSpeech()
{
    if (m_speechPlayer->isPlaying()) {
        m_speechPlayer->stop();
        return;
    }

    const QString text = m_lastTranslatedText.trimmed().isEmpty() ? ui->Translation->text() : m_lastTranslatedText;
    if (text.trimmed().isEmpty()) {
        return;
    }
    m_speechPlayer->play(text, m_lastTargetLanguage);
}

void Translate::onSpeechPlayingChanged(bool playing)
{
    ui->PlayAudio->setText(playing ? "■" : "▶");
    ui->PlayAudio->setToolTip(L10n::text(m_config.appLanguage,
                                         playing ? "dialog.tooltip.stop_audio" : "dialog.tooltip.play_audio"));
}

void Translate::translateSelection()
{
    requestSelectionText();
}

void Translate::applyLanguage(AppLanguage language)
{
    setWindowTitle(L10n::text(language, "dialog.title"));
    ui->OriginalText->setPlaceholderText(L10n::text(language, "dialog.original.placeholder"));
    ui->Translation->setPlaceholderText(L10n::text(language, "dialog.result.placeholder"));
    ui->Convert->setToolTip(L10n::text(language, "dialog.tooltip.swap"));
    ui->PlayAudio->setToolTip(L10n::text(language, m_speechPlayer->isPlaying()
                                                       ? "dialog.tooltip.stop_audio"
                                                       : "dialog.tooltip.play_audio"));
    ui->Fixed->setToolTip(L10n::text(language, "dialog.tooltip.pin"));
    ui->Settings->setToolTip(L10n::text(language, "dialog.tooltip.settings"));
}

void Translate::applyDialogStyle()
{
    setStyleSheet(
        "QDialog {"
        "  background: transparent;"
        "}"
        "QComboBox, QLineEdit {"
        "  background: rgba(210, 214, 220, 0.96);"
        "  color: #30343C;"
        "  border: 1px solid rgba(156, 163, 173, 0.95);"
        "  border-radius: 10px;"
        "  padding: 5px 10px;"
        "  selection-background-color: rgba(182, 188, 198, 0.95);"
        "  selection-color: #1F2329;"
        "}"
        "QComboBox {"
        "  combobox-popup: 1;"
        "  padding-right: 26px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: rgba(40, 44, 50, 0.98);"
        "  border: none;"
        "  color: #FFFFFF;"
        "  outline: 0px;"
        "  selection-background-color: rgba(98, 107, 121, 0.95);"
        "  selection-color: #FFFFFF;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "  color: #FFFFFF;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "  color: #FFFFFF;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 22px;"
        "  border: none;"
        "  background: transparent;"
        "}"
        "QComboBox::down-arrow {"
        "  image: none;"
        "  width: 0px;"
        "  height: 0px;"
        "  border-left: 5px solid transparent;"
        "  border-right: 5px solid transparent;"
        "  border-top: 7px solid #505762;"
        "  margin-right: 6px;"
        "}"
        "QLineEdit:read-only {"
        "  background: rgba(201, 206, 214, 0.96);"
        "}"
        "QPushButton {"
        "  background: rgba(202, 207, 214, 0.96);"
        "  color: #3A4049;"
        "  border: 1px solid rgba(150, 158, 169, 0.95);"
        "  border-radius: 10px;"
        "  padding: 4px 8px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(214, 220, 228, 0.98);"
        "}"
        "QPushButton:pressed {"
        "  background: rgba(184, 190, 199, 0.98);"
        "}"
    );
}

void Translate::applyShortcuts(const ShortcutConfig &shortcuts)
{
    m_config.shortcuts = shortcuts;
    unregisterGlobalHotkeys();
    registerGlobalHotkeys(shortcuts);
}

bool Translate::hasRegisteredHotkeys() const
{
    return m_swapHotkey || m_pinHotkey || m_settingsHotkey || m_selectionHotkey;
}

void Translate::unregisterGlobalHotkeys()
{
    auto cleanup = [](QHotkey *&hotkey) {
        if (!hotkey) {
            return;
        }
        hotkey->setRegistered(false);
        delete hotkey;
        hotkey = nullptr;
    };
    cleanup(m_swapHotkey);
    cleanup(m_pinHotkey);
    cleanup(m_settingsHotkey);
    cleanup(m_selectionHotkey);
}

void Translate::registerGlobalHotkeys(const ShortcutConfig &shortcuts)
{
    const ShortcutConfig defaults = defaultShortcutsForCurrentPlatform();
    ShortcutConfig finalShortcuts = shortcuts;
    bool shouldSaveConfig = false;
    QStringList warnings;

    auto buildSequence = [](const QString &shortcut) {
        return QKeySequence::fromString(shortcut, QKeySequence::PortableText);
    };

    auto registerAction = [this, &buildSequence, &warnings, &shouldSaveConfig](
                              QHotkey *&target,
                              QString &configuredShortcut,
                              const QString &defaultShortcut,
                              void (Translate::*slot)(),
                              const QString &fallbackMessage,
                              const QString &failedMessage) {
        const QKeySequence configuredSeq = buildSequence(configuredShortcut);
        const QKeySequence defaultSeq = buildSequence(defaultShortcut);
        const QString configuredCanonical = configuredSeq.toString(QKeySequence::PortableText);
        const QString defaultCanonical = defaultSeq.toString(QKeySequence::PortableText);

        auto tryRegister = [this, slot](const QKeySequence &sequence) -> QHotkey * {
            if (sequence.isEmpty()) {
                return nullptr;
            }
            QHotkey *hotkey = new QHotkey(sequence, true, this);
            if (!hotkey->isRegistered()) {
                delete hotkey;
                return nullptr;
            }
            connect(hotkey, &QHotkey::activated, this, slot);
            return hotkey;
        };

        target = tryRegister(configuredSeq);
        if (target) {
            return;
        }

        const bool configuredWasEmpty = configuredCanonical.isEmpty();
        const bool canTryDefault = !defaultCanonical.isEmpty() && configuredCanonical != defaultCanonical;

        if (canTryDefault) {
            target = tryRegister(defaultSeq);
            if (target) {
                if (configuredShortcut != defaultCanonical) {
                    configuredShortcut = defaultCanonical;
                    shouldSaveConfig = true;
                }
                if (!configuredWasEmpty) {
                    warnings << fallbackMessage;
                }
                return;
            }
        }

        warnings << failedMessage;
    };

    registerAction(m_swapHotkey,
                   finalShortcuts.swapLanguage,
                   defaults.swapLanguage,
                   &Translate::swapLanguagePair,
                   L10n::text(m_config.appLanguage, "dialog.hotkey.fallback.swap"),
                   L10n::text(m_config.appLanguage, "dialog.hotkey.failed.swap"));
    registerAction(m_pinHotkey,
                   finalShortcuts.toggleOnTop,
                   defaults.toggleOnTop,
                   &Translate::toggleStayOnTop,
                   L10n::text(m_config.appLanguage, "dialog.hotkey.fallback.pin"),
                   L10n::text(m_config.appLanguage, "dialog.hotkey.failed.pin"));
    registerAction(m_settingsHotkey,
                   finalShortcuts.openSettings,
                   defaults.openSettings,
                   &Translate::openSettings,
                   L10n::text(m_config.appLanguage, "dialog.hotkey.fallback.settings"),
                   L10n::text(m_config.appLanguage, "dialog.hotkey.failed.settings"));
    registerAction(m_selectionHotkey,
                   finalShortcuts.translateSelection,
                   defaults.translateSelection,
                   &Translate::translateSelection,
                   L10n::text(m_config.appLanguage, "dialog.hotkey.fallback.selection"),
                   L10n::text(m_config.appLanguage, "dialog.hotkey.failed.selection"));

    m_hotkeyStatusMessage = warnings.join("\n");

    if (shouldSaveConfig) {
        m_config.shortcuts = finalShortcuts;
        ConfigStore::save(m_config);
    }

    if (m_settingsWidget) {
        if (shouldSaveConfig) {
            m_settingsWidget->setConfig(m_config);
        }
        m_settingsWidget->setHotkeyStatusMessage(m_hotkeyStatusMessage);
    }
}

bool Translate::resolveLanguageDirection(const QString &text, QString &from, QString &to) const
{
    QString left;
    QString right;
    if (!parseLanguagePair(selectedLanguagePair(), left, right)) {
        return false;
    }

    const QString detected = LanguageDetector::detect(text);
    if (detected == left) {
        from = left;
        to = right;
        return true;
    }
    if (detected == right) {
        from = right;
        to = left;
        return true;
    }

    from = detected == "auto" ? QStringLiteral("auto") : detected;
    to = right;
    return true;
}

QString Translate::selectedLanguagePair() const
{
    const int index = ui->SelectLanguage->currentIndex();
    if (index <= 0) {
        return m_config.defaultLanguagePair.isEmpty() ? QStringLiteral("zh <> en") : m_config.defaultLanguagePair;
    }
    return ui->SelectLanguage->currentText();
}

void Translate::requestSelectionText()
{
    QClipboard *clipboard = QApplication::clipboard();
    QMimeData *previousMime = cloneMimeData(clipboard->mimeData());

    if (!sendNativeCopyShortcut()) {
        delete previousMime;
        show();
        activateWindow();
        raise();
        ui->Translation->setText(L10n::text(m_config.appLanguage, "dialog.error.selection_permission"));
        return;
    }

    QTimer::singleShot(180, this, [this, clipboard, previousMime]() {
        const QString selectedText = clipboard->text().trimmed();
        clipboard->setMimeData(previousMime);
        show();
        activateWindow();
        raise();
        if (selectedText.isEmpty()) {
            ui->Translation->setText(L10n::text(m_config.appLanguage, "dialog.error.no_selection"));
            return;
        }
        ui->OriginalText->setText(selectedText);
        ui->OriginalText->setFocus();
        triggerTranslate();
    });
}
