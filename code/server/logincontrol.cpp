#include"logincontrol.h"


//登录方法：statu状态码，message回复的交换数据，sockedID句柄，list接受的交换数据
bool logincontrol::login(int &status, QString &message, const QString &sockedID, const QStringList &list) {
    QString sendData = list[0];
    MySql mySql;
    mySql.loginUser(list[1], list[2], sockedID, status, message);
    if (status == 1) {
        sendData += "#" + list[1] + "#" + message + "#true";
        message = sendData;
        return true;
    }
    else {
        sendData += "#" + list[1] + "#" + message + "#false";
        message = sendData;
        return false;
    }
}
//登出方法：userName用户名
void logincontrol::loginOut(const QString & userName)
{
    MySql mySql;
    mySql.loginOut(userName);
}
//登录状态改变：userName用户名，sockedID句柄
void logincontrol::loginStatusChange(const QString & userName, const QString & sockedID)
{
    MySql mySql;
    mySql.loginStatusChange(userName,sockedID);
}

//关闭服务器，所有人下线
void logincontrol::closeServer()
{
    MySql mySql;
    mySql.closeServer();
}
