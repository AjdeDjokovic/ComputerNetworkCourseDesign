#include "chat.h"
#include "login.h"
#include <QMessageBox>
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include "mysql.h"
#include <QFileDialog>
#include <time.h>
#include <QtCore>

#ifdef UNICODE
#define TCHARToQString(x)     QString::fromUtf16((x))
#else
#define TCHARToQString(x)     QString::fromLocal8Bit((x))
#endif

using namespace std;

#pragma execution_character_set("utf-8")
Chat::Chat(QTcpSocket *pTcpSocket,SocketUtil *mSocketUtil,QVariant itemValue)
    :mTcpSocket(pTcpSocket),mSocketUtil(mSocketUtil), value(itemValue)
{
    ui.setupUi(this);
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    mSocketUtil = new SocketUtil(this);
    this->setWindowTitle(value.toString());
    init();
}

Chat::~Chat()
{
    mTcpSocket->close();
    mTcpSocket->deleteLater();
    mTcpSocket->disconnectFromHost();
    mTcpSocket = nullptr;
    mSocket->close();
    mSocket->deleteLater();
    mSocket->disconnectFromHost();
    mSocket = nullptr;
    if (nullptr != localFile) {
        delete localFile;
        localFile = nullptr;
    }
}

//d#发送者#接受者用户名#消息
void Chat::init()
{
    connect(ui.pb_sendChatMessage, SIGNAL(clicked()), this, SLOT(sendMessage()));
    connect(ui.pb_selectFile, SIGNAL(clicked()), this, SLOT(openFile()));
    connect(ui.sendFile, SIGNAL(clicked()), this, SLOT(sendFile()));
    connect(ui.stopFile, SIGNAL(clicked()), this, SLOT(stopFile()));
    connect(ui.resendFile, SIGNAL(clicked()), this, SLOT(resendFile()));
    connect(ui.pb_selectHistoryChat, SIGNAL(clicked()), this, SLOT(selectHistoryChatMessage()));
    byteWritten = 0;
    bytestoWrite = 0;
    perDataSize = 64 * 1024;
    ui.progressBar->setValue(0);
    bytesReceived = 0;
    fileNameSize = 0;
    mSocket = new QTcpSocket;
    QSettings *pIni = new QSettings(QCoreApplication::applicationDirPath() + "/ip.ini", QSettings::IniFormat);
    QString port = "4321";
    mSocket->connectToHost(pIni->value("/setting/arg1").toString(), port.toInt());
    connect(mTcpSocket,&QTcpSocket::readyRead, this, &Chat::resive);
    qDebug() <<pIni->value("/setting/arg1").toString() << "    "<<port.toInt();
//    connect(mTcpSocket,SIGNAL(bytesReceived(qint64)), this, SLOT(updateServerProgress(qint64)));
    delete pIni;
}

//窗口关闭事件监听
void Chat::closeEvent(QCloseEvent *event)
{
    //断开连接
    disconnect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateResendProgress(qint64)));
    disconnect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateClientProgress(qint64)));
    mSocket->disconnectFromHost();
    mSocket->close();
    if (nullptr != outBlock) {
        outBlock.resize(0);
    }
    //保存当前状态信息
    MySql mysql;
    mysql.insertChatFileInfo(mysql.readUser(), value.toString(), currentFileName, tr("%1").arg(bytestoWrite), tr("%1").arg(total));
    qDebug() << "bytestoWrite" << bytestoWrite << "total" << total<<"已发送"<< total- bytestoWrite;
    emit closechat(value.toString());
}

//chat发送内容，w发送文件
void Chat::sendMessage()
{
    //获得发送内容—>发送至服务端—>转发给另一用户
    QString senddata = ui.qpte_chatMessage->toPlainText();
    if (senddata.size()>500)
    {
        QMessageBox::information(this, "警告", "发送消息长度超过显示", QMessageBox::Ok);
        return;
    }
    else if ("" == senddata)
    {
        QMessageBox::information(this, "警告", "发送内容为空", QMessageBox::Ok);
        return;
    }
    else
    {
        //用户名
        QString bs = "chat";
        QString readData = mysql.readUser();
        QString data = bs + "#" + readData + "#" + value.toString() + "#" + senddata;
        mSocketUtil->writeData(mTcpSocket, data.toStdString().data());
        SocketUtil::delayMSec(50);
        ui.tb_chatWindow->append("<font color = red>我("+ getCurrentTime()+"):</font>");
        QString showstr = "<font color = red>"+senddata+"</font>";
        ui.tb_chatWindow->append("  " + showstr);
        ui.tb_chatWindow->moveCursor(QTextCursor::End);
        ui.qpte_chatMessage->clear();
    }
}

//chat#发送者#接收者#消息
void Chat::showMessage(QString data)
{
    QStringList list = data.split("#");
    if ("chat" == list[0])
    {
        //判断当前框是否打开
        QString showstr = list[1];
        showstr += "(";
        showstr += getCurrentTime();
        showstr += "):";
        ui.tb_chatWindow->append("<font color = blue>"+ showstr +"</font>");
        ui.tb_chatWindow->append("<font color = blue>"+ list[2] + "</font>");
        ui.tb_chatWindow->moveCursor(QTextCursor::End);
    }
}

void Chat::displaySocketError(QAbstractSocket::SocketError)
{
    ui.label_error->setText(ui.label_error->text() + mTcpSocket->errorString() + "\n");
}

void Chat::Success(QString message)
{
}

void Chat::Fail(QString message)
{
}

void Chat::openFile()
{
    ui.sendFile->setEnabled(true);
    ui.pb_selectFile->setEnabled(true);
    ui.stopFile->setEnabled(true);
    ui.resendFile->setEnabled(true);
    fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {
        QString currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/') - 1);
        ui.qpte_chatMessage->setPlainText(currentFileName);
    }
    MySql mysql;
    QString currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/') - 1);
    //查找文件名和进度
    QStringList strlChatInfo = mysql.selectChatFileInfo(currentFileName, mysql.readUser(),value.toString());
    if (!strlChatInfo.isEmpty()) {
        if (currentFileName == strlChatInfo[0]
            && strlChatInfo[1].toInt() != 0) {
            bytestoWrite = strlChatInfo[1].toInt();
            this->localFile = new QFile(fileName);
            total = localFile->size();
            resendFile();
        }
    }
}

void Chat::sendFile()
{
    localFile = new QFile(fileName);
    if (!localFile->open(QFile::ReadOnly))
    {
        qDebug() << "打开文件出错";
        return;
    }
    currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/') - 1);
    totalBytes = localFile->size(); //文件总大小
    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_5_9);
    sendOut << qint64(0) << qint64(0) << currentFileName;
    //纯文件大小
    total = totalBytes;
    totalBytes += outBlock.size();
    MySql mysql;
    sendOut << mysql.readUser() << value.toString();
    sendAndRecLen = outBlock.size()+localFile->size();
    qDebug() << outBlock.size();
    qDebug() << localFile->size();
    qDebug() << sendAndRecLen;
    sendAndRecLen -= totalBytes;
    qDebug() << total;
    qDebug() << totalBytes;
    qDebug() << sendAndRecLen;
    //这里的总大小是文件名大小等信息和实际文件大小的总和
    sendOut.device()->seek(0);//函数将读写操作指向从头开始
    sendOut << totalBytes<< qint64((outBlock.size() - sizeof(qint64) * 2)- sendAndRecLen);
    connect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateClientProgress(qint64)));
    mSocket->write(outBlock);//发送完头数据后剩余数据的大小
    qDebug() << outBlock;
    bytestoWrite = localFile->size();
    Hsize = outBlock.size();
    outBlock.resize(0);//清空发送缓冲区以备下次使用

}

int iUpdateCount = 0;
// 更新进度条，实现文件的传送
void Chat::updateClientProgress(qint64 numBytes)
{
    //已经发送数据的大小
    byteWritten += (int)numBytes;
    if (bytestoWrite > 0) //如果已经发送了数据
    {
        //每次发送loadSize大小的数据，这里设置为4KB，如果剩余的数据不足4KB，就发送剩余数据的大小
        outBlock = localFile->read(qMin(bytestoWrite, perDataSize));
        //发送完一次数据后还剩余数据的大小
        bytestoWrite -= (int)mSocket->write(outBlock);
        //清空发送缓冲区
        outBlock.resize(0);
        iUpdateCount++;
    }
    else
    {
        //如果没有发送任何数据，则关闭文件
        localFile->close();
    }
    if (0 == iUpdateCount % 10) {
        //更新进度条
        ui.progressBar->setMaximum(total / 100);
        ui.progressBar->setMinimum(0);
        ui.progressBar->setValue((byteWritten - Hsize) / 100);
    }
    if (byteWritten == (totalBytes + sendAndRecLen)) //发送完毕
    //if (byteWritten == (totalBytes)) //发送完毕
    {
        ui.qpte_chatMessage->clear();
        QString showstr = mysql.readUser();
        showstr += "(";
        showstr += getCurrentTime();
        showstr += "):";
        ui.tb_chatWindow->append("<font color = red>" + showstr + "</font>");
        ui.tb_chatWindow->append("<font color = red>" + fileName + "发送成功!</font>");
        ui.tb_chatWindow->moveCursor(QTextCursor::End);
        ui.label_flileMessage->setText("文件发送成功!");
        outBlock.resize(0);
        localFile->close();
        byteWritten = 0; // 为下次发送做准备
        bytestoWrite = 0;
        totalBytes = 0;
        disconnect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateClientProgress(qint64)));
    }
}

void Chat::stopFile()
{
    //点击暂停按钮，保存用户名、当前文件名、已经发送字节数、总字节数
    //断开连接
    disconnect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateClientProgress(qint64)));
    mSocket->disconnectFromHost();
    mSocket->close();
    localFile->close();
    outBlock.resize(0);

    ui.qpte_chatMessage->clear();
    //显示进度
    QString showstr = value.toString();
    showstr += "(";
    showstr += getCurrentTime();
    showstr += "):";
    ui.tb_chatWindow->append("<font color = red>" + showstr + "</font>");
    ui.tb_chatWindow->append("<font color = red>" + fileName + "</font>");
    double dProgress = (ui.progressBar->value() - ui.progressBar->minimum()) * 100.0
                        / (ui.progressBar->maximum() - ui.progressBar->minimum());
    ui.tb_chatWindow->append("<font color = red>传输完成进度为" + QString::number(dProgress, 10, 2) + "%</font>");
    ui.tb_chatWindow->moveCursor(QTextCursor::End);
    ui.sendFile->setEnabled(false);
}

//重新发送
void Chat::resendFile()
{
    ui.sendFile->setEnabled(false);
    QSettings *pIni = new QSettings(QCoreApplication::applicationDirPath() + "/ip.ini", QSettings::IniFormat);
    QString port = "4321";
    mSocket->connectToHost(pIni->value("/setting/arg1").toString(), port.toInt());
    delete pIni;
    //mSocket->connectToHost("127.0.0.1", port.toInt());
    mSocket->waitForConnected(2000);
    QString currentFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/') - 1);
    MySql mysql;
    QString str = "continue#"+ mysql.readUser() +"#"+value.toString()+"#"+QString::number(localFile->size())
                    + "#" + QString::number(bytestoWrite)+ "#" + currentFileName;
    mSocket->write(str.toStdString().data());
    qDebug() << "重新连接" << str;
    Sleep(10);
    mSocket->waitForBytesWritten();
    connect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateResendProgress(qint64)));
    //打开文件，读取到指定位置
    this->localFile = new QFile(fileName);
    if (!localFile->open(QFile::ReadOnly))
    {
        return;
    }

    //实际文件总大小
    FileSize = localFile->size();
    //剩余文件大小
    FileRemainderSize = bytestoWrite;
    //已经发送文件大小
    FileSendSize = FileSize - FileRemainderSize;
    //localFile->seek(FileSendSize-1);
    localFile->seek(FileSendSize);
    outBlock = localFile->read(qMin(bytestoWrite, perDataSize));
    //发送完一次数据后还剩余数据的大小
    bytestoWrite -= (int)mSocket->write(outBlock);
    //已经发送数据的大小
    FileSendSize += outBlock.size();
    FileRemainderSize -= outBlock.size();
}

// 更新进度条，实现文件的传送
void Chat::updateResendProgress(qint64 numBytes)
{
    if (FileRemainderSize > 0) //如果已经发送了数据
    {
        outBlock = localFile->read(qMin(FileRemainderSize, perDataSize));
        FileRemainderSize -= (int)mSocket->write(outBlock);
        FileSendSize += (int)outBlock.size();
        //清空发送缓冲区
        outBlock.resize(0);
    }
    else
    {
        //如果没有发送任何数据，则关闭文件
        localFile->close();
    }

    //更新进度条
    ui.progressBar->setMaximum(total/100);
    ui.progressBar->setMinimum(0);
    ui.progressBar->setValue(FileSendSize/100);
    if (FileSendSize == FileSize) //发送完毕
    {
        outBlock.resize(0);
        localFile->close();
        FileSendSize = 0;
        FileRemainderSize = 0;
        byteWritten = 0; // 为下次发送做准备
        bytestoWrite = 0;
        //totalBytes = 0;
        //保存当前状态信息
        MySql mysql;
        //username、currentFileName、byteWritten、totalBytes
        mysql.insertChatFileInfo(mysql.readUser(), value.toString(), currentFileName, tr("%1").arg(bytestoWrite), tr("%1").arg(total));
        QString showstr = mysql.readUser();
        showstr += "(";
        showstr += getCurrentTime();
        showstr += "):";
        ui.tb_chatWindow->append("<font color = red>" + showstr + "</font>");
        ui.tb_chatWindow->append("<font color = red>" + fileName + "发送成功!</font>");
        ui.tb_chatWindow->moveCursor(QTextCursor::End);
        ui.label_flileMessage->setText("文件发送成功!");
        localFile->close();
        disconnect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(updateResendProgress(qint64)));
        ui.sendFile->setEnabled(true);
    }
}

void Chat::resive()
{
    qint64 num = mTcpSocket->bytesAvailable();
    qDebug() << num;
    QByteArray byte = mTcpSocket->readAll();
    updateServerProgress(byte,num);
}

//record#发送者#接受者
void Chat::selectHistoryChatMessage()
{
    MySql mysql;
    QString str = "record#" + mysql.readUser() + "#" + value.toString();
    mSocketUtil->writeData(mTcpSocket, str.toStdString().data());
}


// 更新进度条，实现文件的接收
void Chat::updateServerProgress(const QByteArray &byte, const qint64 &availableSize)
{
        ui.sendFile->setEnabled(false);
        ui.pb_selectFile->setEnabled(false);
        ui.stopFile->setEnabled(false);
        ui.resendFile->setEnabled(false);
        QDataStream sendIn(byte);
        sendIn.setVersion(QDataStream::Qt_5_9);
        if (bytesReceived <= sizeof(qint64) * 2)
        {
            //接收数据总大小信息和文件名大小信息
            if ((availableSize >= sizeof(qint64) * 2) && (fileNameSize == 0))
            {
                sendIn >> revtTotalBytes >> fileNameSize;//读取总共需接收的数据和文件名长度
                bytesReceived += sizeof(qint64) * 2;
            }

            //接收文件名，并建立文件
            if ((availableSize >= fileNameSize) && (fileNameSize != 0))
            {
                sendIn >> fileName;
                bytesReceived += fileNameSize;

                localFile = new QFile(QApplication::applicationDirPath()+"/"+fileName);
                if (!localFile->open(QFile::WriteOnly))
                {
                    qDebug() << "写入文件错误";
                }
                return;
            }
            else
                return;
        }
        //如果接收的数据小于总数据，那么写入文件
            if (bytesReceived < revtTotalBytes)
            {
                bytesReceived += availableSize;
                inBlock = byte;
                localFile->write(inBlock);
                qDebug() << "总接收" << revtTotalBytes;
                qDebug() << "已经收到" << bytesReceived;
                qDebug() << "发送块大小" << inBlock.size();
                //localFile->flush();
                inBlock.resize(0);
            }

            //更新进度条
            ui.progressBar->setMaximum(revtTotalBytes/100);
            ui.progressBar->setValue(bytesReceived/100);
            //接收数据完成时
            if (bytesReceived == revtTotalBytes)
            {
                localFile->close();
                bytesReceived = 0;
                totalBytes = 0;
                fileNameSize = 0;
                QString showstr = value.toString();
                showstr += "(";
                showstr += getCurrentTime();
                showstr += "):";
                ui.tb_chatWindow->append("<font color = blue>" + showstr + "</font>");
                ui.tb_chatWindow->append("<font color = blue>" + fileName + "接收成功!</font>");
                ui.tb_chatWindow->moveCursor(QTextCursor::End);
                ui.label_flileMessage->setText("文件接收成功!");
                ui.sendFile->setEnabled(true);
                ui.pb_selectFile->setEnabled(true);
                ui.stopFile->setEnabled(true);
                ui.resendFile->setEnabled(true);
            }
}

void Chat::string_replace(std::string &strBig, const std::string &strsrc, const std::string &strdst)
{
    std::string::size_type pos = 0;
    std::string::size_type srclen = strsrc.size();
    std::string::size_type dstlen = strdst.size();
    while ((pos = strBig.find(strsrc, pos)) != std::string::npos)
    {
        strBig.replace(pos, srclen, strdst);
        pos += dstlen;
    }
}

std::string Chat::GetPathOrURLShortName(std::string strFullName)
{
    if (strFullName.empty())
    {
        return "";
    }

    string_replace(strFullName, "/", "\\");
    std::string::size_type iPos = strFullName.find_last_of('\\') + 1;
    return strFullName.substr(iPos, strFullName.length() - iPos);
}

QString Chat::getCurrentTime() {
    time_t t = time(NULL);
    char ch[64] = { 0 };
    strftime(ch, sizeof(ch) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t));
    QString strCurrentTime;
    for (int i = 0; i < strlen(ch) + 1; i++)
    {
        strCurrentTime += ch[i];
    }

    return strCurrentTime;
}



