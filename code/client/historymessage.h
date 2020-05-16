#ifndef HISTORYMESSAGE_H
#define HISTORYMESSAGE_H

#include <QWidget>
#include <QtWidgets/QWidget>
#include <QtNetwork/QtNetwork>
#include <QtWidgets/QWidget>
#include <QtNetwork/QtNetwork>
#include <QDialog>
#include <QTextBrowser>
#include "ui_historymessage.h"

namespace Ui {
class HistoryMessage;
}

class HistoryMessage : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryMessage(QWidget *parent = nullptr);
    ~HistoryMessage();
    void showMessage(QString strHistoryData);

private:
    Ui::HistoryMessage ui;
    QTcpSocket *mTcpSocket;
};

#endif // HISTORYMESSAGE_H
