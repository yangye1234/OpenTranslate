#include "translate.h"
#include "ui_translate.h"
#include "baidutranslatorservice.h"
#include "configstore.h"
#include "deepltranslatorservice.h"
#include "dictionarytranslatorservice.h"
#include "languagedetector.h"
#include "l10n.h"
#include "ocrservice.h"
#include "openaitranslatorservice.h"
#include "screenshotoverlay.h"
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
#include <QTextEdit>
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

    // QHotkey may fail mapping Ctrl+Meta letter shortcuts on some macOS input sources.
    const struct {
        const char *key;
        UInt32 nativeKey;
    } letterMappings[] = {
        {"A", kVK_ANSI_A}, {"B", kVK_ANSI_B}, {"C", kVK_ANSI_C}, {"D", kVK_ANSI_D},
        {"E", kVK_ANSI_E}, {"F", kVK_ANSI_F}, {"G", kVK_ANSI_G}, {"H", kVK_ANSI_H},
        {"I", kVK_ANSI_I}, {"J", kVK_ANSI_J}, {"K", kVK_ANSI_K}, {"L", kVK_ANSI_L},
        {"M", kVK_ANSI_M}, {"N", kVK_ANSI_N}, {"O", kVK_ANSI_O}, {"P", kVK_ANSI_P},
        {"Q", kVK_ANSI_Q}, {"R", kVK_ANSI_R}, {"S", kVK_ANSI_S}, {"T", kVK_ANSI_T},
        {"U", kVK_ANSI_U}, {"V", kVK_ANSI_V}, {"W", kVK_ANSI_W}, {"X", kVK_ANSI_X},
        {"Y", kVK_ANSI_Y}, {"Z", kVK_ANSI_Z},
    };
    for (const auto &mapping : letterMappings) {
        QHotkey::addGlobalMapping(QKeySequence::fromString("Ctrl+Meta+" + QString::fromLatin1(mapping.key),
                                                           QKeySequence::PortableText),
                                  QHotkey::NativeShortcut(mapping.nativeKey, cmdKey | controlKey));
    }
    QHotkey::addGlobalMapping(QKeySequence::fromString("Ctrl+,", QKeySequence::PortableText),
                              QHotkey::NativeShortcut(kVK_ANSI_Comma, cmdKey));
    QHotkey::addGlobalMapping(QKeySequence::fromString("Ctrl+Meta+Space", QKeySequence::PortableText),
                              QHotkey::NativeShortcut(kVK_Space, cmdKey | controlKey));
}

bool sendNativeCopyShortcut()
{
    const void *keys[] = { kAXTrustedCheckOptionPrompt };
    const void *values[] = { kCFBooleanTrue };
    CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault,
                                                 keys,
                                                 values,
                                                 1,
                                                 &kCFCopyStringDictionaryKeyCallBacks,
                                                 &kCFTypeDictionaryValueCallBacks);
    const bool trusted = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);
    if (!trusted) {
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
    , m_dictionaryDetailsService(new DictionaryTranslatorService(this))
    , m_ocrService(OcrService::create(this))
    , m_speechPlayer(new SpeechPlayer(this))
    , m_swapHotkey(nullptr)
    , m_pinHotkey(nullptr)
    , m_settingsHotkey(nullptr)
    , m_selectionHotkey(nullptr)
    , m_speechHotkey(nullptr)
    , m_screenshotHotkey(nullptr)
    , m_isTranslating(false)
    , m_dictionaryPanelExpanded(false)
{
    ui->setupUi(this);
    
    // 设置窗口为无边框
    setWindowFlags(Qt::FramelessWindowHint);
    
    // 设置窗口背景透明
    setAttribute(Qt::WA_TranslucentBackground);
    
    resize(460, 168);
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
    ui->DictionaryToggle->setAutoDefault(false);
    ui->DictionaryToggle->setDefault(false);
    ui->DictionaryAudio->setAutoDefault(false);
    ui->DictionaryAudio->setDefault(false);
    ui->DictionaryAudio->setVisible(false);
    ui->DictionaryDetails->setVisible(false);
    ui->DictionaryDetails->setReadOnly(true);
    setDictionaryPanelExpanded(false);

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
    connect(ui->DictionaryToggle, &QPushButton::toggled, this, &Translate::setDictionaryPanelExpanded);
    connect(ui->DictionaryAudio, &QPushButton::clicked, this, [this]() {
        if (!m_dictionaryAudioUrl.isEmpty()) {
            m_speechPlayer->play(QString(), QString(), m_dictionaryAudioUrl);
        }
    });
    connect(ui->OriginalText, &QLineEdit::returnPressed, this, &Translate::triggerTranslate);
    connect(m_baiduService, &BaiduTranslatorService::translationFinished,
            this, &Translate::onTranslationFinished);
    connect(m_openAIService, &OpenAITranslatorService::translationFinished,
            this, &Translate::onTranslationFinished);
    connect(m_deepLService, &DeepLTranslatorService::translationFinished,
            this, &Translate::onTranslationFinished);
    connect(m_dictionaryService, &DictionaryTranslatorService::translationFinished,
            this, &Translate::onTranslationFinished);
    connect(m_dictionaryDetailsService, &DictionaryTranslatorService::translationFinished,
            this, &Translate::onDictionaryDetailsFinished);
    connect(m_speechPlayer, &SpeechPlayer::playingChanged,
            this, &Translate::onSpeechPlayingChanged);
    connect(m_speechPlayer, &SpeechPlayer::errorOccurred, this, [this](const QString &message) {
        ui->Translation->setText(message);
    });
    connect(m_ocrService, &OcrService::recognitionFinished,
            this, &Translate::onOcrFinished);

    m_config = ConfigStore::load();
    applyLanguage(m_config.appLanguage);
    reloadLanguagePairs();
    m_baiduService->setConfig(m_config);
    m_openAIService->setConfig(m_config);
    m_deepLService->setConfig(m_config);
    m_dictionaryService->setConfig(m_config);
    configureDictionaryDetailsService();
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
    const bool willPin = !isOnTop;

    if (isOnTop) {
        // 如果已经在顶层，则移除置顶标志
        setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowStaysOnTopHint);
    } else {
        // 如果不在顶层，则添加置顶标志
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }

    // 显示窗口并激活
    show();
    if (!willPin && m_config.windowBehavior.lowerOnUnpin) {
        lower();
    } else {
        activateWindow();
        raise();
        ui->OriginalText->setFocus(Qt::ShortcutFocusReason);
        ui->OriginalText->selectAll();
    }
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
    configureDictionaryDetailsService();
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
    setDictionaryDetailsLoading();
    const auto cached = m_config.cache.enabled ? m_translationCache.find(provider, from, to, sourceText) : std::nullopt;
    if (cached.has_value() || provider != "dictionary") {
        startDictionaryDetailsLookup(sourceText, from, to);
    }
    if (cached.has_value()) {
        m_pendingSourceText.clear();
        m_pendingFrom.clear();
        m_pendingTo.clear();
        m_pendingProvider.clear();
        ui->Translation->setText(cached.value());
        m_lastTranslatedText = cached.value();
        m_lastTargetLanguage = to;
        m_lastAudioUrl.clear();
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
        const bool inlinePhonetic = result.provider != "dictionary" && !result.phoneticText.isEmpty();
        const QString displayText = !inlinePhonetic
                                        ? result.translatedText
                                        : QString("%1\n%2").arg(result.phoneticText, result.translatedText);
        ui->Translation->setText(displayText);
        m_lastTranslatedText = result.translatedText;
        m_lastTargetLanguage = result.targetLanguage.isEmpty() ? m_pendingTo : result.targetLanguage;
        m_lastAudioUrl = result.audioUrl;
        if (result.provider == "dictionary") {
            updateDictionaryDetails(result);
        }
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

    if (result.provider == "dictionary") {
        updateDictionaryDetails(result);
    }
    ui->Translation->setText(result.errorMessage);
    m_lastTranslatedText.clear();
    m_lastTargetLanguage.clear();
    m_lastAudioUrl.clear();
    m_pendingSourceText.clear();
    m_pendingFrom.clear();
    m_pendingTo.clear();
    m_pendingProvider.clear();
}

void Translate::onDictionaryDetailsFinished(const TranslationResult &result)
{
    if (!m_pendingDictionaryText.isEmpty()
        && !result.queryText.isEmpty()
        && result.queryText != m_pendingDictionaryText) {
        return;
    }
    updateDictionaryDetails(result);
}

void Translate::startDictionaryDetailsLookup(const QString &text, const QString &from, const QString &to)
{
    m_pendingDictionaryText = text.trimmed();
    m_dictionaryAudioUrl.clear();
    ui->DictionaryAudio->setVisible(false);
    if (m_pendingDictionaryText.isEmpty()) {
        setDictionaryDetailsMessage(L10n::text(m_config.appLanguage, "dialog.dictionary.empty"));
        return;
    }
    configureDictionaryDetailsService();
    m_dictionaryDetailsService->translate(m_pendingDictionaryText, from, to);
}

void Translate::setDictionaryPanelExpanded(bool expanded)
{
    m_dictionaryPanelExpanded = expanded;
    ui->DictionaryDetails->setVisible(expanded);
    ui->DictionaryToggle->setChecked(expanded);
    ui->DictionaryToggle->setText(QString("%1 %2")
                                      .arg(L10n::text(m_config.appLanguage, "dialog.dictionary.title"),
                                           expanded ? QStringLiteral("▾") : QStringLiteral("▸")));

    const int targetHeight = expanded ? 340 : 168;
    setMinimumSize(460, targetHeight);
    setMaximumSize(460, targetHeight);
    resize(460, targetHeight);
}

void Translate::setDictionaryDetailsLoading()
{
    m_dictionaryAudioUrl.clear();
    ui->DictionaryAudio->setVisible(false);
    ui->DictionaryDetails->setPlainText(L10n::text(m_config.appLanguage, "dialog.dictionary.loading"));
}

void Translate::setDictionaryDetailsMessage(const QString &message)
{
    m_dictionaryAudioUrl.clear();
    ui->DictionaryAudio->setVisible(false);
    ui->DictionaryDetails->setPlainText(message);
}

void Translate::updateDictionaryDetails(const TranslationResult &result)
{
    const QString details = formatDictionaryDetails(result);
    if (details.trimmed().isEmpty()) {
        setDictionaryDetailsMessage(result.errorMessage.trimmed().isEmpty()
                                        ? L10n::text(m_config.appLanguage, "dialog.dictionary.empty")
                                        : result.errorMessage);
        return;
    }

    m_dictionaryAudioUrl = result.audioUrl;
    for (const DictionaryPhonetic &phonetic : result.dictionaryPhonetics) {
        if (!phonetic.audioUrl.trimmed().isEmpty()) {
            m_dictionaryAudioUrl = phonetic.audioUrl;
            break;
        }
    }
    ui->DictionaryAudio->setVisible(!m_dictionaryAudioUrl.isEmpty());
    ui->DictionaryDetails->setPlainText(details);
}

QString Translate::formatDictionaryDetails(const TranslationResult &result) const
{
    QStringList blocks;
    if (!result.dictionaryPhonetics.isEmpty() || !result.phoneticText.trimmed().isEmpty()) {
        QStringList lines;
    for (const DictionaryPhonetic &phonetic : result.dictionaryPhonetics) {
        if (!phonetic.value.trimmed().isEmpty()) {
            const QString label = phonetic.label == "phonetic"
                                      ? dictionarySectionTitle("phonetics")
                                      : phonetic.label;
            lines << QString("%1 [%2]").arg(label, phonetic.value);
        }
    }
        if (lines.isEmpty() && !result.phoneticText.trimmed().isEmpty()) {
            lines << result.phoneticText.trimmed();
        }
        blocks << dictionarySectionTitle("phonetics") + "\n" + lines.join("\n");
    }

    for (const DictionarySection &section : result.dictionarySections) {
        if (!section.lines.isEmpty()) {
            blocks << dictionarySectionTitle(section.title) + "\n" + section.lines.join("\n");
        }
    }

    return blocks.join("\n\n");
}

QString Translate::dictionarySectionTitle(const QString &title) const
{
    const QString key = "dialog.dictionary.section." + title;
    const QString text = L10n::text(m_config.appLanguage, key);
    return text == key ? title : text;
}

void Translate::configureDictionaryDetailsService()
{
    AppConfig detailsConfig = m_config;
    detailsConfig.dictionary.enabled = true;
    m_dictionaryDetailsService->setConfig(detailsConfig);
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
    m_speechPlayer->play(text, m_lastTargetLanguage, m_lastAudioUrl);
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

void Translate::screenshotTranslate()
{
    auto *overlay = new ScreenshotOverlay();
    connect(overlay, &ScreenshotOverlay::captureFinished, this, [this](const QImage &image) {
        show();
        activateWindow();
        raise();
        ui->OriginalText->setFocus(Qt::ShortcutFocusReason);
        ui->Translation->setText(L10n::text(m_config.appLanguage, "dialog.status.ocr"));
        m_ocrService->recognizeText(image, ocrLanguageHints());
    });
    connect(overlay, &ScreenshotOverlay::captureFailed, this, [this](const QString &message) {
        show();
        activateWindow();
        raise();
        ui->Translation->setText(message.startsWith("dialog.")
                                     ? L10n::text(m_config.appLanguage, message)
                                     : message);
    });
    overlay->begin();
}

void Translate::onOcrFinished(bool success, const QString &text, const QString &errorMessage)
{
    show();
    activateWindow();
    raise();
    ui->OriginalText->setFocus(Qt::ShortcutFocusReason);

    const QString ocrText = text.trimmed();
    if (!success) {
        ui->Translation->setText(errorMessage.trimmed().isEmpty()
                                     ? L10n::text(m_config.appLanguage, "dialog.error.ocr_failed")
                                     : errorMessage);
        return;
    }
    if (ocrText.isEmpty()) {
        ui->Translation->setText(L10n::text(m_config.appLanguage, "dialog.error.ocr_empty"));
        return;
    }

    ui->OriginalText->setText(ocrText);
    ui->OriginalText->selectAll();
    triggerTranslate();
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
    ui->DictionaryAudio->setToolTip(L10n::text(language, "dialog.dictionary.play_audio"));
    setDictionaryPanelExpanded(m_dictionaryPanelExpanded);
}

void Translate::applyDialogStyle()
{
    setStyleSheet(
        "QDialog {"
        "  background: transparent;"
        "}"
        "QComboBox, QLineEdit, QTextEdit {"
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
        "QLineEdit:read-only, QTextEdit:read-only {"
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
    return m_swapHotkey || m_pinHotkey || m_settingsHotkey || m_selectionHotkey || m_speechHotkey || m_screenshotHotkey;
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
    cleanup(m_speechHotkey);
    cleanup(m_screenshotHotkey);
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
        if (configuredCanonical.isEmpty() && defaultCanonical.isEmpty()) {
            return;
        }

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
    registerAction(m_speechHotkey,
                   finalShortcuts.toggleSpeech,
                   defaults.toggleSpeech,
                   &Translate::toggleSpeech,
                   L10n::text(m_config.appLanguage, "dialog.hotkey.fallback.speech"),
                   L10n::text(m_config.appLanguage, "dialog.hotkey.failed.speech"));
    registerAction(m_screenshotHotkey,
                   finalShortcuts.screenshotTranslate,
                   defaults.screenshotTranslate,
                   &Translate::screenshotTranslate,
                   L10n::text(m_config.appLanguage, "dialog.hotkey.fallback.screenshot"),
                   L10n::text(m_config.appLanguage, "dialog.hotkey.failed.screenshot"));

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

QStringList Translate::ocrLanguageHints() const
{
    QString left;
    QString right;
    QStringList hints;
    if (parseLanguagePair(selectedLanguagePair(), left, right)) {
        hints << left << right;
    }
    hints.removeDuplicates();
    return hints;
}
