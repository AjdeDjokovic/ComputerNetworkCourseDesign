#ifndef MYTCPSOCKETTHREAD_H
#define MYTCPSOCKETTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QtNetwork>
#include <QApplication>
#include "logincontrol.h"
#include "registercontrol.h"
#include "mysql.h"
#include "util.h"

class MyTcpSocketThread : public QThread
{
    Q_OBJECT

public:
    MyTcpSocketThread();
    ~MyTcpSocketThread();
    void writeDescriptor(const QString&);
    virtual void run();
signals:
    void error(QTcpSocket::SocketError socketError);
    void bytesArrived(const qint64&, const qint32&, const qintptr&);
    void sendChatToServer( QTcpSocket*, const QString&, const QString&, const QStringList&);
    void sendFileToServer( QTcpSocket*, const QString&, const QString&, const int&,const QString&);
    void sendChatDisconToServer( QTcpSocket*, const QString&);
    //文件名，文件总长度，已发送文件长度，发送人
    void sendFileProcessToServer( QTcpSocket*, const QString&, const qint64&, const qint64&, const QString&);

    public slots:
    void receiveFile();
    void receiveChat();
    void disconChat();
    void disconFile();
public:
    QTcpSocket *tcpSocket;                //每个连接的socket
private:
    QString    m_qstrSender;              //发送者
    QString    m_qstrReceiver;            //接受者
    QString    m_qstrSockedID;            //socket句柄
    qintptr    m_qiSocketID;              //socket句柄
    //以下为新文件上传所需变量
    quint16    m_qiBlockSize ;            //存放接收到的信息大小
    QFile     *m_pLocalFile;              //要发送的文件
    qint64     m_qiTotalBytes ;           //数据总大小
    QString    m_qstrFileName;            //保存文件路径
    qint64     m_qiBytesReceived ;        //已收到数据的大小
    qint64     m_qiFileNameSize ;         //文件名的大小信息
    QByteArray m_qByte_InBlock;           //数据缓冲区
    //以下为断点续传所需变量
    qint64     m_qiFileTotalLength;       //断点续传时客户端发送的文件总长度
    qint64     m_qiContinueLength;		  //已经传输的文件大小，供断点续传直接从以前的百分比计算，
                                          //等于总大小减去已经剩余文件大小，剩余大小定义在receiveFile方法内部
    int		   m_iCurrentStatus ;	      // 0 代表断点续传，只是为了标识断点续传，其余情况为纯文件一次性上传
    qint64     progressEmitCount;          //控制不是每次都更新进度条，而是接收一定次数更新一次
};

#endif // MYTCPSOCKETTHREAD_H