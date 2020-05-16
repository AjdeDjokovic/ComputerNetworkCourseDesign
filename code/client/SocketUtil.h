#pragma once
#ifndef SocketController_H
#define SocketController_H
#include <QtNetwork/QtNetwork>
#include <QString>
#include "ISOCKETSink.h"
#include "qdebug.h"
class SocketUtil
{
public:
    SocketUtil(ISOCKETSink* socketSink)
    {
        this->mSock = socketSink;
        sArgIp = "";
        sArgPort = "";
    }
public:
    void connect(QTcpSocket* socket);
    //创建ini文件，保存ip和port，供注册、聊天使用
    void WriteIni(const QString &ip, const QString &port);
    void ReadIni();
    void writeData(QTcpSocket* socket, const char* data);

    static void delayMSec(unsigned int msec);

private:
    ISOCKETSink* mSock;
    QString sArgIp;
    QString sArgPort;
    SocketUtil* mSocket;
};
#endif