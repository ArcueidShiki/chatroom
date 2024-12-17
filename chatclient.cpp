#include "chatclient.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

ChatClient::ChatClient(QWidget *parent) : QMainWindow(parent)
{
    // Initialize the main window
    setWindowTitle("Chat Client");
    resize(600, 400);

    // Central widget and layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Chat window
    chatWindow = new QTextEdit();
    chatWindow->setReadOnly(true);

    // Input field and buttons
    inputField = new QLineEdit();
    sendButton = new QPushButton("Send");
    uploadButton = new QPushButton("Upload File");
    downloadButton = new QPushButton("Download File");
    progressBar = new QProgressBar();
    statusLabel = new QLabel("status: Not connected");

    // Add widgets to layout
    layout->addWidget(chatWindow);
    layout->addWidget(inputField);
    layout->addWidget(sendButton);
    layout->addWidget(uploadButton);
    layout->addWidget(downloadButton);
    layout->addWidget(progressBar);
    layout->addWidget(statusLabel);

    setCentralWidget(centralWidget);
    centralWidget->setLayout(layout);
    // Socket initialization
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &ChatClient::receiveMessage);
    connect(socket, &QTcpSocket::connected, this, &ChatClient::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &ChatClient::onDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &ChatClient::onError);

    // Button connections
    connect(sendButton, &QPushButton::clicked, this, &ChatClient::sendMessage);
    connect(uploadButton, &QPushButton::clicked, this, &ChatClient::uploadFile);
    connect(downloadButton, &QPushButton::clicked, this, &ChatClient::downloadFile);

    // Status bar
    statusBar()->showMessage("Not connected to server.");
    // Connect to server
    socket->connectToHost("172.17.158.25", 9999);
}

void ChatClient::sendMessage()
{
    if (!socket->isOpen()) {
        chatWindow->append("Error: Not connected to server.");
        return;
    }

    QString message = inputField->text().trimmed();
    if (!message.isEmpty()) {
        socket->write(message.toUtf8());
        inputField->clear();
    }
}

#if 0
void ChatClient::sendFile()
{
    if (!socket->isOpen()) {
        chatWindow->append("Error: Not connected to server.");
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "Select File to Send");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileData = file.readAll();
        QString header = QString("FILE:%1:%2\n")
                             .arg(QFileInfo(fileName).fileName())
                             .arg(fileData.size());
        socket->write(header.toUtf8());
        socket->write(fileData);
        file.close();
        chatWindow->append("File sent: " + QFileInfo(fileName).fileName());
    } else {
        chatWindow->append("Error: Failed to open the file.");
    }
}
#endif

void ChatClient::receiveMessage()
{
    QByteArray data = socket->readAll();
    if (isDownloading)
    {
        progressBar->setValue(data.size());
        if (data.size() == 100)
        {
            statusLabel->setText("Download Complete");
            isDownloading = false;
        }
    } else if (isUploading)
    {
        progressBar->setValue(data.size());
    } else
    {
        chatWindow->append(QString::fromUtf8(data));
    }
}

void ChatClient::onConnected()
{
    statusBar()->showMessage("Connected to server.");
    statusLabel->clear();
    statusLabel->setText("Connected to Server.");
    chatWindow->append("Connected to the server!");
}

void ChatClient::onDisconnected()
{
    statusBar()->showMessage("Disconnected from server.");
    chatWindow->append("Disconnected from the server.");
}

void ChatClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    statusBar()->showMessage("Connection error: " + socket->errorString());
    chatWindow->append("Error: " + socket->errorString());
}

void ChatClient::uploadFile()
{
    if (isUploading) return;
    QString filename = QFileDialog::getOpenFileName(this, "Select File");
    if (filename.isEmpty()) return;
    QFile file(filename);
    qDebug() << u8"草泥马0";
    if (!file.open(QIODevice::ReadOnly)) return;

    isUploading = true;
    currentFilePath = filename;
    QByteArray fileData = file.readAll();
    qDebug() << u8"草泥马1";
    socket->write("UPLOAD " + filename.toUtf8() + "\n");
    qDebug() << u8"草泥马2";
    // this conversion has error.
    char size[64];
    // 这里出错
    snprintf(size, sizeof(size), "%lld", fileData.size());
    qDebug() << u8"草泥马3";
    socket->write(size, sizeof(size));
    qDebug() << u8"草泥马4";
    socket->write(fileData);
    qDebug() << u8"草泥马5";
}

void ChatClient::downloadFile()
{
    if (isDownloading) return;
    QString filename = QFileDialog::getSaveFileName(this, "Save File");
    if (filename.isEmpty()) return;
    socket->write("DOWNLOAD " + filename.toUtf8());
    statusLabel->setText("Downloading: " + filename);
    progressBar->setRange(0, 100);
    isDownloading = true;
}

void ChatClient::cancelTransfer()
{
    if (isDownloading)
    {
        socket->abort();
        statusLabel->setText("Download Canceled");
        isDownloading = true;
    }

    if (isUploading)
    {
        socket->abort();
        statusLabel->setText("Upload Canceled");
        isUploading = false;
    }
}

