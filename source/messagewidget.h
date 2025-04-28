#pragma once

#include "clickableframe.h"

class MessageWidget : public ClickableFrame
{
Q_OBJECT

public:
    MessageWidget(qlonglong id, bool temp, bool sentByUser, QString text, QString sender, qlonglong time);
    qlonglong getId();
    bool isTemp();
    bool isSentByUser();
    QString getText();
    QString getSender();
    qlonglong getTime();
    void setId(qlonglong id);
    void setTemp(bool temp);
    void setText(QString text);

private:
    qlonglong id;
    bool sentByUser;
    bool temp;
    QString text;
    QString sender;
    qlonglong time;
};
