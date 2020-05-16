#pragma once
#ifndef  ISOCKETSINK_H
#define  ISOCKETSINK_H
#include <QString>
using std::string;
//用作Socket操作结果的回调
class ISOCKETSink
{
public:
    ISOCKETSink() {};
    virtual ~ISOCKETSink() {};
public:
    //使用两个虚方法更灵活一点
    virtual void Success(QString message) = 0;
    virtual void Fail(QString message) = 0;
};
#endif


