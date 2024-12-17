#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QFile>
#include <QProgressBar>
#include <QLabel>
#include <QTableWidget>
#include <QStatusBar>
#include <QMessageBox>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_lineEdit_ip_textChanged(const QString &arg1);

    void on_btn_connect_clicked();

    void sendMsg();
    void receiveMsg();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError sockErr);
//    void uploadFile();
    void on_btn_close_clicked();
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    Ui::MainWindow *ui;
    QTcpSocket *client_sock;
    QTextEdit *chathistory;
    QLineEdit *input;
    QLineEdit *ipedit;
    QLineEdit *useredit;
    QLineEdit *portedit;
    QPushButton *connectBtn;
    QPushButton *sendBtn;
    QPushButton *closeBtn;
    QLabel *acceptFile;
    QLabel *uploadFile;
    QLabel *cancelFile;
    QLabel *filename;
    QProgressBar *progressBar;
    QTableWidget *tableList;
    QString ip;
    QString port;
    QString username;
};
#endif // MAINWINDOW_H
