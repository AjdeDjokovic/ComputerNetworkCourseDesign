#ifndef LOGINCONTROL_H
#define LOGINCONTROL_H

#include<QString>
#include<QStringList>
#include"MySql.h"

class logincontrol
{
public:
    logincontrol();
    static bool login(int&,QString&, const QString&, const QStringList&);
    static void loginOut(const QString&);
    static void loginStatusChange(const QString&, const QString&);
    static void closeServer();
};

#endif // LOGINCONTROL_H