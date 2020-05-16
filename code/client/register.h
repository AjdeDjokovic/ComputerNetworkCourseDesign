#ifndef REGISTER_H
#define REGISTER_H

#include <QtWidgets/QWidget>
#include <QtNetwork/QtNetwork>
#include <QDialog>
#include "ui_register.h"
#include "ISOCKETSink.h"
#include "SocketUtil.h"
#include <QLineEdit>

namespace Ui {
class Register;
}

class Register : public QDialog, public ISOCKETSink
{
    Q_OBJECT

public:
    Register(QDialog *parent = Q_NULLPTR, QTcpSocket *pTcpSocket = 0, SocketUtil *mSocketUtil=0);
    ~Register();

private:
    Ui::Register ui;
    QTcpSocket *mTcpSocket;
    SocketUtil *mSocketUtil;
    void Success(QString message);
    void Fail(QString message);
    void init();
    void connectSocket();

private slots :
    void displaySocketError(QAbstractSocket::SocketError);
    void readMessages();
    void onRegisterClicked();
};
#endif // REGISTER_H
