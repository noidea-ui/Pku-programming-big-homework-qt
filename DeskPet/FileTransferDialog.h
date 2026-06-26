#ifndef FILETRANSFERDIALOG_H
#define FILETRANSFERDIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;

class FileTransferDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FileTransferDialog(const QPixmap &qrPixmap,
                                const QString &ipv6,
                                quint16 port,
                                int fileCount,
                                QWidget *parent = nullptr);

signals:
    void stopRequested();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void repositionNearPet();

    QLabel *m_qrLabel;
    QLabel *m_urlLabel;
    QLabel *m_statusLabel;
    QPushButton *m_stopButton;
};

#endif // FILETRANSFERDIALOG_H
