#include "registercontrol.h"

RegisterControl::RegisterControl()
{

}

bool RegisterControl::registerUser(int &status, QString &message, const QString &sockedID, const QStringList &list)
{
    QString qstrSendData = list[0];
    MySql mysql;
    mysql.registerUser(list[1], list[2], status, message);
    if (status == 1) {
        qstrSendData += "#" + list[1] + "#" + message + "#true";
        message = qstrSendData;
        return true;
    }
    else {
        qstrSendData += "#" + list[1] + "#" + message + "#false";
        message = qstrSendData;
        return false;
    }
}