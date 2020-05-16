#include "historymessage.h"
#include "ui_historymessage.h"

HistoryMessage::HistoryMessage(QWidget *parent) :
    QWidget(parent)
{
    ui.setupUi(this);
    this->setWindowTitle("聊天记录");
}

HistoryMessage::~HistoryMessage()
{
}

void HistoryMessage::showMessage(QString strHistoryData)
{
    QString breakStr;
    breakStr += strHistoryData;
    //结束符
    if (breakStr.endsWith("****")) {

        QStringList strWholeList = strHistoryData.split("record##");
        for (int i = 1; i < strWholeList.size(); i++)
        {
            QStringList strlDetailList = strWholeList[i].split("##");
            QString strRecord = "(" + strlDetailList[3] + ")";
            strRecord = strlDetailList[0].append(strRecord);
            ui.historytextBrowser->append("<font color = red>" + strRecord + "</font>");
            ui.historytextBrowser->append(strlDetailList[2]);
        }
        breakStr = "";
    }
}
