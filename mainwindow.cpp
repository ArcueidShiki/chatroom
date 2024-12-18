#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

// TODO do file upload in another thread, otherwise, I can't move.

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
    acceptFile = ui->btn_accept;
    uploadFile = ui->btn_upload;
    cancelFile = ui->btn_cancel;
    filename = ui->label_filename;
    speedLb = ui->label_speed;
    progressBar = ui->progressBar;
    tableList = ui->table_userlist;

    statusBar()->showMessage("Not connected to server");
    sendBtn->setDisabled(true);
    progressBar->setValue(50);

    connect(uploadFile, &QPushButton::clicked, this, &MainWindow::upload);
    connect(acceptFile, &QPushButton::clicked, this, &MainWindow::download);
//    connect(cancelFile, &QPushButton::clicked, this, [=](){
//        if (fileWorker) fileWorker->cancelTransfer();
//    });
}

MainWindow::~MainWindow()
{
//    if (workerThread)
//    {
//        workerThread->quit();
//        workerThread->wait();
//        delete fileWorker;
//    }
    if (client_sock) client_sock->disconnectFromHost();
    if (file_sock) file_sock->disconnectFromHost();
    delete ui;
}

void MainWindow::on_btn_connect_clicked()
{
    client_sock = new QTcpSocket(this);
    ip = ipedit->text();
    port = portedit->text().toUInt();
    username = useredit->text();
    connect(client_sock, &QTcpSocket::readyRead, this, &MainWindow::receiveMsg);
    connect(client_sock, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(client_sock, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(client_sock, &QTcpSocket::errorOccurred, this, &MainWindow::onError);
    client_sock->connectToHost(ip, port);
    if (!client_sock->waitForConnected(3000))
    {
        QMessageBox::critical(this, "Error", "Failed to connect to server");
    }
    else
    {
        connect(sendBtn, &QPushButton::clicked, this, &MainWindow::sendMsg);
        QMessageBox::information(this, "Connected", "Connected to server Success");
//        getUserList();
    }
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

void MainWindow::upload()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select File to upload");
    if (filePath.isEmpty()) return;
    if (!file_sock)
    {
        QMessageBox::critical(this, "Error", "File socket connection Error");
        return;
    }
//    connect(file_sock, &QTcpSocket::bytesWritten, this, &MainWindow::updateProgressBar);
//    workerThread = new QThread(this);
//    fileWorker = new FileWorker(ip, port, filePath, true);
//    // QSocketNotifier: Socket notifiers cannot be enabled or disabled from another thread
//    // Keep Socket in Main thread.
//    fileWorker->moveToThread(workerThread);

//    connect(workerThread, &QThread::started, fileWorker, &FileWorker::startTransfer);
//    connect(fileWorker, &FileWorker::progressUpdated, this, &MainWindow::updateProgressBar);
//    connect(fileWorker, &FileWorker::transferComplete, this, &MainWindow::onTransferComplete);
//    connect(fileWorker, &FileWorker::transferFailed, this, &MainWindow::onTransferFailed);

//    workerThread->start();
    cancelFile->setEnabled(true);
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Failed to open file.");
        return;
    }

    file_sock = new QTcpSocket(this);
    file_sock->connectToHost(ip, port);
    if (!file_sock->waitForConnected(3000))
    {
        QMessageBox::critical(this, "Error", "Failed to connect to File server");
    }
    if (file_sock->state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "DEBUG 1";
        delete file_sock;
        return;
    }
    QFileInfo fileInfo(filePath);
    qint64 bytesSent = 0;
    qint64 bytesTransferredInInterval = 0;
    qint64 total = file.size();
    QString fileMeta = QString("UPLOAD %1 %2").arg(fileInfo.fileName()).arg(file.size());
    qDebug() << fileMeta;
    filename->setText(fileInfo.fileName());
    file_sock->write(fileMeta.toUtf8());
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    cancelFile->setEnabled(true);
    timer = new QElapsedTimer();
    timer->start();
    while (!file.atEnd() && file_sock->state() == QAbstractSocket::ConnectedState)
    {
        QByteArray chunk = file.read(4096);
        if (chunk.isEmpty())
        {
            file.close();
            cancelFile->setEnabled(false);
            QMessageBox::information(this, "Upload", "File Upload Complete");
            return;
        }
        file_sock->write(chunk);
        bytesSent += chunk.size();
        bytesTransferredInInterval += chunk.size();
        if (timer->elapsed() >= 100) // ms
        {
            double speed = bytesTransferredInInterval * 10 / 1024.0; // kb/s
            speedLb->setText(QString("Speed: %1 KB/s").arg(speed, 0, 'f', 2));
            int progress = static_cast<int>((bytesSent * 100) / total);
            progressBar->setValue(progress);
            bytesTransferredInInterval = 0;
            QCoreApplication::processEvents();
            // speed and progress, should deeping on server side
            qDebug() << "Progress: " << progress << "DEBUG 5 Speed: " << speedLb->text();
            timer->restart();
        }
    }
    // wait for server complete msg;
    file_sock->waitForReadyRead(INFINITY);
    file.close();
    QMessageBox::information(this, "Upload Complete", "File upload completed successfully.");
    cancelFile->setEnabled(false);
    delete file_sock;
}

void MainWindow::cancelTransfer()
{
    if (file)
    {
        file->close();
        file->deleteLater();
        file = nullptr;
    }
    progressBar->setValue(0);
    cancelFile->setEnabled(false);
    QMessageBox::information(this, "Transfer", "Transfer canceled");
}

void MainWindow::download()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Downloaded File");
    if (filePath.isEmpty()) return;
    QString fileName = QFileInfo(filePath).fileName();
    QString header = QString("DOWNLOAD %1\n").arg(fileName);

//    workerThread = new QThread(this);
//    // there have problems with memory leak, upload and download clicked at same time.
//    fileWorker = new FileWorker(ip, port, filePath, false);
//    connect(workerThread, &QThread::started, fileWorker, &FileWorker::startTransfer);

//    connect(fileWorker, &FileWorker::transferComplete, this, &MainWindow::onTransferComplete);
//    connect(fileWorker, &FileWorker::transferFailed, this, &MainWindow::onTransferFailed);
//    workerThread->start();
//    cancelFile->setEnabled(true);
}

void MainWindow::onTransferComplete()
{
    QMessageBox::information(this, "Transfer Complete", "File transfer completed successful.");
    workerThread->quit();
    cancelFile->setEnabled(false);
}

void MainWindow::onTransferFailed(const QString &errorMsg)
{
    QMessageBox::critical(this, "Transfer Failed", errorMsg);
    workerThread->quit();
    cancelFile->setEnabled(false);
}

void MainWindow::getUserList()
{
    if (client_sock->state() == QAbstractSocket::ConnectedState)
    {
        client_sock->write("USERLIST\n");
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QString wave = username + u8" 退出了群聊";
    if (client_sock->isOpen()) client_sock->write(wave.toUtf8());
}

void MainWindow::on_lineEdit_ip_textChanged(const QString &arg1)
{

}

