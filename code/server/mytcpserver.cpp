#include "MyTcpServer.h"
#include "MyTcpSocketThread.h"
#include <QTime>

//构造函数
MyTcpServer::MyTcpServer(const QString &qstrFlag,QObject *parent) :
    m_qstrFlag(qstrFlag),QTcpServer(parent)
{
    progressEmitCount = 0;
    m_qiBytesWritten = 0;
    m_qiBytesToWrite = 0;
    m_qiFileTotalLength = 0;
    m_qiPeerDataSize = 64 * 1024;
}

//重新定义了incomingConnection这个虚函数，开辟一个新的tcpsocket线程，从TcpServer获得socketDescriptor，并完成相应的信号连接。
void MyTcpServer::incomingConnection(qintptr qintSocketID)
{
    if (m_qstrFlag=="C")
    {
        qDebug() << tr("incomingConnection:chatSocket  connected");
        //聊天socket
        MyTcpSocketThread *chatTcpSocketThread = new MyTcpSocketThread();
        chatTcpSocketThread->writeDescriptor(QString::number(qintSocketID));
        //聊天消息从下层socket发送到server，然后server槽函数接受
        connect(chatTcpSocketThread,SIGNAL(sendChatToServer( QTcpSocket*, const QString&, const QString&, const QStringList&)),
                               this,SLOT(receiveChatFromThread( QTcpSocket*, const QString&, const QString&, const QStringList&)));
        //下层socket触发readyRead信号，在receiveChat中读取数据
        connect(chatTcpSocketThread->tcpSocket, SIGNAL(readyRead()), chatTcpSocketThread, SLOT(receiveChat()));
        //下层socket触发disconnected信号，在disconChat中处理断开
        connect(chatTcpSocketThread->tcpSocket, SIGNAL(disconnected()), chatTcpSocketThread, SLOT(disconChat()));
        //下层socket触发error信号，在disconChat中处理断开
        connect(chatTcpSocketThread->tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), chatTcpSocketThread, SLOT(disconChat()));
        //从下层socket发送聊天socket断开消息到server，然后server槽函数receivedisconChatFromThread接受并进行转发
        connect(chatTcpSocketThread, SIGNAL(sendChatDisconToServer( QTcpSocket*, const QString&)),
                               this, SLOT(receivedisconChatFromThread( QTcpSocket*, const QString&)));
        if (!chatTcpSocketThread->tcpSocket->setSocketDescriptor(qintSocketID))
        {
            return;
        }
        chatTcpSocketThread->start();
    }
    else
    {
        qDebug() << tr("incomingConnection:fileSocket  connected");
        //文件socket
        MyTcpSocketThread *fileTcpSocketThread = new MyTcpSocketThread();
        fileTcpSocketThread->writeDescriptor(QString::number(qintSocketID));
        //有文件通知到达即触发sendFileToServer，receiveFileFromThread进行接收
        connect(fileTcpSocketThread, SIGNAL(sendFileToServer( QTcpSocket*, const QString&, const QString&, const int&, const QString&)),
                               this, SLOT(receiveFileFromThread( QTcpSocket*, const QString&, const QString&, const int&, const QString&)));
        connect(fileTcpSocketThread->tcpSocket, SIGNAL(readyRead()), fileTcpSocketThread, SLOT(receiveFile()));
        //下层socket触发disconnected信号，在disconChat中处理断开
        connect(fileTcpSocketThread->tcpSocket, SIGNAL(disconnected()), fileTcpSocketThread, SLOT(disconFile()));
        //sendFileProcessToServer接收下层的文件进度
        connect(fileTcpSocketThread,SIGNAL(sendFileProcessToServer( QTcpSocket*, const QString&, const qint64&, const qint64&, const QString&)),
                               this,SLOT(receiveFileProcessFromThread( QTcpSocket*, const QString&, const qint64&, const qint64&, const QString&)));
        if (!fileTcpSocketThread->tcpSocket->setSocketDescriptor(qintSocketID))
        {
            return;
        }
        fileTcpSocketThread->start();
    }
}

//接收文件进度并转发到View层处理
void MyTcpServer::receiveFileProcessFromThread(QTcpSocket *socket,
                                         const QString    &qstrFileName,
                                         const qint64     &qstrTotalLength,
                                         const qint64     &qstrReceiveLength,
                                         const QString    &qstrSocketID)
{
    emit sendFileProcessToMain(socket, qstrFileName, qstrTotalLength, qstrReceiveLength, qstrSocketID);
}
//聊天服务断开，即客户端下线，转发到View层处理界面
void MyTcpServer::receivedisconChatFromThread(QTcpSocket  *socket,
                                        const QString     &qstrSocketID)
{
    emit sendDisconChatToMain(socket, qstrSocketID);
}

//接收聊天数据，并转发到View层
void MyTcpServer::receiveChatFromThread( QTcpSocket  *socket,
                                   const QString     &qstrSocketID,
                                   const QString     &qstrChatMessage,
                                   const QStringList &list)
{
    emit sendChatToMain(socket, qstrSocketID, qstrChatMessage, list);
}

//接收开始发送文件通知，并转发到View层
void MyTcpServer::receiveFileFromThread( QTcpSocket *socket,
                                   const QString    &qstrSender,
                                   const QString    &qstrReceiver,
                                   const int        &iFileStartorEnd,
                                   const QString    &qstrFileName)
{
    emit sendFileToMain(socket, qstrSender, qstrReceiver, iFileStartorEnd, qstrFileName);
}

//文件发送完成后再转发到接收者去
void MyTcpServer::writeFiletToReceiver(const QString &qstrSender,
                                       const QString &qstrReceiver,
                                       const QString &qstrFileName)
{
    QMap<QTcpSocket*, QString>::iterator it = Util::g_qmUserMessageMap.begin();
    QTcpSocket *tcp = NULL;
    while (it != Util::g_qmUserMessageMap.end())
    {
        if (it.value() == qstrReceiver)
        {
            tcp = it.key();
            break;
        }
        ++it;
    }
    if (tcp == nullptr)
    {
        return;
    }
    QString str = "file#" + qstrSender;
    tcp->write(str.toStdString().data());
    qDebug() << str.toStdString().data();
    //两条消息之间需要延时一会
    Util::delayMSec(500);
    sendFile(tcp, qstrFileName);
}

void MyTcpServer::sendFile(QTcpSocket * mTcpsocket,const QString &qstrFileName)
{
    qDebug() <<QApplication::applicationDirPath()+"/"+ qstrFileName;
    m_pLocalFile = new QFile(QApplication::applicationDirPath()+"/"+ qstrFileName);
    if (!m_pLocalFile->open(QFile::ReadOnly))
    {
        qDebug() << u8"打开文件出错";
        return;
    }
    m_qiFileTotalLength = m_pLocalFile->size(); //文件总大小
    qDebug() << m_qiFileTotalLength;
    QDataStream sendOut(&m_qByte_OutBlock, QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_5_9);
    sendOut << qint64(0) << qint64(0) << qstrFileName;
    //纯文件大小
    m_qiFileTotalLength += m_qByte_OutBlock.size();//这里的总大小是文件名大小等信息和实际文件大小的总和
    qDebug() << m_qByte_OutBlock.size();
    sendOut.device()->seek(0);//函数将读写操作指向从头开始
    sendOut << m_qiFileTotalLength << qint64((m_qByte_OutBlock.size() - sizeof(qint64) * 2));
    connect(mTcpsocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateReceiveProgress(qint64)));
    mTcpsocket->write(m_qByte_OutBlock);
    m_qiBytesToWrite = m_pLocalFile->size();
    m_qByte_OutBlock.resize(0);//清空发送缓冲区以备下次使用
    m_qiBytesWritten = 0;
}
void MyTcpServer::updateReceiveProgress(qint64 qintNumBytes)
{
    QTcpSocket * mTcpsocket = static_cast<QTcpSocket*>(this->sender());
    //已经发送数据的大小
    m_qiBytesWritten += (int)qintNumBytes;
    qDebug() << m_qiBytesWritten;
    if (m_qiBytesToWrite > 0) //如果已经发送了数据
    {
        //每次发送loadSize大小的数据，这里设置为16KB，如果剩余的数据不足16KB，就发送剩余数据的大小
        m_qByte_OutBlock = m_pLocalFile->read(qMin(m_qiBytesToWrite,m_qiPeerDataSize));
        //发送完一次数据后还剩余数据的大小
        m_qiBytesToWrite -= (int)mTcpsocket->write(m_qByte_OutBlock);
        qDebug() << m_qiBytesToWrite;
        //清空发送缓冲区
        m_qByte_OutBlock.resize(0);
        progressEmitCount++;
        if (progressEmitCount % 5 == 0 || m_qiBytesToWrite == 0) {
            emit sendFileProcessToMain(mTcpsocket, "", m_qiFileTotalLength, m_qiFileTotalLength - m_qiBytesToWrite, "");
        }
    }
     else
     {
             //如果没有发送任何数据，则关闭文件
             m_pLocalFile->close();
             qDebug() << "else";
             //m_qByte_OutBlock.resize(0);
             //progressEmitCount = 0;
             //m_qiBytesWritten = 0;
             //m_qiBytesToWrite = 0;
             //m_qiFileTotalLength = 0;
             //emit sendFileToMain(mTcpsocket, "", "", 2, "");//转发完毕
     }
    if (m_qiBytesWritten == m_qiFileTotalLength)
    {
        ////发送完毕
        progressEmitCount = 0;
        emit sendFileToMain(mTcpsocket, "", "", 2,"");//转发完毕
        m_qByte_OutBlock.resize(0);
        m_pLocalFile->close();
        m_qiBytesWritten = 0;
        m_qiBytesToWrite = 0;
        m_qiFileTotalLength = 0;
        disconnect(mTcpsocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateReceiveProgress(qint64)));
    }
}