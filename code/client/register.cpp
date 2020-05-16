#include "Register.h"
#include "ui_register.h"
#include <QMessageBox>
#pragma execution_character_set("utf-8")
Register::Register(QDialog *parent, QTcpSocket *pTcpSocket, SocketUtil *mSocketUtil)
    : QDialog(parent), mTcpSocket(pTcpSocket), mSocketUtil(mSocketUtil)
{
    ui.setupUi(this);
    //设置父窗口的大小
    this->setWindowTitle("注册");
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    ui.le_password->setEchoMode(QLineEdit::Password);
    ui.le_password2->setEchoMode(QLineEdit::Password);
    init();
}

//析构中一定要关闭socket
Register::~Register()
{
    mTcpSocket->close();
    mTcpSocket->deleteLater();
}

void Register::init()
{
    //绑定注册
    connect(ui.pb_register, SIGNAL(clicked()), this, SLOT(onRegisterClicked()));
    //绑定socket错误提示
    mTcpSocket = new QTcpSocket();
    connect(mTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
        this, SLOT(displaySocketError(QAbstractSocket::SocketError)));
    //绑定接受数据
    connect(mTcpSocket, SIGNAL(readyRead()), this, SLOT(readMessages()));
    connectSocket();
}

void Register::connectSocket()
{
    mSocketUtil->connect(mTcpSocket);
}

//注册
void Register::onRegisterClicked()
{
    QString userName = ui.le_username->text();
    QString password = ui.le_password->text();
    QString password2 = ui.le_password2->text();
    if ("" == userName || "" == password || "" == password2)
    {
        QMessageBox::information(this, "警告", "输入不能为空", QMessageBox::Ok);
        return;
    }
    else if (password != password2)
    {
        QMessageBox::information(this, "警告", "两次密码输入不一致，请重新输入", QMessageBox::Ok);
        return;
    }
    QString bs = "register";
    QString data = bs + "#" + userName + "#" + password;
    mSocketUtil->writeData(mTcpSocket, data.toLatin1());
}

//接口回调成功
void Register::Success(QString message)
{
}

//接口回调失败
void Register::Fail(QString message)
{
}

//从服务端返回消息
void Register::readMessages()
{
    mTcpSocket->flush();
    QString data = mTcpSocket->readAll();
    QStringList list = data.split("#");
    if ("register" == list[0] && "true" == list[3])
    {
        QMessageBox::information(this, "信息提示", "注册成功，请返回登录", QMessageBox::Ok);
        this->setAttribute(Qt::WA_DeleteOnClose, 1);//销毁所有变量
        this->close();
    }
    else if ("register" == list[0] && "false" == list[3])
    {
        QMessageBox::information(this, "信息提示", "注册失败，用户已存在", QMessageBox::Ok);
    }
}

//Socket发生错误
void Register::displaySocketError(QAbstractSocket::SocketError)
{
    ui.label_regresult->setText(ui.label_regresult->text() + mTcpSocket->errorString() + "\n");
}