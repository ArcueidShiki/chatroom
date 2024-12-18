#include "fileworker.h"
#include <QFileInfo>
#include <QTimer>
#include <QDebug>

#define KB 1024
#define MB (1024 * 1024)

FileWorker::FileWorker(QString &ip, quint16 port, const QString &filePath, bool isUpload, size_t filesize)
    : ip(ip), port(port), filePath(filePath), isUpload(isUpload), filesize(filesize)
{
    file = nullptr;
}

FileWorker::~FileWorker()
{
    if (file)
    {
        file->close();
        delete file;
        file = nullptr;
    }
    if (socket)
    {
        socket->disconnectFromHost();
        delete socket;
        socket = nullptr;
    }
}

void FileWorker::startTransfer()
{
    if (isUpload)
    {
        upload();
    } else
    {
        download();
    }
}

void FileWorker::pauseTransfer()
{
    pause = true;
}

void FileWorker::resumeTransfer()
{
    pause = false;
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

void FileWorker::upload()
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Failed to open file!";
        return;
    }

    socket = new QTcpSocket(this);
    if (!socket)
    {
        return;
    }
    socket->connectToHost(ip, port);
    if (!socket->waitForConnected(3000) ||
        socket->state() != QAbstractSocket::ConnectedState)
    {
        delete socket;
        return;
    }
    QFileInfo fileInfo(filePath);

    qint64 bytesSent = 0;
    qint64 total = file.size();
    QString fileMeta = QString("UPLOAD %1 %2").arg(fileInfo.fileName()).arg(file.size());
    qDebug() << fileMeta;
    socket->write(fileMeta.toUtf8());
//    QByteArray data = file.readAll();
    timer = new QElapsedTimer();
    timer->start();
//    while (bytesSent < file.size())
//    {
//        qint64 bytesWritten = socket->write(data.mid(bytesSent));
//        if (bytesWritten == -1)
//        {
//            qDebug() << "Write error:" << socket->errorString();
//            return;  // Error
//        }
//        bytesSent += bytesWritten;
//        if (!socket->waitForBytesWritten(100000))
//        {
//            return;
//        }
//    }
    while (!file.atEnd() && socket->state() == QAbstractSocket::ConnectedState)
    {
        QByteArray chunk = file.read(4096);
        if (chunk.isEmpty())
        {
            file.close();
            emit transferFailed("Read Chunk is empty");
            return;
        }
        qint64 bytesToWrite = chunk.size();
        while (bytesToWrite > 0)
        {
            qint64 bytesWritten = socket->write(chunk);
            if (!socket->waitForBytesWritten(10000))
            {
                emit transferFailed("Wait For bytes Written Timeout");
                return;
            }
            if (bytesWritten == -1)
            {
                emit transferFailed("Write Error:" + socket->errorString());
                return;
            }
            bytesToWrite -= bytesWritten;
            chunk = chunk.mid(bytesWritten);
            bytesSent += bytesWritten;
        }

        double seconds = timer->elapsed() / 1000.0;
        QString speed;
        speedStr(bytesSent, seconds, speed);
        emit speedUpdated(speed);
        int progress = static_cast<int>((bytesSent * 100) / total);
        emit progressUpdated(progress);
//        qDebug() << "Progress: " << progress << "DEBUG 5 Speed: " << speed;
    }
    socket->close();
    file.close();
    emit transferComplete();
}

void FileWorker::download()
{
    socket = new QTcpSocket(this);
    if (!socket)
    {
        return;
    }
    socket->connectToHost(ip, port);
    if (!socket->waitForConnected(3000) ||
        socket->state() != QAbstractSocket::ConnectedState)
    {
        delete socket;
        return;
    }
    QString header = QString("DOWNLOAD %1").arg(filePath);
    qDebug() << header;
    file = new QFile(filePath);
    if (!file->open(QIODevice::WriteOnly))
    {
        qDebug() << "DEBUGXXX";
        emit transferFailed("Failed to open file for download");
        return;
    }

    qint64 bytesReceived = 0;
    socket->write(header.toUtf8());
    qDebug() << header;
    timer = new QElapsedTimer();
    timer->start();
    while(bytesReceived < filesize && socket->state() == QAbstractSocket::ConnectedState)
    {

        QByteArray chunk = socket->read(1024 * 4096 * 200);
        socket->waitForReadyRead(100000);
        file->write(chunk);
        bytesReceived += chunk.size();
        double seconds = timer->elapsed() / 1000;
        QString speed;
        speedStr(bytesReceived, seconds, speed);
        int progress = static_cast<int>(bytesReceived * 100 / filesize);
        emit speedUpdated(speed);
        emit progressUpdated(progress);
    }
    socket->close();
    file->close();
    emit transferComplete();
}

void FileWorker::speedStr(qint64 bytes, double time, QString &str)
{
    if (bytes < KB)
    {
        str = QString("Speed: %1 B/s").arg(bytes / time, 0, 'f', 2);
    }
    else if (bytes < MB)
    {
        str = QString("Speed: %1 KB/s").arg(bytes / time / KB, 0, 'f', 2);
    }
    else
    {
        str = QString("Speed: %1 MB/s").arg(bytes / time / MB, 0, 'f', 2);
    }
}
















