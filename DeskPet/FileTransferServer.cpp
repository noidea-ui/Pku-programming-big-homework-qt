#include "FileTransferServer.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QNetworkInterface>

#include <fstream>
#include <memory>
#include <vector>

#include "httplib.h"

FileTransferServer::FileTransferServer(QObject *parent)
    : QObject(nullptr) // 不能设 parent，否则 moveToThread 会失败
{
    Q_UNUSED(parent);

    connect(&m_workerThread, &QThread::started, this, [this]() {
        m_svr = new httplib::Server();
        m_svr->set_keep_alive_max_count(1);

        setupRoutes();

        // 先绑定端口（非阻塞），确认可用后再进入阻塞监听
        if (!m_svr->bind_to_port("::", m_port)) {
            emit errorOccurred(
                QString("端口 %1 绑定失败，可能被占用或IPv6未启用").arg(m_port));
            return;
        }

        // 绑定成功，立即标记运行状态，外部才能感知到服务器已启动
        m_running.storeRelaxed(1);
        emit serverStarted(m_port);

        // 进入 accept 循环（阻塞直到 m_svr->stop() 被调用）
        m_svr->listen_after_bind();
    });

    connect(&m_workerThread, &QThread::finished, this, [this]() {
        if (m_svr) {
            delete m_svr;
            m_svr = nullptr;
        }
        m_running.storeRelaxed(0);
        emit serverStopped();
    });
}

FileTransferServer::~FileTransferServer()
{
    stop();
}

QString FileTransferServer::getStrictGlobalIPv6()
{
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp)
            || (iface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        const QString name = iface.humanReadableName().toLower();
        if (name.contains("virtual") || name.contains("vmnet")
            || name.contains("vbox") || name.contains("docker")
            || name.contains("bluetooth") || name.contains("tap")
            || name.contains("ppp"))
            continue;

        for (const auto &entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() != QAbstractSocket::IPv6Protocol)
                continue;
            if (ip.isLoopback() || ip.isLinkLocal()
                || ip.isMulticast() || ip.isUniqueLocalUnicast())
                continue;
            if (ip.scopeId().isEmpty())
                return ip.toString();
        }
    }
    return {};
}

bool FileTransferServer::start(quint16 port, const QStringList &files, const QString &token)
{
    if (m_workerThread.isRunning()) {
        return false;
    }

    m_port = port;
    m_sharedFiles = files;
    m_token = token;

    moveToThread(&m_workerThread);
    m_workerThread.start();
    return true;
}

void FileTransferServer::stop()
{
    if (!m_workerThread.isRunning()) {
        return;
    }

    if (m_svr) {
        m_svr->stop();
    }

    m_workerThread.quit();
    m_workerThread.wait(3000);
}

bool FileTransferServer::isRunning() const
{
    return m_running.loadRelaxed() != 0;
}

// ── Route setup ────────────────────────────────────────────

void FileTransferServer::setupRoutes()
{
    using namespace httplib;

    // 1. Home page — serve HTML template with token injected
    m_svr->Get("/", [this](const Request &req, Response &res) {
        if (!isTokenValid(req)) {
            res.status = 403;
            res.set_content("Forbidden: Invalid Token", "text/plain");
            return;
        }

        QFile file(":/html/file_browser.html");
        if (file.open(QIODevice::ReadOnly)) {
            QString html = QString::fromUtf8(file.readAll());
            html.replace("{{TOKEN}}", m_token);
            res.set_content(html.toStdString(), "text/html; charset=utf-8");
        } else {
            res.status = 500;
            res.set_content("Internal Error: template missing", "text/plain");
        }
    });

    // 2. File list API — 仅列出已上传到宠物的文件
    m_svr->Get("/list", [this](const Request &req, Response &res) {
        if (!isTokenValid(req)) {
            res.status = 403;
            return;
        }

        QMutexLocker locker(&m_filesMutex);

        QJsonArray filesArray;
        for (const QString &filePath : m_sharedFiles) {
            const QFileInfo info(filePath);
            if (!info.exists())
                continue;

            QJsonObject obj;
            obj["name"] = info.fileName();

            const qint64 bytes = info.size();
            if (bytes > 1024 * 1024) {
                obj["size"] = QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
            } else if (bytes > 1024) {
                obj["size"] = QString::number(bytes / 1024.0, 'f', 1) + " KB";
            } else {
                obj["size"] = QString::number(bytes) + " B";
            }

            obj["mtime"] = info.lastModified().toString("yyyy-MM-dd hh:mm");
            filesArray.append(obj);
        }

        QJsonDocument doc;
        doc.setArray(filesArray);
        res.set_content(doc.toJson(QJsonDocument::Compact).toStdString(),
                        "application/json; charset=utf-8");
    });

    // 3. File download — streaming, 只允许下载拖拽列表中的文件
    m_svr->Get("/download", [this](const Request &req, Response &res) {
        if (!isTokenValid(req)) {
            res.status = 403;
            return;
        }

        auto filename = req.get_param_value("file");
        if (filename.empty()) {
            res.status = 400;
            res.set_content("Missing file param", "text/plain");
            return;
        }

        const QString safePath = resolveFilePath(filename);
        if (safePath.isEmpty() || !QFile::exists(safePath)) {
            res.status = 404;
            res.set_content("File not found", "text/plain");
            return;
        }

        const std::string stdPath = safePath.toStdString();
        auto filePtr = std::make_shared<std::ifstream>(stdPath, std::ios::binary | std::ios::ate);
        if (!filePtr->is_open()) {
            res.status = 500;
            res.set_content("Cannot open file", "text/plain");
            return;
        }

        const auto fileSize = filePtr->tellg();
        filePtr->seekg(0, std::ios::beg);

        res.set_header("Content-Disposition",
                       ("attachment; filename=\"" + filename + "\"").c_str());
        res.set_header("Content-Length", std::to_string(fileSize).c_str());
        res.set_header("Content-Type", "application/octet-stream");

        const QString fileNameCopy = QString::fromStdString(filename);

        res.set_content_provider(
            fileSize,
            "application/octet-stream",
            [filePtr](size_t /*offset*/, size_t length,
                                     httplib::DataSink &sink) -> bool {
                constexpr size_t kBufferSize = 1024 * 64;
                std::vector<char> buffer(kBufferSize);
                while (length > 0) {
                    const size_t readSize = std::min(length, kBufferSize);
                    if (!filePtr->read(buffer.data(), static_cast<std::streamsize>(readSize))) {
                        return false;
                    }
                    if (!sink.write(buffer.data(), readSize)) {
                        return false;
                    }
                    length -= readSize;
                }
                return true;
            },
            [this, fileNameCopy](bool success) {
                if (success) {
                    QMetaObject::invokeMethod(this, [this, fileNameCopy]() {
                        emit fileDownloaded(fileNameCopy);
                    }, Qt::QueuedConnection);
                }
            });
    });
}

// ── Helpers ────────────────────────────────────────────────

bool FileTransferServer::isTokenValid(const httplib::Request &req)
{
    const auto token = req.get_param_value("token");
    return !token.empty() && token == m_token.toStdString();
}

void FileTransferServer::addFiles(const QStringList &files)
{
    QMutexLocker locker(&m_filesMutex);
    for (const QString &path : files) {
        if (!m_sharedFiles.contains(path))
            m_sharedFiles.append(path);
    }
}

QString FileTransferServer::resolveFilePath(const std::string &filename) const
{
    QMutexLocker locker(&m_filesMutex);
    const QString name = QString::fromStdString(filename);
    for (const QString &path : m_sharedFiles) {
        if (QFileInfo(path).fileName() == name)
            return path;
    }
    return {};
}
