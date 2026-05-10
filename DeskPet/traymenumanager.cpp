#include "traymenumanager.h"
#include<QApplication>
#include<QIcon>

TrayMenuManager::TrayMenuManager(QWidget *parentWindow)
    : QObject{parentWindow},m_parentWindow(parentWindow)
{
    //初始化托盘的图标
    m_trayIcon = new QSystemTrayIcon(this);

    //设置托盘的图标（等一下再去处理那些素材）
    m_trayIcon->setIcon(QIcon(":/lion.png"));

    //初始化右键弹出的菜单
    m_trayMenu = new QMenu(m_parentWindow);

    QAction *actionShowHide = new QAction("Show/Hide",this);
    connect(actionShowHide,&QAction::triggered,[this](){
        if(m_parentWindow->isVisible()){
            m_parentWindow->hide();
        }
        else{
            m_parentWindow->showNormal();
        }
    });

    QAction *actionExit = new QAction("Exit",this);
    connect(actionExit,&QAction::triggered,qApp,&QCoreApplication::quit);

    m_trayMenu->addAction(actionShowHide);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(actionExit);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon,&QSystemTrayIcon::activated,this,&TrayMenuManager::onTrayIconActivated);

    m_trayIcon->show();
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
