#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    chathistory = ui->textEdit_chat_history;
    input = ui->lineEdit_inputbox;
    ipedit = ui->lineEdit_ip;
    portedit = ui->lineEdit_port;
    useredit = ui->lineEdit_user;
    connectBtn = ui->btn_connect;
    sendBtn = ui->btn_send;
    closeBtn = ui->btn_close;
    acceptFile = ui->label_accept;
    uploadFile = ui->label_upload;
    cancelFile = ui->label_cancel;
    filename = ui->label_filename;
    progressBar = ui->progressBar;
    tableList = ui->table_userlist;
//    connect(upload, &QPushButton::clicked, this, &MainWindow::sendFile);
    statusBar()->showMessage("Not connected to server");
    // dispatch recv and send thread.
    sendBtn->setDisabled(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_lineEdit_ip_textChanged(const QString &arg1)
{
    qDebug() << "Test";
}


void MainWindow::on_btn_connect_clicked()
{
    client_sock = new QTcpSocket(this);
    connect(client_sock, &QTcpSocket::readyRead, this, &MainWindow::receiveMsg);
    connect(client_sock, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(client_sock, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(client_sock, &QTcpSocket::errorOccurred, this, &MainWindow::onError);

    // Button connections
    connect(sendBtn, &QPushButton::clicked, this, &MainWindow::sendMsg);
    ip = ipedit->text();
    port = portedit->text();
    username = useredit->text();
    client_sock->connectToHost(ip, atol(port.toStdString().c_str()));
    qDebug() << "Connected Success!\n";
    sendBtn->setDisabled(false);
}

void MainWindow::sendMsg()
{
    if (!client_sock->isOpen())
    {
        chathistory->append("Error: Not connected to server");
        return;
    }

    QString message = u8"[" + username + "]: " + input->text().trimmed();
    if (!message.isEmpty())
    {
        client_sock->write(message.toUtf8());
        input->clear();
    }
}

void MainWindow::receiveMsg()
{
    QByteArray data = client_sock->readAll();
    chathistory->append(QString::fromUtf8(data));
}

void MainWindow::onConnected()
{
    statusBar()->showMessage("连接成功");
    // TODO send greeting msg
    QString greeting = username + u8" 进入了群聊";
    client_sock->write(greeting.toUtf8());
}

void MainWindow::onDisconnected()
{
    statusBar()->showMessage("断开链接");
    chathistory->append("Disconnected from the server.");
}

void MainWindow::onError(QAbstractSocket::SocketError sockErr)
{
    Q_UNUSED(sockErr);
    statusBar()->showMessage("Connection error: " + client_sock->errorString());
    chathistory->append("Error: " + client_sock->errorString());
}

void MainWindow::on_btn_close_clicked()
{
    this->close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QString wave = username + u8" 退出了群聊";
    if (client_sock->isOpen()) client_sock->write(wave.toUtf8());
}
