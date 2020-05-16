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
    void initSql();
    void createTable();
    bool loginUser(const QString name, const QString passWard, const QString sockID,
                   int &status,  QString &message);//登录
    bool loginOut(const QString socketID);
    bool closeServer();
    //登出
    bool registerUser(const QString name, const QString passWard,  int &status,  QString &message);//注册
    bool loginStatusChange(const QString name, const QString socketID);//客户端每次打开新窗口需要new新的socket，因此服务端需要根据登录名更新sockedID
    bool insertBreakRecord(const QString &fileName, const QString &fromUser,
                           const QString &fileTotalLength, const QString &hasReceiveLength);//插入断点记录
    bool deleteBreakRecord(const QString &fileName, const QString &fromUser);//删除已经完成的断点记录
    bool insertChatRecord(const QString &sender,  const QString &receiver,
                          const QString &content, const QString &time);//插入聊天记录
    bool selectChatRecord(const QString &sender, const QString &receiver, QString &result);//查找聊天记录
private:
    QSqlQuery *query;
};
#endif // MYSQL_H