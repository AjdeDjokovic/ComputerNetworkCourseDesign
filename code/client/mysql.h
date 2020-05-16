#pragma once

#ifndef MYSQL_H
#define MYSQL_H
#include <QSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>

class MySql
{
public:
    ~MySql();
    void initsql();
    void createtable();
    void insertUser(QString name, QString password);
    void insertChatFileInfo(QString, QString, QString, QString, QString);
    void deleteUserInfo();
    QStringList selectChatFileInfo(QString, QString, QString);
    QString readUser();

private:
    QSqlQuery *query;
    QSqlDatabase db;
};
#endif
