#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QtNetwork>
#include <QFile>

class Util
{
public:

    static qint64  strToQi64(const char    *cArrData);              //string转qint64的方法
    static void    delayMSec(const int     &iMsec);                 //利用事件循环实现的主线程不阻塞的延迟方法
    static int     iterFlod (const QString &qstrPath,
                             const QString &qstrFileName,
                             int     doubleNameCount );          //遍历文件夹文件夹下寻找是否存在同名文件，返回重名个数
    static QMap<QTcpSocket *, QString>     g_qmUserMessageMap;      //维护在线用户列表
};

#endif // UTIL_H