#include "translate.h"
#include "./ui_translate.h"
#include "baidutranslatorservice.h"
#include "configstore.h"
#include "l10n.h"
#include "settingswidget.h"

#include <QLinearGradient>
#include <QRadialGradient>
#include <QPushButton>

Translate::Translate(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Translate)
    , m_dragPosition(0, 0)
    , m_settingsWidget(nullptr)
    , m_baiduService(new BaiduTranslatorService(this))
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

    connect(ui->Fixed,&QPushButton::clicked,this,&Translate::toggleStayOnTop);
    connect(ui->Settings, &QPushButton::clicked, this, &Translate::openSettings);
    connect(ui->Convert, &QPushButton::clicked, this, &Translate::swapLanguagePair);
    connect(ui->OriginalText, &QLineEdit::returnPressed, this, &Translate::triggerTranslate);
    connect(m_baiduService, &BaiduTranslatorService::translationFinished,
            this, &Translate::onTranslationFinished);

    m_config = ConfigStore::load();
    applyLanguage(m_config.appLanguage);
    reloadLanguagePairs();
    m_baiduService->setConfig(m_config);
}

Translate::~Translate()
{
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

    // Frosted translucent backdrop for the dialog itself.
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    gradient.setColorAt(0.0, QColor(68, 74, 82, 165));
    gradient.setColorAt(1.0, QColor(85, 92, 102, 165));
    painter.fillPath(path, gradient);

    QRadialGradient glow1(QPointF(rect.width() * 0.2, rect.height() * 0.1), rect.width() * 0.7);
    glow1.setColorAt(0.0, QColor(255, 255, 255, 26));
    glow1.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.fillPath(path, glow1);

    QRadialGradient glow2(QPointF(rect.width() * 0.85, rect.height() * 0.85), rect.width() * 0.5);
    glow2.setColorAt(0.0, QColor(255, 255, 255, 18));
    glow2.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.fillPath(path, glow2);

    painter.setPen(QPen(QColor(214, 220, 228, 150), 1));
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
}

void Translate::reloadLanguagePairs()
{
    const QString current = ui->SelectLanguage->currentText();
    ui->SelectLanguage->clear();

    QStringList pairs = m_config.languagePairs;
    if (pairs.isEmpty()) {
        pairs << "en->zh" << "zh->en";
    }
    ui->SelectLanguage->addItems(pairs);

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

    QString from;
    QString to;
    if (!parseLanguagePair(ui->SelectLanguage->currentText(), from, to)) {
        ui->Translation->setText(L10n::text(m_config.appLanguage, "dialog.error.invalid_pair"));
        return;
    }

    m_isTranslating = true;
    ui->OriginalText->setEnabled(false);
    ui->Translation->setText(L10n::text(m_config.appLanguage, "dialog.status.translating"));
    m_baiduService->translate(sourceText, from, to);
}

void Translate::onTranslationFinished(bool success, const QString &translatedText, const QString &errorMessage)
{
    m_isTranslating = false;
    ui->OriginalText->setEnabled(true);

    if (success) {
        ui->Translation->setText(translatedText);
        ui->Translation->setFocus();
        ui->Translation->selectAll();
        return;
    }

    ui->Translation->setText(errorMessage);
}

bool Translate::parseLanguagePair(const QString &pair, QString &from, QString &to) const
{
    QString normalized = pair.trimmed();
    normalized.replace("-->", "->");
    normalized.remove(' ');

    const QStringList parts = normalized.split("->", Qt::SkipEmptyParts);
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
    if (!parseLanguagePair(ui->SelectLanguage->currentText(), from, to)) {
        return;
    }

    const QString reversed = to + "->" + from;
    int index = ui->SelectLanguage->findText(reversed);

    if (index < 0) {
        m_config.languagePairs << reversed;
        ConfigStore::save(m_config);
        reloadLanguagePairs();
        index = ui->SelectLanguage->findText(reversed);
    }

    if (index >= 0) {
        ui->SelectLanguage->setCurrentIndex(index);
    }
}

void Translate::applyLanguage(AppLanguage language)
{
    setWindowTitle(L10n::text(language, "dialog.title"));
    ui->OriginalText->setPlaceholderText(L10n::text(language, "dialog.original.placeholder"));
    ui->Translation->setPlaceholderText(L10n::text(language, "dialog.result.placeholder"));
    ui->Convert->setToolTip(L10n::text(language, "dialog.tooltip.swap"));
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
        "  background: rgba(219, 222, 227, 0.96);"
        "  color: #333840;"
        "  border: 1px solid rgba(169, 175, 184, 0.95);"
        "  border-radius: 10px;"
        "  padding: 5px 10px;"
        "  selection-background-color: rgba(186, 192, 201, 0.95);"
        "  selection-color: #1F2329;"
        "}"
        "QComboBox {"
        "  padding-right: 26px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: rgba(54, 58, 65, 0.98);"
        "  color: #F2F4F7;"
        "  border: 1px solid rgba(130, 136, 146, 0.95);"
        "  outline: 0px;"
        "  selection-background-color: rgba(112, 120, 133, 0.95);"
        "  selection-color: #FFFFFF;"
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
        "  border-top: 7px solid #4D535D;"
        "  margin-right: 6px;"
        "}"
        "QLineEdit:read-only {"
        "  background: rgba(205, 210, 217, 0.96);"
        "}"
        "QPushButton {"
        "  background: rgba(214, 218, 224, 0.95);"
        "  color: #3F454D;"
        "  border: 1px solid rgba(162, 168, 177, 0.95);"
        "  border-radius: 10px;"
        "  padding: 4px 8px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(226, 230, 236, 0.98);"
        "}"
        "QPushButton:pressed {"
        "  background: rgba(196, 201, 209, 0.98);"
        "}"
    );
}
