#include "fileworker.h"
#include <QFileInfo>
#include <QTimer>
#include <QDebug>

FileWorker::FileWorker(QString &ip, QString &port, const QString &filePath, bool isUpload)
    : ip(ip), port(port), filePath(filePath), isUpload(isUpload), file(nullptr), bytesTransferredInInterval(0)
{

}

FileWorker::~FileWorker()
{
    if (file)
    {
        file->close();
        delete file;
    }
}

void FileWorker::startTransfer()
{
    if (isUpload)
    {
        uploadFile();
    } else
    {
        downloadFile();
    }
}

void FileWorker::cancelTransfer()
{
    if (file)
    {
        file->close();
        delete file;
        file = nullptr;
    }
//    socket->abort();
    emit transferFailed("Transfer canceled");
}

void FileWorker::uploadFile()
{
    file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly))
    {
        emit transferFailed("Failed to open file for upload");
        return;
    }

    QFileInfo fileInfo(filePath);
    qint64 totalBytes = file->size();
    QString header = QString("UPLOAD %1 %2").arg(fileInfo.fileName()).arg(totalBytes);
    file_sock->write(header.toUtf8());
    qint64 bytesSent = 0;
    timer.start();
    while (!file->atEnd() && file_sock->state() == QAbstractSocket::ConnectedState)
    {
        QByteArray chunk = file->read(4096);
        file_sock->write(chunk);
        bytesSent += chunk.size();
        bytesTransferredInInterval += chunk.size();

        if (timer.elapsed() >= 1000) // ms
        {
            double speed = bytesTransferredInInterval / 1024.0; // kb/s
            emit progressUpdated(static_cast<int>((bytesSent * 100) / totalBytes), bytesSent, speed);
            bytesTransferredInInterval = 0;
            timer.restart();
        }
    }
    file->close();
    delete file;
    file = nullptr;
    if (bytesSent == totalBytes)
    {
        emit transferComplete();
    }
    else
    {
        emit transferFailed("File upload interrupted,");
    }
}

void FileWorker::downloadFile()
{
    file = new QFile(filePath);
    if (!file->open(QIODevice::WriteOnly))
    {
        emit transferFailed("Failed to open file for download");
        return;
    }
    // TODO may have issues.
    qint64 totalBytes = 0;
    qint64 bytesReceived = 0;
    timer.start();

    connect(file_sock, &QTcpSocket::readyRead, [=]()mutable {
        if (totalBytes == 0)
        {
            QByteArray header = file_sock->readLine();
            totalBytes = header.trimmed().toLongLong();
            emit progressUpdated(0, bytesReceived, 0.0);
        }
        QByteArray data = file_sock->readAll();
        file->write(data);
        bytesReceived += data.size();
        bytesTransferredInInterval += data.size();
        if (timer.elapsed() >= 1000)
        {
            double speed = bytesTransferredInInterval / 1024.0;
            emit progressUpdated(static_cast<int>((bytesReceived * 100) / totalBytes), bytesReceived, speed);
            bytesTransferredInInterval = 0;
            timer.restart();
        }
        if (bytesReceived >= totalBytes)
        {
            file->close();
            delete file;
            file = nullptr;
            emit transferComplete();
        }
    });

    connect(file_sock, &QTcpSocket::disconnected, [=](){
        if (file)
        {
            file->close();
            delete file;
            file = nullptr;
        }
        emit transferFailed("File download interrupted.");
    });
}
















