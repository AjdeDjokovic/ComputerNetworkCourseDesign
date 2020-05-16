#ifndef REGISTERCONTROL_H
#define REGISTERCONTROL_H

#include<QString>
#include<QStringList>
#include"MySql.h"

class RegisterControl
{
public:
    RegisterControl();
    static bool registerUser(int&, QString&, const QString&, const QStringList&);
};

#endif // REGISTERCONTROL_H