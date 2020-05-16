
#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QtWidgets/QWidget>
#include <QtNetwork/QtNetwork>
#include "ISOCKETSink.h"
#include "SocketUtil.h"
#include <QMessageBox>
#include "mysql.h"
#include "register.h"
#include "onlineUserlist.h"

namespace Ui {
class Login;
}

class Login : public QWidget, public ISOCKETSink
{
    Q_OBJECT
public:
    Login(QWidget *parent = Q_NULLPTR);
    ~Login();
private slots :
    void on_pb_login_clicked();
    void on_pb_register_clicked();
    void displaySocketError(QAbstractSocket::SocketError);
    void readMessages();

private:
    Register *mRegister;
    OnlineUserlist *mOnlineUserlist;
    QTcpSocket *mTcpSocket;
    SocketUtil *mSocketUtil;
    Ui::Login *ui;
    void Success(QString message);
    void Fail(QString message);
    void connectSocket();
    void init();
};
#endif
