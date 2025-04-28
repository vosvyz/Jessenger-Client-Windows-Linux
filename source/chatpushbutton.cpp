#include "chatpushbutton.h"

ChatPushButton::ChatPushButton(qlonglong id, QString name, bool group, qlonglong messageId, QString senderName, qlonglong time) {
    this->id = id;
    this->name = name;
    this->group = group;
    this->lastMessageId = messageId;
    this->lastMessageSenderName = senderName;
    this->lastMessageTime = time;
}

qlonglong ChatPushButton::getId() {
    return id;
}

QString ChatPushButton::getName() {
    return name;
}

bool ChatPushButton::isGroup() {
    return group;
}

qlonglong ChatPushButton::getLastMessageId() {
    return lastMessageId;
}

qlonglong ChatPushButton::getLastMessageTime() {
    return lastMessageTime;
}

QString ChatPushButton::getLastMessageSenderName() {
    return lastMessageSenderName;
}

void ChatPushButton::setLastMessageId(qlonglong messageId) {
    this->lastMessageId = messageId;
}

void ChatPushButton::setLastMessageTime(qlonglong time) {
    this->lastMessageTime = time;
}

void ChatPushButton::setLastMessageSenderName(QString senderName) {
    this->lastMessageSenderName = senderName;
}
