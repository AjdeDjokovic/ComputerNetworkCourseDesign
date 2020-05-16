#include "onlineuserlist.h"
#include "ui_onlineuserlist.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QListView>
#include "mysql.h"
#include <QLineEdit>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>

OnlineUserlist::OnlineUserlist(QDialog *parent, QTcpSocket *pTcpSocket, SocketUtil *mSocketUtil)
    : QDialog(parent), mTcpSocket(pTcpSocket),mSocketUtil(mSocketUtil)
{
    ui.setupUi(this);
    //设置父窗口的大小
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    //搜索功能
    QPushButton *pSearchButton = new QPushButton(this);
    pSearchButton->setCursor(Qt::PointingHandCursor);
    pSearchButton->setFixedSize(22, 22);
    pSearchButton->setToolTip("搜索");
    pSearchButton->setStyleSheet("QPushButton{border-image:url(" + QApplication::applicationDirPath() + "/" + "icon_search_normal.png)}");
    //防止文本框输入内容位于按钮之下
    QMargins margins = ui.m_pSearchLineEdit->textMargins();
    ui.m_pSearchLineEdit->setTextMargins(margins.left(), margins.top(), pSearchButton->width(), margins.bottom());
    ui.m_pSearchLineEdit->setPlaceholderText("请输入搜索内容");
    QHBoxLayout *pSearchLayout = new QHBoxLayout();
    pSearchLayout->addStretch();
    pSearchLayout->addWidget(pSearchButton);
    pSearchLayout->setSpacing(0);
    pSearchLayout->setContentsMargins(0, 0, 0, 0);
    ui.m_pSearchLineEdit->setLayout(pSearchLayout);
    //搜索功能槽函数
    connect(pSearchButton, SIGNAL(clicked(bool)), this, SLOT(search()));

    mSocketUtil = new SocketUtil(this);
    bFlag = false;
    chatMap = new QMap<QString, Chat*>();
    init();
}

OnlineUserlist::~OnlineUserlist()
{
    mTcpSocket->close();
    mTcpSocket->deleteLater();
}

void OnlineUserlist::init()
{
    mTcpSocket = new QTcpSocket();
    connectSocket();
    MySql mysql;
    QString readData = mysql.readUser();
    this->setWindowTitle(readData);
    //绑定socket错误提示
    connect(mTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(displaySocketError(QAbstractSocket::SocketError)));
    //绑定接受数据
    connect(mTcpSocket, SIGNAL(readyRead()), this, SLOT(readMessages()));
    //mTcpSocket->waitForReadyRead();
    connect(ui.on_refresh, SIGNAL(clicked()), this, SLOT(getUserlist()));
    getUserlist();
    //发送方设置避免粘包
    mTcpSocket->waitForBytesWritten();
    SocketUtil::delayMSec(100);
    mTcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    QString str = "newconnect#";
    str += readData;
    mSocketUtil->writeData(mTcpSocket, str.toStdString().data());
}

void OnlineUserlist::connectSocket()
{
    mSocketUtil->connect(mTcpSocket);
}


void OnlineUserlist::getUserlist()
{
    QString data = "getUserList#";
    mSocketUtil->writeData(mTcpSocket, data.toLatin1());
}

void OnlineUserlist::displaySocketError(QAbstractSocket::SocketError)
{
    ui.label_UserlistError->setText(ui.label_UserlistError->text() + mTcpSocket->errorString() + "\n");
}

void OnlineUserlist::chatDestroyed(QString values)
{
    for (QMap<QString, Chat*>::iterator iterStrList = chatMap->begin();
        iterStrList != chatMap->end(); iterStrList++)
    {
        if (iterStrList.key() == values)
        {
            chatMap->remove(values);
            break;
        }
    }
}

void OnlineUserlist::search()
{
    QString strInputText = ui.m_pSearchLineEdit->text();
    bool bIsExist = false;
    if (!strInputText.isEmpty())
    {
        //遍历查找并高亮显示
        QList<QString>::Iterator it = user.begin(), itend = user.end();
        int i = 0;
        for (; it != itend; it++, i++) {
            if (*it == strInputText) {//找到，高亮显示
                QModelIndex index = mListModel->index(i);
                ui.listView->setCurrentIndex(index);
                bIsExist = true;
                break;
            }
        }
        if (!bIsExist) {
            QMessageBox::information(this, "信息提示", "该用户不存在!", QMessageBox::Ok);
        }
    }
}

void OnlineUserlist::readMessages() {

    //接收文件
    qint64 availableSize = mTcpSocket->bytesAvailable();
    QByteArray xx = mTcpSocket->readAll();
    QString data = xx.toStdString().data();
    QStringList list = data.split("#");
    if (data.endsWith("****")) {
        list[0] = "record";
    }
    if ("getUserList" == list[0])
    {
        //显示在线用户
        user.clear();
        for (int i = 1; i < list.size(); i++)
        {
            user += list[i];
        }
        mListModel = new QStringListModel(this);
        ui.listView->setModel(mListModel);
        mListModel->setStringList(user);
        ui.listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        connect(ui.listView, SIGNAL(doubleClicked(const QModelIndex)), this, SLOT(newChatWindow()));
    }
    else if ("chat" == list[0])
    {
        for (QMap<QString, Chat*>::iterator iterStrList = chatMap->begin(); iterStrList != chatMap->end(); iterStrList++)
        {
                if (iterStrList.key() == list[1])
                {
                    iterStrList.value()->showMessage(data);
                    return;
                }
        }
        //消息到来未打开窗口，自动弹出新窗口
        popWindow(list[1]);
        for (QMap<QString, Chat*>::iterator iterStrList = chatMap->begin(); iterStrList != chatMap->end(); iterStrList++)
        {
            if (iterStrList.key() == list[1])
            {
                iterStrList.value()->showMessage(data);
                return;
            }
        }
    }
    ////查找历史聊天记录
    //else if ("record" == list[0] && data.endsWith("****")) {
    //	view = new HistoryMessage();//将类指针实例化
    //	view->show();
    //	view->showMessage(data);
    //}
    else if ("record" == list[0]) {
        if (data.startsWith("record") && data.endsWith("****")) {
            //完整聊天
            view = new HistoryMessage();//将类指针实例化
            view->show();
            view->showMessage(data);
        }
        else  if (data.startsWith("record") && !data.endsWith("****")) {
            //不完整的第一段聊天
            view = new HistoryMessage();
            view->show();
            view->showMessage(data);
        }
        else  if (!data.startsWith("record") && data.endsWith("****")) {
            //不完整的第二段聊天，不弹窗
            view->showMessage(data);
        }
    }
}


void OnlineUserlist::newChatWindow()
{
    //打开消息发送窗口
    value = mListModel->data(ui.listView->currentIndex(), Qt::DisplayRole).toString();
    for (QMap<QString,Chat*>::iterator iterStrList = chatMap->begin(); iterStrList != chatMap->end(); iterStrList++)
    {
        if (iterStrList.key() == value)
        {
            bFlag = true;
        }
    }

    if (bFlag == false)
    {

        mchat = new Chat(mTcpSocket,mSocketUtil, value);
        mchat->show();
        connect(mchat, SIGNAL(closechat(QString)), this, SLOT(chatDestroyed(QString)));
        chatMap->insert(value,mchat);
    }

    bFlag = false;
}


void OnlineUserlist::popWindow(QString str)
{
    for (QMap<QString, Chat*>::iterator iterStrList = chatMap->begin(); iterStrList != chatMap->end(); iterStrList++)
    {
        if (iterStrList.key() == str)
        {
            bFlag = true;
        }
    }

    if (bFlag == false)
    {

        mchat = new Chat(mTcpSocket, mSocketUtil, str);
        mchat->show();
        connect(mchat, SIGNAL(closechat(QString)), this, SLOT(chatDestroyed(QString)));
        chatMap->insert(str, mchat);
    }

    bFlag = false;
}

void OnlineUserlist::Success(QString message)
{
}

void OnlineUserlist::Fail(QString message)
{
}


