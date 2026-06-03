#ifndef AICHATMANAGER_H
#define AICHATMANAGER_H

#include <QObject>
#include <QList>
#include <QJsonArray>
#include <QNetworkAccessManager>

struct AIChatSettings
{
    bool enabled = false;
    QString apiKey;
    QString endpointUrl = QStringLiteral("https://api.deepseek.com/chat/completions");
    QString model = QStringLiteral("gpt-3.5-turbo");
    QString systemPrompt = QStringLiteral("你是一只活泼的小猫，喜欢用短句和颜文字。");
    double temperature = 0.7;
    int historyLimit = 10;
};

class AIChatManager : public QObject
{
    Q_OBJECT
public:
    explicit AIChatManager(QObject *parent = nullptr);

    AIChatSettings settings() const;
    void setSettings(const AIChatSettings &settings);
    void loadSettings();
    void saveSettings() const;

    bool isEnabled() const;
    void setEnabled(bool enabled);

    void clearConversation();
    void sendUserMessage(const QString &message);

signals:
    void replyReady(const QString &reply);
    void requestFailed(const QString &message);
    void settingsChanged(const AIChatSettings &settings);

private:
    struct ChatMessage
    {
        QString role;
        QString content;
    };

    QString decodeApiKey(const QString &value) const;
    QString encodeApiKey(const QString &value) const;
    void trimHistory();
    void appendHistory(const QString &role, const QString &content);
    QJsonArray buildMessagesPayload() const;
    QString parseErrorMessage(const QByteArray &payload, int httpStatus) const;

    AIChatSettings m_settings;
    QList<ChatMessage> m_history;
    QNetworkAccessManager m_networkManager;
};

#endif // AICHATMANAGER_H