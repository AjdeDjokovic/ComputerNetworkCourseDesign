#include "Util.h"
#include <QApplication>
//static 对象在cpp文件中定义
QMap<QTcpSocket *, QString> Util::g_qmUserMessageMap;


//字符串转64位整型
qint64 Util::strToQi64(const char *cArrData)
{
    int len = 0;
    int i = 0;
    int j = 0;
    qint64 nTmpRes = 0;
    qint64 ntmp10 = 1;
    if (cArrData == nullptr)
    {
        return 0;
    }
    len = strlen(cArrData);

    for (i = len - 1; i >= 0; i--)
    {
        ntmp10 = 1;
        for (j = 1; j<(len - i); j++)
        {
            ntmp10 = ntmp10 * 10;
        }
        nTmpRes = nTmpRes + (cArrData[i] - 48)* ntmp10;
    }
    return nTmpRes;
}

//遍历当层文件夹，寻找同名文件，若存在，返回重名个数
int Util::iterFlod(const QString & path, const QString & fileName,  int doubleNameCount)
{
        QFile *file = new QFile(path+"/"+fileName);
        QDir dir(path);

        foreach(QFileInfo mfi, dir.entryInfoList())
        {
            if (mfi.isFile())
            {
              QString filen =  mfi.fileName();
                if (filen == fileName)
                {
                    //重组，递归调用
                    filen = fileName.split(".")[0] + "(" + (QString::number(doubleNameCount + 1)) + ")" + fileName.split(".")[1];
                    doubleNameCount=iterFlod(QApplication::applicationDirPath(), filen, doubleNameCount);
                }
            }
        }
        return doubleNameCount;
    }

//非阻塞延时方法
void Util::delayMSec(const  int  &iMsec)
{
    QTime m_qTimer = QTime::currentTime().addMSecs(iMsec);
    while (QTime::currentTime() < m_qTimer)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

