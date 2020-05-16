#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H
#include <QTcpServer>
#include <QFile>
#include <QTcpSocket>

//继承自QTcpServer，完成TcpServer的建立的类
class MyTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit MyTcpServer(const QString &flag, QObject *parent = 0);
    void writeFiletToReceiver(const QString &sender, const QString &receiver, const QString &fileName);
    void sendFile(QTcpSocket * mTcpSocket,const QString & fileName);

    //此信号用来更新UI
signals:
    //参数说明：客户端socket，socketID(句柄),服务端返回的完整消息体，客户端发送的完整消息体
    void sendChatToMain( QTcpSocket*, const QString&, const QString&, const QStringList&);
    //参数说明：客户端socket，socketID(句柄),服务端返回的完整消息体，客户端发送的完整消息体
    void sendFileToMain( QTcpSocket*, const QString&, const QString&, const int&, const QString&);
    void bytesArrived(const qint64&, const qint32&, const qintptr&);
    void sendDisconChatToMain( QTcpSocket*, const QString&);
    void sendFileProcessToMain( QTcpSocket*, const QString&, const qint64&, const qint64&, const QString&);

    public slots:
    void receiveChatFromThread( QTcpSocket*, const QString&, const QString&, const QStringList&);
    void receiveFileFromThread( QTcpSocket*, const QString&, const QString&, const int&, const QString&);
    void receivedisconChatFromThread( QTcpSocket*, const QString&);
    void receiveFileProcessFromThread( QTcpSocket*, const QString&, const qint64&, const qint64&, const QString&);
    void updateReceiveProgress(qint64);
protected:
    void incomingConnection(qintptr qintSocketID);
private:
    QString     m_qstrFlag;                           //标识聊天服务还是文件服务
    QFile      *m_pLocalFile;                         //文件对象
    qint64      m_qiFileTotalLength;                  //文件总长度
    qint64      m_qiBytesToWrite;                     //文件剩余大小
    qint64      m_qiBytesWritten;                     //已发送大小
    qint64      m_qiPeerDataSize;                     //每次转发文件大小
    QByteArray  m_qByte_OutBlock;                     //文件缓冲区
    qint64     progressEmitCount;          //控制不是每次都更新进度条，而是接收一定次数更新一次
};
#endif // MYTCPSERVER_H