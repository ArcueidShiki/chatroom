#include "fileworker.h"
#include <QFileInfo>
#include <QTimer>
#include <QDebug>

#define KB 1024
#define MB (1024 * 1024)

FileWorker::FileWorker(QString &ip, quint16 port, const QString &filePath, bool isUpload)
    : ip(ip), port(port), filePath(filePath), isUpload(isUpload)
{
    file = nullptr;
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
    qint64 bytes = 0;
    qint64 total = file.size();
    QString fileMeta = QString("UPLOAD %1 %2").arg(fileInfo.fileName()).arg(file.size());
    qDebug() << fileMeta;
    socket->write(fileMeta.toUtf8());
    timer = new QElapsedTimer();
    timer->start();
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
            if (bytesWritten == -1)
            {
                qDebug() << "Write Error:" << socket->errorString();
                return;
            }
            bytesToWrite -= bytesWritten;
            chunk = chunk.mid(bytesWritten);
            bytesSent += bytesWritten;
            bytes += bytesWritten;
        }

        // ensure data is sent
        socket->waitForBytesWritten();
        if (timer->elapsed() >= 100) // ms
        {
            double elapsedTime = timer->elapsed() / 1000.0;
            QString speed;
            speedStr(bytes, elapsedTime, speed);
            emit speedUpdated(speed);
            int progress = static_cast<int>((bytesSent * 100) / total);
            emit progressUpdated(progress);
            bytes = 0;
            qDebug() << "Progress: " << progress << "DEBUG 5 Speed: " << speed;
            timer->restart();
        }
    }
    file.close();
    delete socket;
    emit transferComplete();
}

void FileWorker::download()
{
//    file = new QFile(filePath);
//    if (!file->open(QIODevice::WriteOnly))
//    {
//        emit transferFailed("Failed to open file for download");
//        return;
//    }
//    // TODO may have issues.
//    qint64 totalBytes = 0;
//    qint64 bytesReceived = 0;
//    timer.start();

//    connect(file_sock, &QTcpSocket::readyRead, [=]()mutable {
//        if (totalBytes == 0)
//        {
//            QByteArray header = file_sock->readLine();
//            totalBytes = header.trimmed().toLongLong();
//            emit progressUpdated(0, bytesReceived, 0.0);
//        }
//        QByteArray data = file_sock->readAll();
//        file->write(data);
//        bytesReceived += data.size();
//        bytesTransferredInInterval += data.size();
//        if (timer.elapsed() >= 1000)
//        {
//            double speed = bytesTransferredInInterval / 1024.0;
//            emit progressUpdated(static_cast<int>((bytesReceived * 100) / totalBytes), bytesReceived, speed);
//            bytesTransferredInInterval = 0;
//            timer.restart();
//        }
//        if (bytesReceived >= totalBytes)
//        {
//            file->close();
//            delete file;
//            file = nullptr;
//            emit transferComplete();
//        }
//    });

//    connect(file_sock, &QTcpSocket::disconnected, [=](){
//        if (file)
//        {
//            file->close();
//            delete file;
//            file = nullptr;
//        }
//        emit transferFailed("File download interrupted.");
    //    });
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
















