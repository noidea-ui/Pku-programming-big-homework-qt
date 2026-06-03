#include "aisettingsdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {
const QString kDefaultEndpoint = QStringLiteral("https://api.deepseek.com/chat/completions");
}

AISettingsDialog::AISettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_enabledCheckBox(new QCheckBox(QStringLiteral("启用 AI 对话"), this))
    , m_apiKeyEdit(new QLineEdit(this))
    , m_endpointEdit(new QLineEdit(this))
    , m_modelEdit(new QLineEdit(this))
    , m_systemPromptEdit(new QTextEdit(this))
    , m_temperatureSpinBox(new QDoubleSpinBox(this))
    , m_clearConversationButton(new QPushButton(QStringLiteral("清空上下文"), this))
{
    setWindowTitle(QStringLiteral("AI 对话设置"));

    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(QStringLiteral("输入 API-Key"));
    m_endpointEdit->setPlaceholderText(kDefaultEndpoint);
    m_endpointEdit->setText(kDefaultEndpoint);
    m_modelEdit->setPlaceholderText(QStringLiteral("deepseek-v4-flash"));
    m_systemPromptEdit->setPlaceholderText(QStringLiteral("你是一只北京大学信息科学技术学院的吉祥物——工程狮，喜欢用短句和颜文字。输出时不需要描述动作表情。"));
    m_systemPromptEdit->setMinimumHeight(120);

    m_temperatureSpinBox->setRange(0.0, 2.0);
    m_temperatureSpinBox->setSingleStep(0.1);
    m_temperatureSpinBox->setDecimals(2);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(m_enabledCheckBox);
    formLayout->addRow(QStringLiteral("API-Key"), m_apiKeyEdit);
    formLayout->addRow(QStringLiteral("API 端点"), m_endpointEdit);
    formLayout->addRow(QStringLiteral("模型名称"), m_modelEdit);
    formLayout->addRow(QStringLiteral("系统提示词"), m_systemPromptEdit);
    formLayout->addRow(QStringLiteral("Temperature"), m_temperatureSpinBox);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_clearConversationButton, &QPushButton::clicked, this, &AISettingsDialog::clearConversationRequested);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(m_clearConversationButton);
    bottomLayout->addStretch();
    bottomLayout->addWidget(buttonBox);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(bottomLayout);
    setLayout(mainLayout);
    resize(540, 420);
}

void AISettingsDialog::setSettings(const AIChatSettings &settings)
{
    m_enabledCheckBox->setChecked(settings.enabled);
    m_apiKeyEdit->setText(settings.apiKey);
    m_endpointEdit->setText(settings.endpointUrl.trimmed().isEmpty() ? kDefaultEndpoint : settings.endpointUrl);
    m_modelEdit->setText(settings.model);
    m_systemPromptEdit->setPlainText(settings.systemPrompt);
    m_temperatureSpinBox->setValue(settings.temperature);
}

AIChatSettings AISettingsDialog::settings() const
{
    AIChatSettings settings;
    settings.enabled = m_enabledCheckBox->isChecked();
    settings.apiKey = m_apiKeyEdit->text().trimmed();
    settings.endpointUrl = m_endpointEdit->text().trimmed();
    if (settings.endpointUrl.isEmpty()) {
        settings.endpointUrl = kDefaultEndpoint;
    }
    settings.model = m_modelEdit->text().trimmed();
    settings.systemPrompt = m_systemPromptEdit->toPlainText().trimmed();
    settings.temperature = m_temperatureSpinBox->value();
    return settings;
}