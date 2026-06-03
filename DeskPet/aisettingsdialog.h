#ifndef AISETTINGSDIALOG_H
#define AISETTINGSDIALOG_H

#include "aichatmanager.h"

#include <QDialog>

class QCheckBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;
class QTextEdit;

class AISettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AISettingsDialog(QWidget *parent = nullptr);

    void setSettings(const AIChatSettings &settings);
    AIChatSettings settings() const;

signals:
    void clearConversationRequested();

private:
    QCheckBox *m_enabledCheckBox;
    QLineEdit *m_apiKeyEdit;
    QLineEdit *m_endpointEdit;
    QLineEdit *m_modelEdit;
    QTextEdit *m_systemPromptEdit;
    QDoubleSpinBox *m_temperatureSpinBox;
    QPushButton *m_clearConversationButton;
};

#endif // AISETTINGSDIALOG_H