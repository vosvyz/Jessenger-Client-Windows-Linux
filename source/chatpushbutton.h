#pragma once

#include "qpushbutton.h"
#include <QDebug>

// This widget allows storing additional information about the chat - its ID, name, and whether the chat is a group.
class ChatPushButton : public QPushButton
{
Q_OBJECT
public:
    ChatPushButton(qlonglong id, QString name, bool group, qlonglong lastMessageId, QString lastMessageSenderName = "", qlonglong lastMessageTime = 0);
    qlonglong getId();
    QString getName();
    bool isGroup();
    qlonglong getLastMessageId();
    qlonglong getLastMessageTime();
    QString getLastMessageSenderName();
    void setLastMessageId(qlonglong messageId);
    void setLastMessageTime(qlonglong time);
    void setLastMessageSenderName(QString senderName);
private:
    qlonglong id;
    QString name;
    bool group;
    qlonglong lastMessageId;
    QString lastMessageSenderName;
    qlonglong lastMessageTime;
};
