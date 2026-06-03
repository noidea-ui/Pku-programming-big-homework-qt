#include "petwindow.h"

#include "aichatmanager.h"
#include "aisettingsdialog.h"
#include "chatbubbledialog.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QStyleHints>

PetWindow::PetWindow(QWidget *parent)
    : QWidget(parent)
    , m_controller(nullptr)
    , m_trayManager(nullptr)
    , m_aiManager(new AIChatManager(this))
    , m_settingsDialog(nullptr)
    , m_isDragging(false)
    , m_pressStartedOnPet(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(180, 180);

    m_controller = new PetController(this);
    connect(m_controller, &PetController::frameUpdated, this, &PetWindow::onFrameUpdated);
    connect(m_controller, &PetController::positionChanged, this, &PetWindow::setWindowPosition);

    m_trayManager = new TrayMenuManager(this, m_aiManager);
    connect(m_trayManager, &TrayMenuManager::showHideRequested, this, &PetWindow::handleTrayShowHide);
    connect(m_trayManager, &TrayMenuManager::settingsRequested, this, &PetWindow::openAiSettings);
    connect(m_trayManager, &TrayMenuManager::clearConversationRequested, this, &PetWindow::clearConversation);
    connect(m_trayManager, &TrayMenuManager::aiEnabledToggled, this, &PetWindow::setAiEnabled);
    connect(m_trayManager, &TrayMenuManager::clearForcedStateRequested, this, &PetWindow::clearForcedStateFromTray);
    connect(m_trayManager, &TrayMenuManager::forcedStateRequested, this, &PetWindow::setForcedStateFromTray);
    connect(m_trayManager, &TrayMenuManager::exitRequested, qApp, &QCoreApplication::quit);
    m_trayManager->setAiEnabled(m_aiManager->isEnabled());
    syncTrayStateSelection();

    connect(m_aiManager, &AIChatManager::replyReady, this, &PetWindow::onAiReplyReady);
    connect(m_aiManager, &AIChatManager::requestFailed, this, &PetWindow::onAiRequestFailed);
}

PetWindow::~PetWindow()
{
}

void PetWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPixmap currentFrame = m_controller->getCurrentFrame();
    if (!currentFrame.isNull()) {
        painter.drawPixmap(rect(), currentFrame);
    } else {
        static QPixmap s_lionPixmap(QStringLiteral(":/lion.png"));
        if (!s_lionPixmap.isNull()) {
            painter.drawPixmap(rect(), s_lionPixmap);
        } else {
            painter.setBrush(QColor(255, 200, 0, 200));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(rect());

            painter.setPen(Qt::black);
            painter.drawText(rect(), Qt::AlignCenter, "载入\n素材中...");
        }
    }
}

void PetWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    m_pressStartedOnPet = true;
    m_isDragging = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_pressGlobalPosition = event->globalPosition().toPoint();
#else
    m_pressGlobalPosition = event->globalPos();
#endif
    m_dragPosition = m_pressGlobalPosition - frameGeometry().topLeft();
    event->accept();
}

void PetWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_pressStartedOnPet || !(event->buttons() & Qt::LeftButton)) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPoint currentPos = event->globalPosition().toPoint();
#else
    const QPoint currentPos = event->globalPos();
#endif

    if (!m_isDragging) {
        if ((currentPos - m_pressGlobalPosition).manhattanLength() < qApp->styleHints()->startDragDistance()) {
            return;
        }
        m_isDragging = true;
        m_controller->startDrag();
    }

    move(currentPos - m_dragPosition);
    event->accept();
}

void PetWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    const bool wasDragging = m_isDragging;
    m_pressStartedOnPet = false;
    m_isDragging = false;

    if (wasDragging) {
        m_controller->stopDrag();
    } else {
        m_controller->changeState(PetState::IDLE);
        if (m_aiManager->isEnabled()) {
            openChatInput();
        }
    }

    event->accept();
}

void PetWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 保留扩展点
    }
}

void PetWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *defaultAct = menu.addAction("Default");
    menu.addSeparator();

    QAction *sleepAct = menu.addAction("SLEEPING");
    QAction *workAct = menu.addAction("WORKING");
    QAction *celebrateAct = menu.addAction("CELEBRATING");
    QAction *sadAct = menu.addAction("SAD");

    defaultAct->setCheckable(true);
    sleepAct->setCheckable(true);
    workAct->setCheckable(true);
    celebrateAct->setCheckable(true);
    sadAct->setCheckable(true);

    bool hasForced = m_controller->hasForcedState();
    defaultAct->setChecked(!hasForced);
    if (hasForced) {
        PetState fs = m_controller->forcedState();
        sleepAct->setChecked(fs == PetState::SLEEPING);
        workAct->setChecked(fs == PetState::WORKING);
        celebrateAct->setChecked(fs == PetState::CELEBRATING);
        sadAct->setChecked(fs == PetState::SAD);
    }

    QAction *selected = menu.exec(event->globalPos());
    if (!selected) {
        return;
    }

    if (selected == defaultAct) {
        m_controller->clearForcedState();
    } else if (selected == sleepAct) {
        m_controller->setForcedState(PetState::SLEEPING);
    } else if (selected == workAct) {
        m_controller->setForcedState(PetState::WORKING);
    } else if (selected == celebrateAct) {
        m_controller->setForcedState(PetState::CELEBRATING);
    } else if (selected == sadAct) {
        m_controller->setForcedState(PetState::SAD);
    }

    syncTrayStateSelection();

    event->accept();
}

void PetWindow::handleTrayShowHide()
{
    if (isVisible()) {
        hide();
    } else {
        showNormal();
        raise();
        activateWindow();
    }
}

void PetWindow::openAiSettings()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new AISettingsDialog(this);
        connect(m_settingsDialog, &AISettingsDialog::clearConversationRequested, this, &PetWindow::clearConversation);
    }

    m_settingsDialog->setSettings(m_aiManager->settings());
    if (m_settingsDialog->exec() == QDialog::Accepted) {
        m_aiManager->setSettings(m_settingsDialog->settings());
        m_trayManager->setAiEnabled(m_aiManager->isEnabled());
    }
}

void PetWindow::setAiEnabled(bool enabled)
{
    AIChatSettings settings = m_aiManager->settings();
    settings.enabled = enabled;
    m_aiManager->setSettings(settings);
    m_trayManager->setAiEnabled(enabled);
}

void PetWindow::clearConversation()
{
    m_aiManager->clearConversation();
    ChatBubbleDialog::showMessage(this, QStringLiteral("已清空上下文，可以开始新对话。"));
}

void PetWindow::clearForcedStateFromTray()
{
    m_controller->clearForcedState();
    syncTrayStateSelection();
}

void PetWindow::setForcedStateFromTray(PetState state)
{
    m_controller->setForcedState(state);
    syncTrayStateSelection();
}

void PetWindow::onAiReplyReady(const QString &replyText)
{
    ChatBubbleDialog::showMessage(this, replyText);
}

void PetWindow::onAiRequestFailed(const QString &message)
{
    QMessageBox::warning(this, QStringLiteral("AI 对话失败"), message);
}

void PetWindow::openChatInput()
{
    const bool hadForcedState = m_controller->hasForcedState();
    const PetState previousForcedState = hadForcedState ? m_controller->forcedState() : PetState::IDLE;

    // 输入期间强制保持待机，避免宠物移动导致气泡脱离。
    m_controller->setForcedState(PetState::IDLE);

    const QString userText = ChatBubbleDialog::getText(this, QStringLiteral("AI 对话"), QStringLiteral("输入你想对宠物说的话"));

    if (hadForcedState) {
        m_controller->setForcedState(previousForcedState);
    } else {
        m_controller->clearForcedState();
    }

    if (userText.isEmpty()) {
        return;
    }

    m_aiManager->sendUserMessage(userText);
}

void PetWindow::onFrameUpdated()
{
    update();
}

void PetWindow::setWindowPosition(const QPoint &pos)
{
    move(pos);
}

void PetWindow::syncTrayStateSelection()
{
    m_trayManager->setStateSelection(m_controller->hasForcedState(), m_controller->forcedState());
}