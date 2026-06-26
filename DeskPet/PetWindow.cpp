#include "petwindow.h"

#include "aichatmanager.h"
#include "aisettingsdialog.h"
#include "chatbubbledialog.h"
#include "FileTransferDialog.h"
#include "FileTransferServer.h"
#include "QRCodeGenerator.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeData>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QStyleHints>
#include <QUuid>

#include <algorithm>

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
    setAcceptDrops(true);
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

    m_fileServer = new FileTransferServer(); // 不设 parent（moveToThread 要求）
    connect(m_fileServer, &FileTransferServer::errorOccurred, this, [this](const QString &msg) {
        ChatBubbleDialog::showMessage(this, msg);
    });
    connect(m_fileServer, &FileTransferServer::fileDownloaded, this, [this](const QString &fileName) {
        ChatBubbleDialog::showMessage(this,
            QStringLiteral("文件 %1 已被下载到手机").arg(fileName));
    });
}

PetWindow::~PetWindow()
{
    stopFileTransfer();
    delete m_fileServer;
    m_fileServer = nullptr;
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

void PetWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void PetWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (!mimeData->hasUrls())
        return;

    QStringList newPaths;
    for (const QUrl &url : mimeData->urls()) {
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        if (!m_uploadedFiles.contains(path)) {
            m_uploadedFiles.append(path);
            newPaths.append(path);
        }
    }

    if (newPaths.isEmpty())
        return;

    // 如果服务器正在运行，同步添加
    if (m_fileServer && m_fileServer->isRunning()) {
        m_fileServer->addFiles(newPaths);
    }

    QStringList newNames;
    for (const QString &path : newPaths)
        newNames.append(QFileInfo(path).fileName());

    const int total = m_uploadedFiles.size();
    QString msg;
    if (newNames.size() == 1) {
        msg = QStringLiteral("已添加「%1」，当前共 %2 个文件").arg(newNames.first()).arg(total);
    } else {
        msg = QStringLiteral("已添加 %1 个文件，当前共 %2 个文件").arg(newNames.size()).arg(total);
    }

    if (m_fileServer && m_fileServer->isRunning()) {
        msg += QStringLiteral("。手机端刷新页面即可看到");
    } else {
        msg += QStringLiteral("。输入 /文件传输 开始共享");
    }

    ChatBubbleDialog::showMessage(this, msg);
    event->acceptProposedAction();
}

void PetWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    // 文件传输运行时显示上传按钮
    QAction *uploadAct = nullptr;
    if (m_fileServer && m_fileServer->isRunning()) {
        uploadAct = menu.addAction(QStringLiteral("上传文件"));
        menu.addSeparator();
    }

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

    if (uploadAct && selected == uploadAct) {
        uploadFilesForTransfer();
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

    m_controller->setForcedState(PetState::IDLE);

    const QString userText = ChatBubbleDialog::getText(this,
        QStringLiteral("AI 对话"),
        QStringLiteral("输入你想对宠物说的话（/文件传输 启动文件共享）"));

    if (hadForcedState) {
        m_controller->setForcedState(previousForcedState);
    } else {
        m_controller->clearForcedState();
    }

    if (userText.isEmpty()) {
        return;
    }

    if (userText.trimmed().startsWith(QStringLiteral("/文件传输"))) {
        handleFileTransferCommand();
        return;
    }

    m_aiManager->sendUserMessage(userText);
}

void PetWindow::handleFileTransferCommand()
{
    // 如果已经在运行，先停止
    if (m_fileServer->isRunning() || m_fileTransferDialog) {
        stopFileTransfer();
    }

    // 清理已经不存在的文件
    const auto it = std::remove_if(m_uploadedFiles.begin(), m_uploadedFiles.end(),
                                   [](const QString &path) { return !QFile::exists(path); });
    m_uploadedFiles.erase(it, m_uploadedFiles.end());

    const QString ipv6 = FileTransferServer::getStrictGlobalIPv6();
    if (ipv6.isEmpty()) {
        ChatBubbleDialog::showMessage(this,
            QStringLiteral("未检测到公网IPv6，请检查网络连接。"));
        return;
    }

    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const quint16 port = 8080;

    if (!m_fileServer->start(port, m_uploadedFiles, token)) {
        ChatBubbleDialog::showMessage(this,
            QStringLiteral("文件共享服务启动失败，请稍后重试。"));
        return;
    }

    const QString url = QString("http://[%1]:%2/?token=%3").arg(ipv6).arg(port).arg(token);
    const QPixmap qr = QRCodeGenerator::generateQRCode(url, 300);

    m_fileTransferDialog = new FileTransferDialog(qr, ipv6, port,
                                                  m_uploadedFiles.size(), nullptr);
    connect(m_fileTransferDialog, &FileTransferDialog::stopRequested,
            this, &PetWindow::stopFileTransfer);

    const QPoint petTopRight = frameGeometry().topRight();
    m_fileTransferDialog->move(petTopRight + QPoint(10, 0));

    m_fileTransferDialog->show();

    const int count = m_uploadedFiles.size();
    if (count == 0) {
        ChatBubbleDialog::showMessage(this,
            QStringLiteral("共享列表为空。右键宠物→上传文件 来添加文件。注意：手机需连接同一Wi-Fi。"));
    } else {
        ChatBubbleDialog::showMessage(this,
            QStringLiteral("共享 %1 个文件，请用手机扫码下载。右键宠物可继续上传。").arg(count));
    }
}

void PetWindow::uploadFilesForTransfer()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("选择要共享的文件"), QString(),
        QStringLiteral("所有文件 (*)"));

    if (files.isEmpty())
        return;

    QStringList newNames;
    for (const QString &path : files) {
        if (!m_uploadedFiles.contains(path)) {
            m_uploadedFiles.append(path);
            newNames.append(QFileInfo(path).fileName());
        }
    }

    if (newNames.isEmpty())
        return;

    // 同步到正在运行的服务器（线程安全）
    m_fileServer->addFiles(files);

    const int total = m_uploadedFiles.size();
    QString msg;
    if (newNames.size() == 1) {
        msg = QStringLiteral("已上传「%1」，当前共 %2 个文件。手机端刷新页面即可看到。")
                  .arg(newNames.first()).arg(total);
    } else {
        msg = QStringLiteral("已上传 %1 个文件，当前共 %2 个文件。手机端刷新页面即可看到。")
                  .arg(newNames.size()).arg(total);
    }

    ChatBubbleDialog::showMessage(this, msg);
}

void PetWindow::stopFileTransfer()
{
    if (m_fileServer) {
        m_fileServer->stop();
    }

    if (m_fileTransferDialog) {
        m_fileTransferDialog->close();
        m_fileTransferDialog = nullptr;
    }
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
