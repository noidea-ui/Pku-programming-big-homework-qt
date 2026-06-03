#include "aichatmanager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

namespace {
constexpr char kSettingsGroup[] = "AIChat";
constexpr char kEnabledKey[] = "enabled";
constexpr char kApiKeyKey[] = "apiKey";
constexpr char kEndpointKey[] = "endpointUrl";
constexpr char kModelKey[] = "model";
constexpr char kSystemPromptKey[] = "systemPrompt";
constexpr char kTemperatureKey[] = "temperature";
constexpr char kHistoryLimitKey[] = "historyLimit";
}

AIChatManager::AIChatManager(QObject *parent)
    : QObject(parent)
{
    loadSettings();
}

AIChatSettings AIChatManager::settings() const
{
    return m_settings;
}

void AIChatManager::setSettings(const AIChatSettings &settings)
{
    m_settings = settings;
    trimHistory();
    saveSettings();
    emit settingsChanged(m_settings);
}

void AIChatManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    m_settings.enabled = settings.value(QString::fromLatin1(kEnabledKey), m_settings.enabled).toBool();
    m_settings.apiKey = decodeApiKey(settings.value(QString::fromLatin1(kApiKeyKey)).toString());
    m_settings.endpointUrl = settings.value(QString::fromLatin1(kEndpointKey), m_settings.endpointUrl).toString();
    m_settings.model = settings.value(QString::fromLatin1(kModelKey), m_settings.model).toString();
    m_settings.systemPrompt = settings.value(QString::fromLatin1(kSystemPromptKey), m_settings.systemPrompt).toString();
    m_settings.temperature = settings.value(QString::fromLatin1(kTemperatureKey), m_settings.temperature).toDouble();
    m_settings.historyLimit = settings.value(QString::fromLatin1(kHistoryLimitKey), m_settings.historyLimit).toInt();
    settings.endGroup();
    trimHistory();
}

void AIChatManager::saveSettings() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    settings.setValue(QString::fromLatin1(kEnabledKey), m_settings.enabled);
    settings.setValue(QString::fromLatin1(kApiKeyKey), encodeApiKey(m_settings.apiKey));
    settings.setValue(QString::fromLatin1(kEndpointKey), m_settings.endpointUrl);
    settings.setValue(QString::fromLatin1(kModelKey), m_settings.model);
    settings.setValue(QString::fromLatin1(kSystemPromptKey), m_settings.systemPrompt);
    settings.setValue(QString::fromLatin1(kTemperatureKey), m_settings.temperature);
    settings.setValue(QString::fromLatin1(kHistoryLimitKey), m_settings.historyLimit);
    settings.endGroup();
}

bool AIChatManager::isEnabled() const
{
    return m_settings.enabled;
}

void AIChatManager::setEnabled(bool enabled)
{
    if (m_settings.enabled == enabled) {
        return;
    }
    m_settings.enabled = enabled;
    saveSettings();
    emit settingsChanged(m_settings);
}

void AIChatManager::clearConversation()
{
    m_history.clear();
}

void AIChatManager::sendUserMessage(const QString &message)
{
    const QString trimmedMessage = message.trimmed();
    if (trimmedMessage.isEmpty()) {
        emit requestFailed(QStringLiteral("请输入有效的聊天内容。"));
        return;
    }
    if (!m_settings.enabled) {
        emit requestFailed(QStringLiteral("AI 对话尚未启用。"));
        return;
    }
    if (m_settings.apiKey.trimmed().isEmpty()) {
        emit requestFailed(QStringLiteral("请先在设置中填写 API-Key。"));
        return;
    }

    const QUrl endpointUrl(m_settings.endpointUrl.trimmed());
    if (!endpointUrl.isValid() || endpointUrl.scheme().isEmpty()) {
        emit requestFailed(QStringLiteral("API 端点 URL 无效，请检查设置。"));
        return;
    }

    appendHistory(QStringLiteral("user"), trimmedMessage);

    QJsonObject root;
    root.insert(QStringLiteral("model"), m_settings.model);
    root.insert(QStringLiteral("messages"), buildMessagesPayload());
    root.insert(QStringLiteral("temperature"), m_settings.temperature);

    QNetworkRequest request(endpointUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_settings.apiKey).toUtf8());

    QNetworkReply *reply = m_networkManager.post(request, QJsonDocument(root).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(parseErrorMessage(payload, httpStatus));
            reply->deleteLater();
            return;
        }

        QJsonParseError jsonError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &jsonError);
        if (jsonError.error != QJsonParseError::NoError || !document.isObject()) {
            emit requestFailed(QStringLiteral("AI 返回内容无法解析。"));
            reply->deleteLater();
            return;
        }

        const QJsonObject rootObject = document.object();
        const QJsonArray choices = rootObject.value(QStringLiteral("choices")).toArray();
        if (choices.isEmpty()) {
            emit requestFailed(QStringLiteral("AI 返回中没有可用回复。"));
            reply->deleteLater();
            return;
        }

        const QJsonObject choiceObject = choices.first().toObject();
        const QJsonObject messageObject = choiceObject.value(QStringLiteral("message")).toObject();
        const QString assistantReply = messageObject.value(QStringLiteral("content")).toString().trimmed();
        if (assistantReply.isEmpty()) {
            emit requestFailed(QStringLiteral("AI 返回了空回复。"));
            reply->deleteLater();
            return;
        }

        appendHistory(QStringLiteral("assistant"), assistantReply);
        emit replyReady(assistantReply);
        reply->deleteLater();
    });
}

QString AIChatManager::decodeApiKey(const QString &value) const
{
    if (!value.startsWith(QStringLiteral("b64:"))) {
        return value;
    }
    const QByteArray encoded = value.mid(4).toUtf8();
    return QString::fromUtf8(QByteArray::fromBase64(encoded));
}

QString AIChatManager::encodeApiKey(const QString &value) const
{
    if (value.isEmpty()) {
        return {};
    }
    return QStringLiteral("b64:%1").arg(QString::fromUtf8(value.toUtf8().toBase64()));
}

void AIChatManager::trimHistory()
{
    const int limit = qMax(0, m_settings.historyLimit);
    while (m_history.size() > limit) {
        m_history.removeFirst();
    }
}

void AIChatManager::appendHistory(const QString &role, const QString &content)
{
    m_history.append(ChatMessage{role, content});
    trimHistory();
}

QJsonArray AIChatManager::buildMessagesPayload() const
{
    QJsonArray messages;

    if (!m_settings.systemPrompt.trimmed().isEmpty()) {
        QJsonObject systemMessage;
        systemMessage.insert(QStringLiteral("role"), QStringLiteral("system"));
        systemMessage.insert(QStringLiteral("content"), m_settings.systemPrompt);
        messages.append(systemMessage);
    }

    for (const ChatMessage &message : m_history) {
        QJsonObject jsonMessage;
        jsonMessage.insert(QStringLiteral("role"), message.role);
        jsonMessage.insert(QStringLiteral("content"), message.content);
        messages.append(jsonMessage);
    }

    return messages;
}

QString AIChatManager::parseErrorMessage(const QByteArray &payload, int httpStatus) const
{
    QString prefix;
    switch (httpStatus) {
    case 401:
        prefix = QStringLiteral("鉴权失败：API-Key 无效或已过期。 ");
        break;
    case 429:
        prefix = QStringLiteral("请求过于频繁：服务端正在限流。 ");
        break;
    case 500:
    case 502:
    case 503:
    case 504:
        prefix = QStringLiteral("服务端暂时不可用。 ");
        break;
    default:
        break;
    }

    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (document.isObject()) {
        const QJsonObject rootObject = document.object();
        const QJsonObject errorObject = rootObject.value(QStringLiteral("error")).toObject();
        const QString errorMessage = errorObject.value(QStringLiteral("message")).toString().trimmed();
        if (!errorMessage.isEmpty()) {
            return prefix + errorMessage;
        }
    }

    if (httpStatus > 0) {
        return prefix + QStringLiteral("HTTP %1。请检查接口地址、模型名与网络连接。").arg(httpStatus);
    }
    return prefix + QStringLiteral("网络请求失败，请检查网络连接或接口地址。");
}