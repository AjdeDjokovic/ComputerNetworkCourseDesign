#include "MyTcpSocketThread.h"
#include "logincontrol.h"
#include "RegisterControl.h"

//对于文件头信息只读一次，且文件只打开一次
//false表示第一次纯文件上传发送文件头，true表示开始发送正式文件，从而避免再次读取文件头等信息，此变量和断点续传无关
//此变量出现在receiveFile槽函数中，只和纯文件下载有关
bool m_bIsReaded = false;

MyTcpSocketThread::MyTcpSocketThread()
{
    tcpSocket = new QTcpSocket;
    m_qiBytesReceived = 0;
    m_qiTotalBytes = 0;
    m_qiFileNameSize = 0;
    m_iCurrentStatus = -1;
    progressEmitCount = 0;
}

MyTcpSocketThread::~MyTcpSocketThread()
{
    delete m_pLocalFile;
    m_pLocalFile = nullptr;
    tcpSocket->close();
}

void MyTcpSocketThread::run()
{}

//接受聊天消息
void MyTcpSocketThread::receiveChat()
{
    QByteArray  rvData = tcpSocket->readAll();
    QString buff = QString::fromStdString(rvData.toStdString());
    QStringList list = buff.split("#");
    if (list.isEmpty())
    {
        return;
    }
    if (list[0] == "login")
        //登录客户端发送数据格式： login#username#password
        //登录服务端返回数据格式： login#username#password#true
    {
        int status;
        QString message;
        if (logincontrol::login(status, message, m_qstrSockedID, list))
        {
            emit sendChatToServer(tcpSocket, m_qstrSockedID, message, list);
        }
        tcpSocket->write(message.toStdString().data());
    }else if (list[0] == "register")
        //注册客户端发送数据格式： register#username#password
        //注册服务端返回数据格式： register#username#password#true
    {
        int status;
        QString message;
        if (RegisterControl::registerUser(status, message, m_qstrSockedID, list))
        {
            emit sendChatToServer(tcpSocket, m_qstrSockedID, message, list);
        }
        tcpSocket->write(message.toStdString().data());
    }
    else if (list[0] == "getUserList")
        //获取在线用户列表客户端发送数据格式： getUserList
        //获取在线用户列表服务端返回数据格式：getUserList#username1#username2#username3.....
    {
        QString sendData = list[0];
        QMap<QTcpSocket*, QString>::iterator it = Util::g_qmUserMessageMap.begin();
        while (it != Util::g_qmUserMessageMap.end())
        {
            sendData += "#";
            QString x = it.value();
            sendData += x;
            ++it;
        }
        tcpSocket->write(sendData.toStdString().data());
    }
    else if (list[0] == "chat")
        //假设A要和B聊天
        //点对点聊天客户端发送数据格式：   chat#A#B#chatMessage
        //点对点聊天服务端返回给B数据格式：chat#A#chatMessage
    {
        QMap<QTcpSocket*, QString>::iterator qmapIterator = Util::g_qmUserMessageMap.begin();
        QTcpSocket *tcp = NULL;
        while (qmapIterator != Util::g_qmUserMessageMap.end())
        {
            if (qmapIterator.value() == list[2])
            {
                tcp = qmapIterator.key();
                break;
            }
            ++qmapIterator;
        }
        if (tcp == NULL)
        {
            return;
        }
        qDebug() << "chatfrom:" << buff;
        QString mes = "chat#" + list[1] + "#" + list[3];
        qDebug() << "chatTo:" << mes;
        MySql mySql;
        QDateTime time = QDateTime::currentDateTime();
        QString str = time.toString("yyyy-MM-dd hh:mm:ss:zzz");
        mySql.insertChatRecord(list[1], list[2], list[3], str);
        Util::delayMSec(10);
        tcp->write(mes.toStdString().data());
    }
    else if (list[0] == "record")
        //请求格式：record#发送者#接受者
        //返回格式：record##发送者##接受者##文本内容##发送时间##（精确到毫秒）
        //返回格式：record##发送者##接受者##文本内容##发送时间##（精确到毫秒）
    {
        QString result;
        MySql mySql;
        mySql.selectChatRecord(list[1], list[2], result);
        tcpSocket->write((result+ "****").toStdString().data());

    }
    else if (list[0] == "newconnect")
        //有新连接到达发送数据格式： newconnect#username
        //因为客户端打开某些窗口需要new新的socket，因此要通知服务端根据登录名去数据库更改客户端socket句柄
    {
        QMap<QTcpSocket*, QString>::iterator qmapIterator = Util::g_qmUserMessageMap.begin();
        QTcpSocket *tcp = NULL;
        while (qmapIterator != Util::g_qmUserMessageMap.end())
        {
            if (qmapIterator.value() == list[1])
            {
                Util::g_qmUserMessageMap.remove(qmapIterator.key());
                Util::g_qmUserMessageMap.insert(tcpSocket, list[1]);
                //重置用户表socketID
                logincontrol::loginStatusChange(list[1],m_qstrSockedID);
                break;
            }
            ++qmapIterator;
        }

        qmapIterator = Util::g_qmUserMessageMap.begin();
        QString qstrSendData = "getUserList";
        while (qmapIterator != Util::g_qmUserMessageMap.end())
        {
            qstrSendData += "#";
            QString x = qmapIterator.value();
            qstrSendData += x;
            ++qmapIterator;
        }
        qmapIterator = Util::g_qmUserMessageMap.begin();
        while (qmapIterator != Util::g_qmUserMessageMap.end())
        {
            qmapIterator.key()->write(qstrSendData.toStdString().data());
            qmapIterator++;
        }
    }
}
//接受文件消息,包括对一次性文件上传和断点续传的处理
void MyTcpSocketThread::receiveFile()
{
    QString qstrFileLength;
    QString qstrLaveLength;
    QDataStream QdsSendIn(tcpSocket);
    //为了保证不乱码，需要统一数据流写入版本
    QdsSendIn.setVersion(QDataStream::Qt_5_9);
    //bytesAvailable方法只是获取tcpSocket现有数据大小，但是不读出
    qint64 AvailableData = tcpSocket->bytesAvailable();
    if (!m_bIsReaded)
    {
        //这块的读取顺序需要和客户端商量，按照文件总长度+文件名长度+具体文件名的格式发送第一包，第二包开始即为正式文件
        //读取总共需接收的数据和文件名长度
        QdsSendIn >> m_qiTotalBytes >> m_qiFileNameSize ;
        //读取文件名
        QdsSendIn >> m_qstrFileName;
        //读取发送者和接受者
        QdsSendIn >> m_qstrSender >> m_qstrReceiver;
    }
    //readAll方法执行完之后数据被取出，tcpSocket便空了，里面没有数据了
    QByteArray  qbyte_RecData = tcpSocket->readAll();
    QString qstrRecData = QString::fromStdString(qbyte_RecData.toStdString());
    QStringList list = qstrRecData.split("#");
    //断点续传数据交换格式：continue#发送者#接受者#文件总长度#剩余数据长度#文件名(不带路径)
    if ((!list.isEmpty() && list.size() == 6) && list[0] == "continue")
    {		//断点续传需要在第一包先发一段文件信息，格式如上
            m_iCurrentStatus = 0;
            m_qstrSender = list[1];
            m_qstrReceiver = list[2];
            qstrFileLength = list[3];
            qstrLaveLength = list[4];
            m_qiFileTotalLength = qstrFileLength.toInt();
            m_qiContinueLength = m_qiFileTotalLength- qstrLaveLength.toInt();
            m_qstrFileName = list[5];
            m_pLocalFile = new QFile(QApplication::applicationDirPath() + "/" + m_qstrFileName);
            if (!m_pLocalFile->open(QFile::ReadWrite | QFile::Append))
            {
                qDebug() << tr(u8"写入文件错误");
            }
            return;
    }


    if (0==m_iCurrentStatus)
    {
        if (m_qiContinueLength < m_qiFileTotalLength)
        {
        //断点续传的时候continueLength直接从已发送大小开始追加写入
        m_qiContinueLength += AvailableData;
        m_qByte_InBlock = qbyte_RecData;
        m_pLocalFile->write(m_qByte_InBlock);
        m_qByte_InBlock.resize(0);
        progressEmitCount++;
        if (progressEmitCount % 10 == 0 || m_qiContinueLength == m_qiFileTotalLength) {
            //每接收5次更新一次进度条
            emit sendFileProcessToServer(tcpSocket,
                m_qstrFileName,
                m_qiFileTotalLength,
                m_qiContinueLength,
                m_qstrSender);
        }
    }
        //接收数据完成时
        if (m_qiContinueLength == m_qiFileTotalLength)
        {
            progressEmitCount = 0;
            m_pLocalFile->close();
            m_qiContinueLength = 0;
            m_qiFileTotalLength = 0;
            m_iCurrentStatus = -1;
            m_bIsReaded = false;
            qDebug() << tr(u8"成功接收文件 ");
            emit sendFileToServer(tcpSocket, m_qstrSender, m_qstrReceiver, 1, m_qstrFileName);
        }
    }
    else
    {
        //证明是新文件上传
        //如果接收到的数据小于等于16个字节，那么是刚开始接收数据，保存为头文件信息
        if (m_qiBytesReceived <= sizeof(qint64) * 2)
        {
            //接收文件名，并建立文件
            //确保连接上的数据已包含完整的文件名且文件名长度不为0(表示已从TCP连接接收文件名长度字段，处于第二步操作中)
            if ((AvailableData >= m_qiFileNameSize) && (m_qiFileNameSize != 0))
            {
                m_qiBytesReceived += sizeof(qint64) * 2;
                m_qiBytesReceived += m_qiFileNameSize;
                int doubleName = Util::iterFlod(QApplication::applicationDirPath(), m_qstrFileName, 0);
                if (doubleName==0)
                {
                    m_pLocalFile = new QFile(QApplication::applicationDirPath() + "/" + m_qstrFileName);
                }
                else
                {
                    //存在同名文件修改文件名为(1)、(2)
                    m_pLocalFile = new QFile(QApplication::applicationDirPath()
                                            + "/"
                                            + QString(m_qstrFileName+"(1%)").arg(doubleName));
                }
                if (!m_pLocalFile->open(QFile::WriteOnly))
                {
                    qDebug() << "文件打开错误";
                    return;
                }
                emit sendFileToServer(tcpSocket, m_qstrSender, m_qstrReceiver, 0, m_qstrFileName);
                m_bIsReaded = true;
                return;
            }
            else if (0==m_iCurrentStatus)
            {
            }
            else
            {
                return;
            }
        }
        //如果接收的数据小于总数据，那么写入文件
        if (m_qiBytesReceived < m_qiTotalBytes)
        {
            m_qiBytesReceived += AvailableData;
            m_qByte_InBlock = qbyte_RecData;
            m_pLocalFile->write(m_qByte_InBlock);
            m_qByte_InBlock.resize(0);
            //qDebug() << tr(u8"新文件总  长  度：") << m_qiTotalBytes- sizeof(qint64) * 2 - m_qiFileNameSize;
            //qDebug() << tr(u8"新文件已发送长度：") << m_qiBytesReceived - sizeof(qint64) * 2 - m_qiFileNameSize;
            progressEmitCount++;
            if (progressEmitCount % 10 == 0) {
                //每接收5次更新一次进度条
                emit sendFileProcessToServer(tcpSocket,
                    m_qstrFileName,
                    m_qiTotalBytes,
                    m_qiBytesReceived,
                    m_qstrSender);
            }
        }
        //接收数据完成时
        if (m_qiBytesReceived == m_qiTotalBytes)
        {
            progressEmitCount = 0;
            m_pLocalFile->close();
            m_qByte_InBlock.clear();
            AvailableData = 0;
            m_qiBytesReceived = 0;
            m_qiTotalBytes = 0;
            m_qiFileNameSize = 0;
            m_iCurrentStatus = -1;
            m_bIsReaded = false;
            qDebug() << tr(u8"成功接收文件 ");
            emit sendFileToServer(tcpSocket, m_qstrSender, m_qstrReceiver, 1, m_qstrFileName);
        }
    }
}

//聊天socket断开，发送到Server层
void MyTcpSocketThread::disconChat()
{
    emit sendChatDisconToServer(tcpSocket, m_qstrSockedID);
}
//文件socket断开
void MyTcpSocketThread::disconFile()
{
    while(tcpSocket->bytesAvailable() > 0) {
        receiveFile();
    }
}

void MyTcpSocketThread::writeDescriptor(const QString &qstrSocketID)
{
    m_qstrSockedID = qstrSocketID;
}

