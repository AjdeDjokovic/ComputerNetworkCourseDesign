#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mytcpserver.h"
#include <QtNetwork>
#include "mysql.h"
#include <QInputDialog>
#include <iostream>
#include <QMessageBox>
#include <QThread>
#include <QCloseEvent>
#include "util.h"
#include "logincontrol.h"
#include "registercontrol.h"
#include <QListWidget>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void pushUserList();
    void getLocalIP();
public slots:
    void startChat();            //打开聊天服务
    void closeChat();            //关闭聊天服务
    void startFile();            //打开文件服务
    void closeFile();            //关闭文件服务
    void receiveChatFromServer(QTcpSocket*, const QString&, const QString&, const QStringList&);             //接受来自服务的聊天消息
    void receiveFileFromServer(QTcpSocket*, const QString&, const QString&, const int&, const QString&);     //接受来自服务的文件通知消息
    void receivedisconChatFromServer(QTcpSocket*, const QString&);                                           //接受来自服务的聊天断开事件
    void receiveFileProcessFromServer(QTcpSocket*,const QString&,const qint64&,const qint64&,const QString&);//接受来自服务的文件进度通知
protected:
    void closeEvent(QCloseEvent *event);
private:
    QString localIP;
    Ui::MainWindow *ui;
    MyTcpServer *chatServer;     //聊天服务（QTServer）
    MyTcpServer *fileServer;     //文件服务（QTServer）
};

#endif // MAINWINDOW_H