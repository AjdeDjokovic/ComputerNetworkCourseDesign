#include "mysql.h"

#pragma execution_character_set("utf-8")
MySql::~MySql()
{
    delete query;
}

void MySql::initsql()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("IM.db");
    if (db.open())
    {
        qDebug() << "Database connected successfully!";
        createtable();
        return;
    }
    else
    {
        qDebug() << "Database connected failed!";
        return;
    }
}

void MySql::createtable()
{
    query = new QSqlQuery;
    //创建用户表
    query->exec("create table user(name VARCHAR(30)  NOT NULL,password VARCHAR(30) NOT NULL)");
    query->exec("create table chatInfo(username VARCHAR(30)  NOT NULL,receiver VARCHAR(30) NOT NULL,currentFileName VARCHAR(100) NOT NULL,byteWritten VARCHAR(30),totalBytes VARCHAR(30))");
}

void MySql::insertUser(QString name,QString password)
{
    QString strcheck = QString("insert into user values('%1','%2')").arg(name).arg(password);
    query = new QSqlQuery;
    if (!query->exec(strcheck))
    {
        qDebug() << query->lastError();
        return;
    }
}

QString MySql::readUser()
{
    QString strcheck = QString("select name from user");
    query = new QSqlQuery;
    if (!query->exec(strcheck))
    {
        qDebug() << query->lastError();
        return "";
    }

    while (query->next())
    {
        return query->value(0).toString();
    }
}

void MySql::insertChatFileInfo(QString username, QString receiver, QString currentFileName, QString byteWritten, QString totalBytes)
{
        query = new QSqlQuery;
        QString strFind = QString("select * from chatInfo where username='%1' and currentFileName='%2' and receiver = '%3'")
                         .arg(username).arg(currentFileName).arg(receiver);
        if (query->exec(strFind))
        {
            if (query->next())
            {
                QString strUpdate = QString("update chatInfo set byteWritten ='%1'and totalBytes ='%2' where username ='%3' and currentFileName='%4' and receiver = '%5'")
                    .arg(byteWritten).arg(totalBytes).arg(username).arg(currentFileName).arg(receiver);
                bool result = query->exec(strUpdate);
                if (result)
                {
                    return;
                }
            }
            else {
                QString strInsert = QString("insert into chatInfo values('%1','%2','%3','%4','%5')")
                    .arg(username).arg(receiver).arg(currentFileName).arg(byteWritten).arg(totalBytes);
                bool result = query->exec(strInsert);
                if (result)
                {
                    return;
                }
            }
        }
        return;
}

void MySql::deleteUserInfo()
{
    QString strdelete = QString("delete from user");
    query = new QSqlQuery;
    if (!query->exec(strdelete))
    {
        qDebug() << query->lastError();
        return;
    }
}

QStringList MySql::selectChatFileInfo(QString filename,QString strUserName, QString receiver)
{
    QStringList strWholeList;
    QString strselect = QString("select * from chatInfo where currentFileName = '%1' and username = '%2' and receiver ='%3'")
        .arg(filename).arg(strUserName).arg(receiver);
    query = new QSqlQuery;
    if (!query->exec(strselect))
    {
        qDebug() << query->lastError();
        return strWholeList;
    }

    while (query->next())
    {
        strWholeList.append(query->value("currentFileName").toString());
        strWholeList.append(query->value("byteWritten").toString());
        strWholeList.append(query->value("totalBytes").toString());
    }

    return strWholeList;
}



