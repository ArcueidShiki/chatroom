#ifndef CHATCLIENT_H
#define CHATCLIENT_H
#include <QMainWindow>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QStatusBar>

class ChatClient : public QMainWindow {
    Q_OBJECT

private:
    QTcpSocket *socket;        // Socket for connecting to the server
    QTextEdit *chatWindow;     // Chat messages display
    QLineEdit *inputField;     // Input field for sending messages
    QPushButton *sendButton;   // Button to send messages
    QPushButton *fileButton;   // Button to send files

public:
    ChatClient(QWidget *parent = nullptr) : QMainWindow(parent) {
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
        fileButton = new QPushButton("Send File");

        // Add widgets to layout
        layout->addWidget(chatWindow);
        layout->addWidget(inputField);
        layout->addWidget(sendButton);
        layout->addWidget(fileButton);

        setCentralWidget(centralWidget);

        // Socket initialization
        socket = new QTcpSocket(this);
        connect(socket, &QTcpSocket::readyRead, this, &ChatClient::receiveMessage);
        connect(socket, &QTcpSocket::connected, this, &ChatClient::onConnected);
        connect(socket, &QTcpSocket::disconnected, this, &ChatClient::onDisconnected);
        connect(socket, &QTcpSocket::errorOccurred, this, &ChatClient::onError);

        // Button connections
        connect(sendButton, &QPushButton::clicked, this, &ChatClient::sendMessage);
        connect(fileButton, &QPushButton::clicked, this, &ChatClient::sendFile);

        // Status bar
        statusBar()->showMessage("Not connected to server.");

        // Connect to server
        socket->connectToHost("172.17.158.25", 9999);
    }

private slots:
    void sendMessage() {
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

    void sendFile() {
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

    void receiveMessage() {
        QByteArray data = socket->readAll();
        chatWindow->append(QString::fromUtf8(data));
    }

    void onConnected() {
        statusBar()->showMessage("Connected to server.");
        chatWindow->append("Connected to the server!");
    }

    void onDisconnected() {
        statusBar()->showMessage("Disconnected from server.");
        chatWindow->append("Disconnected from the server.");
    }

    void onError(QAbstractSocket::SocketError socketError) {
        Q_UNUSED(socketError);
        statusBar()->showMessage("Connection error: " + socket->errorString());
        chatWindow->append("Error: " + socket->errorString());
    }
};
#endif // CHATCLIENT_H
