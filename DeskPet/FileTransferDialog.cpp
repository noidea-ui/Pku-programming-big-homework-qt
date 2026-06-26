#include "FileTransferDialog.h"

#include <QCloseEvent>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

FileTransferDialog::FileTransferDialog(const QPixmap &qrPixmap,
                                       const QString &ipv6,
                                       quint16 port,
                                       int fileCount,
                                       QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("文件传输"));
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    auto *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    // QR code
    m_qrLabel = new QLabel(this);
    m_qrLabel->setPixmap(qrPixmap);
    m_qrLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_qrLabel);

    // URL text
    const QString url = QString("http://[%1]:%2/?token=***").arg(ipv6).arg(port);
    m_urlLabel = new QLabel(url, this);
    m_urlLabel->setWordWrap(true);
    m_urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_urlLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_urlLabel);

    layout->addSpacing(8);

    // Status with file count
    m_statusLabel = new QLabel(
        QStringLiteral("共享 %1 个文件，请用手机扫码访问").arg(fileCount), this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
    layout->addWidget(m_statusLabel);

    layout->addSpacing(4);

    // Warnings
    auto *warnLabel = new QLabel(this);
    warnLabel->setWordWrap(true);
    warnLabel->setText(
        QStringLiteral("请确保手机与电脑连接 <b>同一 Wi-Fi</b>（同一局域网）。"
                       "如手机无法访问，请检查 Windows 防火墙是否放行 TCP %1 端口。")
            .arg(port));
    warnLabel->setStyleSheet("color: #c0392b; background: #fdecea; padding: 8px; border-radius: 4px;");
    layout->addWidget(warnLabel);

    layout->addSpacing(8);

    // Stop button
    m_stopButton = new QPushButton(QStringLiteral("停止传输"), this);
    m_stopButton->setStyleSheet(
        "QPushButton { background: #e74c3c; color: white; padding: 8px 24px; border-radius: 4px; }"
        "QPushButton:hover { background: #c0392b; }");
    layout->addWidget(m_stopButton);

    connect(m_stopButton, &QPushButton::clicked, this, &FileTransferDialog::stopRequested);
}

void FileTransferDialog::closeEvent(QCloseEvent *event)
{
    emit stopRequested();
    event->accept();
}
