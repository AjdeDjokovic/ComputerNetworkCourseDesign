#include "SocketUtil.h"

//具体的连接操作
void SocketUtil::connect(QTcpSocket* socket)
{
    //取消已有的连接
    socket->abort();
    mSocket = new SocketUtil(mSock);
    QSettings *pIni = new QSettings(QCoreApplication::applicationDirPath() + "/ip.ini", QSettings::IniFormat);
    sArgIp = pIni->value("/setting/arg1").toString();
    sArgPort = pIni->value("/setting/arg2").toString();
    delete pIni;
    //连接服务器
    socket->connectToHost(sArgIp,sArgPort.toInt());
    //等待连接成功,10S等待时间
    if (!socket->waitForConnected(10000))
    {
        mSock->Fail(QString::fromLocal8Bit("Socket Fail"));
    }
    mSock->Success(QString::fromLocal8Bit("Socket Success"));
}


void SocketUtil::WriteIni(const QString &ip, const QString &port)
{
    //创建配置文件
    QSettings *pIni = new QSettings(QCoreApplication::applicationDirPath() + "/ip.ini", QSettings::IniFormat);
    //写入数据
    pIni->setValue("/setting/arg1", ip);
    pIni->setValue("/setting/arg2", port);
    delete pIni;
}

void SocketUtil::ReadIni()
{
    QSettings *pIni = new QSettings(QCoreApplication::applicationDirPath() + "/ip.ini", QSettings::IniFormat);
    sArgIp = pIni->value("/setting/arg1").toString();
    sArgPort = pIni->value("/setting/arg2").toString();
    delete pIni;
}

//写数据
void SocketUtil::writeData(QTcpSocket* socket, const char* data)
{
    socket->write(data);
    socket->flush();
}

//非阻塞延时方法
void SocketUtil::delayMSec(unsigned int msec)
{
    QTime _Timer = QTime::currentTime().addMSecs(msec);
    while (QTime::currentTime() < _Timer)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}