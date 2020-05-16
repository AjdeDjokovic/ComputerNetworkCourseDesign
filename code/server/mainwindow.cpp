#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    getLocalIP();
    setWindowTitle("imServer  IP:" + localIP);
    ui->fileMsg->setStyleSheet("color:black");
    ui->fileMsg->setText(tr(u8"------离线模式下服务端接收完毕后进行转发------ \n"));
    chatServer = new MyTcpServer("C",this);
    fileServer = new MyTcpServer("F",this);
    ui->actClose->setEnabled(false);
    ui->actFileClose->setEnabled(false);
    connect(ui->actStart,&QAction::triggered,this,&MainWindow::startChat);
    connect(ui->actClose,&QAction::triggered,this,&MainWindow::closeChat);
    connect(ui->actFileStart,&QAction::triggered,this,&MainWindow::startFile);
    connect(ui->actFileClose,&QAction::triggered,this,&MainWindow::closeFile);
    MySql mySql;
    mySql.initSql();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete chatServer;
    delete fileServer;
    QMap<QTcpSocket*, QString>::iterator qmapIterator = Util::g_qmUserMessageMap.begin();
    while (qmapIterator != Util::g_qmUserMessageMap.end())
    {
         delete qmapIterator.key();
        qmapIterator++;
    }
    qmapIterator->clear();
}

void MainWindow::pushUserList()
{
    QString qstrSendData = "getUserList";
    QMap<QTcpSocket*, QString>::iterator qmapIterator = Util::g_qmUserMessageMap.begin();
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

void MainWindow::getLocalIP()
{
    QHostInfo hostInfo = QHostInfo::fromName(QHostInfo::localHostName());
    QList<QHostAddress> addList = hostInfo.addresses();
    if(!addList.isEmpty())
        for(int i = 0;i < addList.count();i++)
        {
            QHostAddress aHost = addList.at(i);
            if(QAbstractSocket::IPv4Protocol == aHost.protocol())
            {
                localIP = aHost.toString();
                break;
            }
        }
}

void MainWindow::startChat()
{
    ui->actStart->setEnabled(false);
    ui->actClose->setEnabled(true);
    bool ok = false;
    int inputValue = QInputDialog::getInt(this,"输入","输入要监听的端口号",1200,1000,100000,1,&ok);
    if(ok)
        chatServer->listen(QHostAddress(localIP),inputValue);
    connect(chatServer, SIGNAL(sendChatToMain( QTcpSocket*,const QString&, const QString&, const QStringList&)),
                  this, SLOT(receiveChatFromServer( QTcpSocket*, const QString&, const QString&, const QStringList&)));
    connect(chatServer, SIGNAL(sendDisconChatToMain( QTcpSocket*, const QString&)),
                  this, SLOT(receivedisconChatFromServer( QTcpSocket*, const QString&)));
    qDebug() << "chat  service start success";
}

void MainWindow::closeChat()
{
    ui->actStart->setEnabled(true);
    ui->actClose->setEnabled(false);
    qDebug() << ("chat service close");
    //主动关闭服务前给每个客户端发送close消息
    QMap<QTcpSocket*, QString>::iterator it = Util::g_qmUserMessageMap.begin();
    while (it != Util::g_qmUserMessageMap.end())
    {
        it.key()->write("close");
        it++;
    }
    //发送close消息完成之后删除所有客户端socket占用的内存
    while (it != Util::g_qmUserMessageMap.end())
    {
        delete it.key();
        it++;
    }
    chatServer->close();
    ui->onlineList->clear();
    Util::g_qmUserMessageMap.clear();

}

void MainWindow::receiveChatFromServer(QTcpSocket *socket,const QString &qstrSocketID,const QString &qstrResultMessage,const QStringList &list)
{
    if (list[0] == "login")
    {		//因为登录需要更新界面，所以把登录单出来
            ui->onlineList->addItem(list[1]);
            Util::g_qmUserMessageMap.insert(socket, list[1]);
    }
    qDebug() << "receiveChatFromServer：" << qstrResultMessage;
}

void MainWindow::receivedisconChatFromServer(QTcpSocket* socket, const QString &qstrSocketID)
{
    logincontrol::loginOut(qstrSocketID);
    QString name = Util::g_qmUserMessageMap.value(socket);
    QList<QListWidgetItem*> list = ui->onlineList->findItems(name, Qt::MatchCaseSensitive);
    if (list.isEmpty())
        return;
    QListWidgetItem *sel = list[0];
    int r = ui->onlineList->row(sel);
    QListWidgetItem *item = ui->onlineList->takeItem(r);
    ui->onlineList->removeItemWidget(item);
    //从在线用户列表删除
    Util::g_qmUserMessageMap.remove(socket);
    pushUserList();

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QString qstrSendData = "getUserList#";
    QMap<QTcpSocket*, QString>::iterator qmapIterator = Util::g_qmUserMessageMap.begin();
    while (qmapIterator != Util::g_qmUserMessageMap.end())
    {
        qmapIterator.key()->write(qstrSendData.toStdString().data());
        qmapIterator++;
    }
    logincontrol::closeServer();
}

void MainWindow::startFile()
{
    ui->actFileStart->setEnabled(false);
    ui->actFileClose->setEnabled(true);
    fileServer->listen(QHostAddress::Any, 4321);
    //提前绑定MyTcpServer类和主界面的槽函数,包括文件传输进度接收、开始发送文件通知
    connect(fileServer, SIGNAL(sendFileToMain( QTcpSocket*, const QString&, const QString&, const int&, const QString&)),
                  this, SLOT(receiveFileFromServer( QTcpSocket*, const QString&, const QString&, const int&, const QString&)));
    connect(fileServer, SIGNAL(sendFileProcessToMain( QTcpSocket*, const QString&, const qint64&, const qint64&, const QString&)),
                  this, SLOT(receiveFileProcessFromServer( QTcpSocket*, const QString&, const qint64&, const qint64&, const QString&)));
    qDebug() << "file  service start success";
}
//关闭文件服务
void MainWindow::closeFile()
{
    ui->actFileStart->setEnabled(true);
    ui->actFileClose->setEnabled(false);
    qDebug() << ("file service close");
    fileServer->close();
}

void MainWindow::receiveFileFromServer( QTcpSocket *socket,
                                    const QString    &qstrSender,
                                    const QString    &qstrReceiver,
                                    const int        &iFileStartorEnd,
                                    const QString    &qstrFileName)
{
    if (0 == iFileStartorEnd)
    {
    //0代表客户端文件开始发送
        QString text = QString(tr("收到来自%1的文件,转发给%2\n")).arg(qstrSender).arg(qstrReceiver);
        ui->fileMsg->setText(ui->fileMsg->toPlainText().append(text));
    }
    else  if (1 == iFileStartorEnd)
    {
        //1代表客户端文件发送完成
            ui->fileMsg->setText(ui->fileMsg->toPlainText().append(tr(u8"离线文件接收完毕，开始转发 \n")));
            fileServer->writeFiletToReceiver(qstrSender, qstrReceiver, qstrFileName);
            ui->progressBar->setValue(0);
    }
    else  if (2 == iFileStartorEnd)
    {
        //2代表服务端文件转发完成
        ui->fileMsg->setText(ui->fileMsg->toPlainText().append(tr(u8"转发完毕\n")));
    }
}

void MainWindow::receiveFileProcessFromServer( QTcpSocket *socket,
                                           const QString    &qstrFileName,
                                           const qint64     &qiTotalLength,
                                           const qint64     &qiReceiveLength,
                                           const QString    &qiSocketID)
{
    ui->progressBar->setOrientation(Qt::Horizontal);     // 水平方向
    ui->progressBar->setMinimum(0);						// 最小值
    ui->progressBar->setMaximum(qiTotalLength/100);		    // 最大值
    ui->progressBar->setValue(qiReceiveLength/100);		    // 当前进度
}
