#ifndef FILETRANSFERSERVER_H
#define FILETRANSFERSERVER_H

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QAtomicInt>
#include <QString>
#include <QStringList>

namespace httplib {
class Server;
struct Request;
struct Response;
}

class FileTransferServer : public QObject
{
    Q_OBJECT
public:
    explicit FileTransferServer(QObject *parent = nullptr);
    ~FileTransferServer() override;

    static QString getStrictGlobalIPv6();

    bool start(quint16 port, const QStringList &files, const QString &token);
    void stop();
    bool isRunning() const;

    /// 服务器运行期间动态添加文件（线程安全）
    void addFiles(const QStringList &files);

signals:
    void serverStarted(quint16 actualPort);
    void serverStopped();
    void fileDownloaded(const QString &fileName);
    void errorOccurred(const QString &msg);

private:
    void setupRoutes();
    bool isTokenValid(const httplib::Request &req);
    QString resolveFilePath(const std::string &filename) const;

    QThread m_workerThread;
    httplib::Server *m_svr = nullptr;
    QAtomicInt m_running;
    QString m_token;
    QStringList m_sharedFiles;
    mutable QMutex m_filesMutex;
    quint16 m_port = 0;
};

#endif // FILETRANSFERSERVER_H
