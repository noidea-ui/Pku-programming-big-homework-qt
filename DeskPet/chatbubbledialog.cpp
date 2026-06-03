#include "chatbubbledialog.h"

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

ChatBubbleDialog::ChatBubbleDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
    , m_background(mode == Mode::Input ? QPixmap(QStringLiteral(":/images/thoughtbubble.png")) : QPixmap(QStringLiteral(":/images/bubble.png")))
    , m_messageLabel(new QLabel(this))
    , m_displayLabel(new QLabel(this))
    , m_inputEdit(new QLineEdit(this))
    , m_scrollArea(new QScrollArea(this))
    , m_sendButton(new QPushButton(QStringLiteral("发送"), this))
    , m_closeButton(new QPushButton(QStringLiteral("关闭"), this))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    if (!m_background.isNull()) {
        setFixedSize(m_background.size());
    } else {
        setFixedSize(420, 240);
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(42, 30, 42, 28);
    mainLayout->setSpacing(10);

    m_messageLabel->setWordWrap(true);
    m_messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_displayLabel->setWordWrap(true);
    m_displayLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setWidget(m_displayLabel);

    if (m_mode == Mode::Input) {
        m_displayLabel->hide();
        m_scrollArea->hide();

        setupInputMode();
        mainLayout->addWidget(m_messageLabel);
        mainLayout->addWidget(m_inputEdit);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_sendButton);
        buttonLayout->addWidget(m_closeButton);
        mainLayout->addLayout(buttonLayout);

        connect(m_sendButton, &QPushButton::clicked, this, &QDialog::accept);
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_inputEdit, &QLineEdit::returnPressed, this, &QDialog::accept);
    } else {
        m_messageLabel->hide();
        m_inputEdit->hide();
        m_sendButton->hide();

        setupDisplayMode();
        mainLayout->addWidget(m_scrollArea, 1);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_closeButton);
        mainLayout->addLayout(buttonLayout);

        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
    }
}

QString ChatBubbleDialog::getText(QWidget *anchor, const QString &title, const QString &placeholder)
{
    ChatBubbleDialog dialog(Mode::Input, anchor);
    dialog.setWindowTitle(title);
    dialog.setPromptText(placeholder);
    dialog.repositionNearAnchor(anchor);
    if (dialog.exec() == QDialog::Accepted) {
        return dialog.m_inputEdit->text().trimmed();
    }
    return {};
}

ChatBubbleDialog *ChatBubbleDialog::showMessage(QWidget *anchor, const QString &text)
{
    ChatBubbleDialog *dialog = new ChatBubbleDialog(Mode::Display, anchor);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setMessageText(text);
    dialog->repositionNearAnchor(anchor);
    dialog->show();
    QTimer::singleShot(6500, dialog, &QDialog::close);
    return dialog;
}

void ChatBubbleDialog::setMessageText(const QString &text)
{
    m_messageText = text;
    if (m_mode == Mode::Display) {
        m_displayLabel->setText(text);
        m_scrollArea->verticalScrollBar()->setValue(0);
    } else {
        m_messageLabel->setText(text);
    }
}

void ChatBubbleDialog::setPromptText(const QString &text)
{
    if (m_mode == Mode::Input) {
        m_messageLabel->setText(text);
        m_inputEdit->setFocus();
    }
}

void ChatBubbleDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!m_background.isNull()) {
        painter.drawPixmap(rect(), m_background);
    } else {
        painter.fillRect(rect(), QColor(255, 255, 255, 230));
    }
}

void ChatBubbleDialog::setupInputMode()
{
    m_messageLabel->setText(QStringLiteral("和我说点什么吧"));
    m_inputEdit->setPlaceholderText(QStringLiteral("例如：今天心情怎么样？"));
}

void ChatBubbleDialog::setupDisplayMode()
{
    m_displayLabel->setText(QStringLiteral("正在思考..."));
    m_closeButton->setText(QStringLiteral("知道了"));
}

void ChatBubbleDialog::repositionNearAnchor(QWidget *anchor)
{
    if (!anchor) {
        return;
    }

    const QRect anchorRect = anchor->frameGeometry();
    QPoint pos = anchorRect.topRight() + QPoint(12, -20);

    if (QScreen *screen = QGuiApplication::screenAt(anchorRect.center())) {
        const QRect available = screen->availableGeometry();
        if (pos.x() + width() > available.right()) {
            pos.setX(available.right() - width());
        }
        if (pos.y() + height() > available.bottom()) {
            pos.setY(available.bottom() - height());
        }
        if (pos.x() < available.left()) {
            pos.setX(available.left());
        }
        if (pos.y() < available.top()) {
            pos.setY(available.top());
        }
    }

    move(pos);
}