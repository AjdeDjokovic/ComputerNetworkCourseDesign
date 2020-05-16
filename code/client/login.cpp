#include "login.h"
#include "ui_login.h"

#pragma execution_character_set("utf-8")
Login::Login(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);
    this->setWindowTitle("登录");
    ui->qle_ip->setFocus();
    ui->qle_ip->setText("127.0.0.1");
    ui->qle_port->setText("1234");
    ui->label_result->setStyleSheet("color:red;");
    ui->label_result->setWordWrap(true);
    ui->label_result->setAlignment(Qt::AlignTop);
    ui->qle_password->setEchoMode(QLineEdit::Password);
//    connect(ui->pushButton,SIGNAL(clicked()),this,SLOT(on_pb_login_clicked()));
//    connect(ui->pushButton_2,SIGNAL(clicked()),this,SLOT(on_pb_register_clicked()));
    QMetaObject::connectSlotsByName(ui->pb_login);
    QMetaObject::connectSlotsByName(ui->pb_register);
    mSocketUtil = new SocketUtil(this);
    mTcpSocket = new QTcpSocket();
    init();
}

//析构中一定要关闭socket
Login::~Login()
{
    mTcpSocket->close();
    mTcpSocket->deleteLater();
    mTcpSocket->disconnectFromHost();
    mTcpSocket = nullptr;
}

void Login::init()
{
    //绑定socket错误提示
    connect(mTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(displaySocketError(QAbstractSocket::SocketError)));
    //绑定接受数据
    connect(mTcpSocket, SIGNAL(readyRead()), this, SLOT(readMessages()));
}

void Login::on_pb_login_clicked()
{
    if ("" == ui->qle_ip->text() || "" == ui->qle_port->text())
    {
        QMessageBox::information(this, "警告", "ip或端口号为空!", QMessageBox::Ok);
        return;
    }
    connectSocket();
    QString strUserName = ui->qle_username->text();
    QString strPassword = ui->qle_password->text();
    if ( "" == strUserName || "" == strPassword)
    {
        QMessageBox::information(this, "警告", "输入不能为空", QMessageBox::Ok);
        return;
    }

    QString strModuleName = "login";
    QString strSendData = strModuleName + "#" + strUserName + "#" + strPassword;
    mSocketUtil->writeData(mTcpSocket, strSendData.toLatin1());
}

//注册
void Login::on_pb_register_clicked()
{
    //获取IP地址
    QString strIp = ui->qle_ip->text();
    //获取端口号
    QString strPort = ui->qle_port->text();
    mSocketUtil->WriteIni(strIp, strPort);
    mRegister= new Register(NULL, mTcpSocket, mSocketUtil);
    mRegister->setModal(true);
    mRegister->exec();
}

//接口回调成功
void Login::Success(QString message)
{
    ui->label_result->setText(ui->label_result->text() + message + "\n");
}

//接口回调失败
void Login::Fail(QString message)
{
    ui->label_result->setText(ui->label_result->text() + message + "\n");
}

//socket连接,默认进入界面直接建立连接
void Login::connectSocket()
{
    //获取IP地址
    QString strIp = ui->qle_ip->text();
    //获取端口号
    QString strPort = ui->qle_port->text();
    mSocketUtil->WriteIni(strIp, strPort);
    mSocketUtil->connect(mTcpSocket);
}

//从服务端返回消息
void Login::readMessages()
{
    QString strRevData = mTcpSocket->readAll();
    QStringList strlRevlist = strRevData.split("#");
    if ("login" == strlRevlist[0] && "true" == strlRevlist[3])
    {
        MySql mysql;
        mysql.initsql();
        mysql.deleteUserInfo();
        mysql.insertUser(strlRevlist[1], strlRevlist[2]);
        //更新用户信息
        mOnlineUserlist = new OnlineUserlist(NULL, mTcpSocket, mSocketUtil);
        mOnlineUserlist->show();
        this->close();
    }
    else if ("login" == strlRevlist[0] && "false" == strlRevlist[3])
    {
        QMessageBox::information(this, "信息提示", "登录失败，用户名或密码错误", QMessageBox::Ok);
    }
    else if ("nouser" == strlRevlist[0] && "false" == strlRevlist[3])
    {
        QMessageBox::information(this, "信息提示", "还未注册,请先注册", QMessageBox::Ok);
    }
}

//Socket发生错误
void Login::displaySocketError(QAbstractSocket::SocketError)
{
    ui->label_result->setText(ui->label_result->text() + mTcpSocket->errorString() + "\n");
}
