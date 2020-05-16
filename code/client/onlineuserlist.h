#ifndef ONLINEUSERLIST_H
#define ONLINEUSERLIST_H

#include <QtWidgets/QWidget>
#include <QtNetwork/QtNetwork>
#include <QDialog>
#include "ISOCKETSink.h"
#include "SocketUtil.h"
#include <QLineEdit>
#include "chat.h"
#include <list>
#include <string>
#include "historymessage.h"
#include "ui_onlineuserlist.h"
using namespace std;

namespace Ui {
class OnlineUserlist;
}

class OnlineUserlist : public QDialog, public ISOCKETSink
{
    Q_OBJECT

public:
    OnlineUserlist(QDialog *parent = Q_NULLPTR, QTcpSocket *pTcpSocket = 0, SocketUtil *mSocketUtil = 0);
    ~OnlineUserlist();
    void popWindow(QString str);
    void Success(QString message);
    void Fail(QString message);
    void init();
    void connectSocket();

private:
    Ui::OnlineUserlist ui;
    QLineEdit *m_pSearchLineEdit;
    QTcpSocket *mTcpSocket;
    SocketUtil *mSocketUtil;
    QStringListModel *mListModel;
    QStringList user;
    Chat *mchat;
    QMap<QString, Chat*> *chatMap;
    bool bFlag;//标志窗口是否在线打开
    QString value;//通过用户名标志子窗口id
    HistoryMessage *view;

private slots :
    void readMessages();
    void newChatWindow();
    void getUserlist();//获得在线用户列表
    void displaySocketError(QAbstractSocket::SocketError);
    void chatDestroyed(QString values);
    void search();
};
#endif


