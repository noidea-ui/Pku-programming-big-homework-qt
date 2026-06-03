#ifndef TRAYMENUMANAGER_H
#define TRAYMENUMANAGER_H

#include <QObject>
#include<QSystemTrayIcon>
#include<QMenu>
#include<QAction>
#include<QWidget>

#include "animationmanager.h"

class AIChatManager;


class TrayMenuManager : public QObject
{
    Q_OBJECT
public:
    explicit TrayMenuManager(QWidget *parentWindow, AIChatManager *chatManager);
    void setAiEnabled(bool enabled);
    bool isAiEnabled() const;
    void setStateSelection(bool hasForcedState, PetState forcedState);

signals:
    void showHideRequested();
    void settingsRequested();
    void clearConversationRequested();
    void aiEnabledToggled(bool enabled);
    void clearForcedStateRequested();
    void forcedStateRequested(PetState state);
    void exitRequested();
private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason);

private:
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QWidget *m_parentWindow;
    AIChatManager *m_chatManager;
    QAction *m_aiEnabledAction;
    QAction *m_defaultStateAction;
    QAction *m_sleepStateAction;
    QAction *m_workStateAction;
    QAction *m_celebrateStateAction;
    QAction *m_sadStateAction;
};

#endif // TRAYMENUMANAGER_H
