#include "MySql.h"

#pragma execution_character_set("utf-8")
MySql::~MySql()
{
    delete query;
}

//初始化数据库
void MySql::initSql()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("SERVER.db");
    if (db.open())
    {
        qDebug() << "Database connected successfully!";
        createTable();
        return;
    }
    else
    {
        qDebug() << "Database connected failed!";
        return;
    }
}

//初始化，建立三张表，用户表，聊天记录表，断点续传表
void MySql::createTable()
{
    query = new QSqlQuery;
    //创建用户表
    query->exec("create table user(name VARCHAR(30)  NOT NULL,password VARCHAR(30) NOT NULL,isLogin VARCHAR(1),socketID VARCHAR(20))");
    //创建聊天记录表
    query->exec("create table chat(id INTEGER PRIMARY KEY AUTOINCREMENT,pointA varchar(30)  NOT NULL,pointB varchar(30) NOT NULL,content TEXT(5000),time varchar(50) NOT NULL)");
    //创建断点续传表
    query->exec("create table breakfile(fromA VARCHAR(30)  NOT NULL,fileName VARCHAR(150),fileTotalLength VARCHAR(20),hasReceiveLength VARCHAR(20))");
}

//用户登录
bool MySql::loginUser(const QString name, const QString passWard, const QString socketID, int &status, QString &message)
{
    QString strCheck = QString("select * from user where name='%1' and password='%2'").arg(name).arg(passWard);
    query = new QSqlQuery;
    if (!query->exec(strCheck))
    {
        qDebug() << query->lastError();
        return false;
    }
    if (!query->next())
    {
        status = 0;
        message = "wrong username or password";
        return false;
    }
    int isLogin = query->value("isLogin").toInt();
    if (isLogin)
    {
        //已经登录，不能重复登录
        status = 0;
        message = "repeat login";//重复登录
        return false;
    }
    QString strlogin = QString("update user set isLogin ='%1' , socketID ='%2' where name ='%3'").arg("1").arg(socketID).arg(name);
    bool ret = query->exec(strlogin);
    if (ret)
    {
        status = 1;
        message = "login success";//登录成功;
        return true;
    }
    return false;
}

bool MySql::registerUser(const QString name, const QString passWard, int &status, QString &message)
{
    QString strreg = QString("select * from user where name='%1'").arg(name);
    query = new QSqlQuery;
    if (!query->exec(strreg))
    {
        qDebug() << query->lastError();
        return false;
    }
    if (query->next())
    {
        status = 0;
        message = "username exists";
        return false;
    }
    strreg = QString("insert into user values('%1','%2','%3','%4')").arg(name).arg(passWard).arg(0).arg(" ");
    bool ret = query->exec(strreg);
    if (ret)
    {
        status = 1;
        message = "register success";
        return true;
    }
    return false;
}

bool MySql::loginStatusChange(QString userName, QString socketID)
{
    QString strsc = QString("update user set  socketID ='%1' where name ='%2'").arg(socketID).arg(userName);
    query = new QSqlQuery;
    bool result = query->exec(strsc);
    if (result)
    {
        return true;
    }
    return false;
}

bool MySql::loginOut(QString socketID)
{
    QString strsOut = QString("select * from user where socketID='%1'").arg(socketID);
    query = new QSqlQuery;
    if (query->exec(strsOut))
    {
        if (query->next())
        {
            QString strsOut = QString("update user set isLogin ='%1', socketID ='%2' where socketID ='%3'").arg("0").arg("0").arg(socketID);
            bool result=query->exec(strsOut);
            if (result)
            {
                return true;
            }
        }
    }
    return false;
}
//关闭服务，所有人下线
bool MySql::closeServer()
{
    QString strsOut = QString("select * from user");
    query = new QSqlQuery;
    if (query->exec(strsOut))
    {
        while (query->next())
        {
            QString strsOut = QString("update user set isLogin ='%1', socketID ='%2'").arg("0").arg("0");
            query->exec(strsOut);
        }
        return true;
    }
    return false;
}


//插入或者更新同名文件的断点记录
bool MySql::insertBreakRecord(const QString & fileName,
                              const QString & fromUser,
                              const QString & fileTotalLength,
                              const QString & hasReceiveLength)
{
    query = new QSqlQuery;
    QString strFind = QString("select * from breakfile where fromA='%1' and fileName='%2'").arg(fromUser).arg(fileName);
    if (query->exec(strFind))
    {
        if (query->next())
        {
            QString strUpdate = QString("update breakfile set fileTotalLength ='%1'and hasReceiveLength ='%2' where fromA ='%3' and fileName='%4'")
                                        .arg(fileTotalLength).arg(hasReceiveLength).arg(fromUser).arg(fileName);
            bool result = query->exec(strUpdate);
            if (result)
            {
                return true;
            }
        }
        else {
            QString strInsert = QString("insert into breakfile values('%1','%2','%3','%4')").arg(fromUser).arg(fileName).arg(fileTotalLength).arg(hasReceiveLength);
            bool result = query->exec(strInsert);
            if (result)
            {
                return true;
            }
        }
    }
    return false;
}

//如果断点文件已上传完成，则删除此条记录
bool MySql::deleteBreakRecord(const QString & fileName, const QString & fromUser)
{
    query = new QSqlQuery;
    QString strDelete = QString("delete from breakfile where fileName = '%1' and fromA = '%2'").arg(fileName).arg(fromUser);
    if (query->exec(strDelete))
    {
        return true;
    }
    return false;
}
//插入聊天记录
bool MySql::insertChatRecord(const QString & sender, const QString & receiver, const QString & content, const QString & time)
{
    query = new QSqlQuery;
    QString strInsert = QString("insert into chat values(null,'%1','%2','%3','%4')").arg(sender).arg(receiver).arg(content).arg(time);
    bool result = query->exec(QString::fromLocal8Bit(strInsert.toLocal8Bit()));
    if (result)
    {
        qDebug() << u8"插入聊天记录:" << strInsert;
        return true;
    }
    return false;
}

//查找聊天记录
bool MySql::selectChatRecord(const QString & sender, const QString & receiver, QString &result)
{
    QString strChat = QString("select * from chat where (pointA='%1' and pointB='%2') or (pointA='%3' and pointB='%4') order by id")
                            .arg(sender).arg(receiver).arg(receiver).arg(sender);
    query = new QSqlQuery;
    if (!query->exec(strChat))
    {
        qDebug() << query->lastError();
        return false;
    }
    QString resultData = "record##";
    while (query->next()) {
        QString senderString = query->value(1).toString();
        QString receiverString = query->value(2).toString();
        QString contentString = query->value(3).toString();
        QString timeString = query->value(4).toString();
        result = result + resultData;
        result += senderString + "##";
        result += receiverString + "##";
        result += contentString + "##";
        result += timeString + "##";
    }
    qDebug() << u8"服务返回聊天记录:" << result;
    return true;
}