#ifndef CHAT_H
#define CHAT_H

#include <QtWidgets/QWidget>
#include <QtNetwork/QtNetwork>
#include <QDialog>
#include <QTextBrowser>
#include "ui_chat.h"
#include "ISOCKETSink.h"
#include "SocketUtil.h"
#include <windows.h>
#include <commdlg.h>
#include <QFile>
#include <QCloseEvent>
#include <mysql.h>
#include <QDebug>

namespace Ui {
class Chat;
}

class Chat: public QDialog, public ISOCKETSink
{
    Q_OBJECT

public:
    //取消继承parent使得窗口可以随意显示
    Chat(QTcpSocket *pTcpSocket,SocketUtil *mSocketUtil = 0, QVariant value = "");
    ~Chat();
    void Success(QString message);
    void Fail(QString message);
    void init();
    QVariant value;
    void showMessage(QString str);
//***发送文件部分
    void string_replace(std::string & strBig, const std::string & strsrc, const std::string & strdst);
    std::string GetPathOrURLShortName(std::string strFullName);
    QString getCurrentTime();
    void updateServerProgress(const QByteArray&,const qint64 &);
protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::Chat ui;
    QTcpSocket *mTcpSocket;
    QTcpSocket *mSocket;
    SocketUtil *mSocketUtil;
    QTextBrowser *textBrowser;
//***发送文件部分
    QString fileName;
    TCHAR szFile[MAX_PATH];
    //传送文件相关变量
    qint64 totalBytes;
    qint64 total;
    qint64 Hsize;
    qint64 bytestoWrite;
    qint64 byteWritten;
    //每次发送数据大小
    qint64 perDataSize;
    QFile *localFile;
    //本地缓冲区
    QByteArray inBlock;
    QByteArray outBlock;
    MySql mysql;
    //断点续传
    qint64 FileSize;
    qint64 FileRemainderSize;
    qint64 FileSendSize;

    //接收文件
    qint64 bytesReceived;  //已收到数据的大小
    qint64 fileNameSize;  //文件名的大小信息
    qint64 revtTotalBytes;

    qint64 sendAndRecLen;

    QString currentFileName;
signals:
    void closechat(QString);

private slots:
    void sendMessage();
    void displaySocketError(QAbstractSocket::SocketError);
    void resive();
//***发送文件部分
    //打开并选择文件
    void openFile();
    //发送文件
    void sendFile();
    //更新进度
    void updateClientProgress(qint64);
    void stopFile();
    void resendFile();
    void updateResendProgress(qint64 numBytes);
    void selectHistoryChatMessage();

};
#endif
