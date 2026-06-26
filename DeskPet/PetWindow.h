#ifndef PETWINDOW_H
#define PETWINDOW_H

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QStringList>
#include <QWidget>

#include "petcontroller.h"
#include "traymenumanager.h"

class AIChatManager;
class AISettingsDialog;
class FileTransferDialog;
class FileTransferServer;

class PetWindow : public QWidget
{
    Q_OBJECT
public:
    PetWindow(QWidget *parent = nullptr);
    ~PetWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onFrameUpdated();
    void setWindowPosition(const QPoint &pos);
    void handleTrayShowHide();
    void openAiSettings();
    void setAiEnabled(bool enabled);
    void clearConversation();
    void clearForcedStateFromTray();
    void setForcedStateFromTray(PetState state);
    void onAiReplyReady(const QString &replyText);
    void onAiRequestFailed(const QString &message);
    void openChatInput();
    void handleFileTransferCommand();
    void uploadFilesForTransfer();

private:
    void syncTrayStateSelection();
    void stopFileTransfer();

    PetController *m_controller;
    TrayMenuManager *m_trayManager;
    AIChatManager *m_aiManager;
    AISettingsDialog *m_settingsDialog;
    FileTransferServer *m_fileServer = nullptr;
    FileTransferDialog *m_fileTransferDialog = nullptr;
    QStringList m_uploadedFiles;
    QPoint m_dragPosition;
    QPoint m_pressGlobalPosition;
    bool m_isDragging;
    bool m_pressStartedOnPet;
};

#endif // PETWINDOW_H
