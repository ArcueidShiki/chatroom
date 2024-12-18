#ifndef FILEWORKER_H
#define FILEWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QElapsedTimer>

class FileWorker : public QObject
{
    Q_OBJECT
public:
    explicit FileWorker(QString &ip, QString &port, const QString &filePath, bool isUpload);
    ~FileWorker();
signals:
    void progressUpdated(int percentage, qint64 bytesTransferred, double speed);
    void transferComplete();
    void transferFailed(const QString &errorMsg);
public slots:
    void startTransfer();
    void cancelTransfer();
private:
    void uploadFile();
    void downloadFile();
    QTcpSocket *file_sock;
    QString ip;
    QString port;
    QString filePath;
    bool isUpload;
    QFile *file;
    QElapsedTimer timer;
    qint64 bytesTransferredInInterval;
};

#endif // FILEWORKER_H
