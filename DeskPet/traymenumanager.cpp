#include "traymenumanager.h"
#include "aichatmanager.h"
#include<QApplication>
#include<QIcon>
#include <QActionGroup>

TrayMenuManager::TrayMenuManager(QWidget *parentWindow, AIChatManager *chatManager)
    : QObject{parentWindow},m_parentWindow(parentWindow),m_chatManager(chatManager),m_aiEnabledAction(nullptr),
    m_defaultStateAction(nullptr),m_sleepStateAction(nullptr),m_workStateAction(nullptr),m_celebrateStateAction(nullptr),m_sadStateAction(nullptr)
{
    //初始化托盘的图标
    m_trayIcon = new QSystemTrayIcon(this);

    //设置托盘的图标（等一下再去处理那些素材）
    m_trayIcon->setIcon(QIcon(":/lion.png"));

    //初始化右键弹出的菜单
    m_trayMenu = new QMenu(m_parentWindow);

    QAction *actionShowHide = new QAction("Show/Hide",this);
    connect(actionShowHide,&QAction::triggered,this,&TrayMenuManager::showHideRequested);

    QMenu *chatConfigMenu = new QMenu(QStringLiteral("聊天配置"), m_trayMenu);

    m_aiEnabledAction = new QAction(QStringLiteral("启用 AI 对话"), this);
    m_aiEnabledAction->setCheckable(true);
    connect(m_aiEnabledAction, &QAction::toggled, this, &TrayMenuManager::aiEnabledToggled);
    if (m_chatManager) {
        m_aiEnabledAction->setChecked(m_chatManager->isEnabled());
    }

    QAction *actionSettings = new QAction(QStringLiteral("AI 设置..."), this);
    connect(actionSettings, &QAction::triggered, this, &TrayMenuManager::settingsRequested);

    QAction *actionNewChat = new QAction(QStringLiteral("新对话"), this);
    connect(actionNewChat, &QAction::triggered, this, &TrayMenuManager::clearConversationRequested);

    chatConfigMenu->addAction(m_aiEnabledAction);
    chatConfigMenu->addAction(actionSettings);
    chatConfigMenu->addAction(actionNewChat);

    QMenu *stateControlMenu = new QMenu(QStringLiteral("状态控制"), m_trayMenu);
    QActionGroup *stateActionGroup = new QActionGroup(this);
    stateActionGroup->setExclusive(true);

    m_defaultStateAction = stateControlMenu->addAction(QStringLiteral("Default"));
    m_sleepStateAction = stateControlMenu->addAction(QStringLiteral("SLEEPING"));
    m_workStateAction = stateControlMenu->addAction(QStringLiteral("WORKING"));
    m_celebrateStateAction = stateControlMenu->addAction(QStringLiteral("CELEBRATING"));
    m_sadStateAction = stateControlMenu->addAction(QStringLiteral("SAD"));

    QAction *stateActions[] = {
        m_defaultStateAction,
        m_sleepStateAction,
        m_workStateAction,
        m_celebrateStateAction,
        m_sadStateAction
    };
    for (QAction *action : stateActions) {
        action->setCheckable(true);
        stateActionGroup->addAction(action);
    }

    connect(m_defaultStateAction, &QAction::triggered, this, &TrayMenuManager::clearForcedStateRequested);
    connect(m_sleepStateAction, &QAction::triggered, this, [this]() { emit forcedStateRequested(PetState::SLEEPING); });
    connect(m_workStateAction, &QAction::triggered, this, [this]() { emit forcedStateRequested(PetState::WORKING); });
    connect(m_celebrateStateAction, &QAction::triggered, this, [this]() { emit forcedStateRequested(PetState::CELEBRATING); });
    connect(m_sadStateAction, &QAction::triggered, this, [this]() { emit forcedStateRequested(PetState::SAD); });

    setStateSelection(false, PetState::IDLE);

    QAction *actionExit = new QAction("Exit",this);
    connect(actionExit,&QAction::triggered,this,&TrayMenuManager::exitRequested);
    connect(actionExit,&QAction::triggered,qApp,&QCoreApplication::quit);

    m_trayMenu->addAction(actionShowHide);
    m_trayMenu->addSeparator();
    m_trayMenu->addMenu(chatConfigMenu);
    m_trayMenu->addMenu(stateControlMenu);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(actionExit);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon,&QSystemTrayIcon::activated,this,&TrayMenuManager::onTrayIconActivated);

    m_trayIcon->show();
}

void TrayMenuManager::setAiEnabled(bool enabled)
{
    if (!m_aiEnabledAction) {
        return;
    }

    m_aiEnabledAction->blockSignals(true);
    m_aiEnabledAction->setChecked(enabled);
    m_aiEnabledAction->blockSignals(false);
}

bool TrayMenuManager::isAiEnabled() const
{
    return m_aiEnabledAction && m_aiEnabledAction->isChecked();
}

void TrayMenuManager::setStateSelection(bool hasForcedState, PetState forcedState)
{
    if (!m_defaultStateAction || !m_sleepStateAction || !m_workStateAction || !m_celebrateStateAction || !m_sadStateAction) {
        return;
    }

    QAction *targetAction = m_defaultStateAction;
    if (hasForcedState) {
        if (forcedState == PetState::SLEEPING) {
            targetAction = m_sleepStateAction;
        } else if (forcedState == PetState::WORKING) {
            targetAction = m_workStateAction;
        } else if (forcedState == PetState::CELEBRATING) {
            targetAction = m_celebrateStateAction;
        } else if (forcedState == PetState::SAD) {
            targetAction = m_sadStateAction;
        }
    }

    QAction *actions[] = {
        m_defaultStateAction,
        m_sleepStateAction,
        m_workStateAction,
        m_celebrateStateAction,
        m_sadStateAction
    };
    for (QAction *action : actions) {
        action->blockSignals(true);
        action->setChecked(action == targetAction);
        action->blockSignals(false);
    }
}

void TrayMenuManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason){
    if(reason == QSystemTrayIcon::DoubleClick){
        if(m_parentWindow->isVisible()){
            m_parentWindow->hide();
        }
        else{
            m_parentWindow->showNormal();
        }
    }

}
