#ifndef TRAYMENUMANAGER_H
#define TRAYMENUMANAGER_H

#include <QObject>
#include<QSystemTrayIcon>
#include<QMenu>
#include<QAction>
#include<QWidget>


class TrayMenuManager : public QObject
{
    Q_OBJECT
public:
    explicit TrayMenuManager(QWidget *parentWindow);
private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason);

private:
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QWidget *m_parentWindow;
};

#endif // TRAYMENUMANAGER_H
