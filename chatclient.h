#ifndef CHATCLIENT_H
#define CHATCLIENT_H
#include <QMainWindow>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QFileDialog>
#include <QFile>
#include <QStatusBar>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QDebug>

class ChatClient : public QMainWindow {
    Q_OBJECT
public:
    ChatClient(QWidget *parent = nullptr);

private:
    QTcpSocket *socket;        // Socket for connecting to the server
    QTextEdit *chatWindow;     // Chat messages display
    QLineEdit *inputField;     // Input field for sending messages
    QPushButton *sendButton;   // Button to send messages
    QPushButton *uploadButton;
    QPushButton *downloadButton;
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QString currentFilePath;
    bool isDownloading = false;
    bool isUploading = false;

private slots:
    void sendMessage();
    void receiveMessage();
    void onConnected();
    void onDisconnected();
    void uploadFile();
    void downloadFile();
    void cancelTransfer();
    void onError(QAbstractSocket::SocketError socketError);
};
#endif // CHATCLIENT_H
