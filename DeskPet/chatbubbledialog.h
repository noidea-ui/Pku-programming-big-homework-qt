#ifndef CHATBUBBLEDIALOG_H
#define CHATBUBBLEDIALOG_H

#include <QDialog>
#include <QPixmap>

class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QWidget;

class ChatBubbleDialog : public QDialog
{
    Q_OBJECT
public:
    enum class Mode {
        Input,
        Display
    };

    static QString getText(QWidget *anchor, const QString &title, const QString &placeholder);
    static ChatBubbleDialog *showMessage(QWidget *anchor, const QString &text);

    explicit ChatBubbleDialog(Mode mode, QWidget *parent = nullptr);

    void setMessageText(const QString &text);
    void setPromptText(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupInputMode();
    void setupDisplayMode();
    void repositionNearAnchor(QWidget *anchor);

    Mode m_mode;
    QString m_messageText;
    QPixmap m_background;
    QLabel *m_messageLabel;
    QLabel *m_displayLabel;
    QLineEdit *m_inputEdit;
    QScrollArea *m_scrollArea;
    QPushButton *m_sendButton;
    QPushButton *m_closeButton;
};

#endif // CHATBUBBLEDIALOG_H