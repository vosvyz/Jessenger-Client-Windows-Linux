#include <userpushbutton.h>

/*
 * This class implements a button capable of storing additional information about a particular user
 * Used in the pool of chats
 */

UserPushButton::UserPushButton(qlonglong id, QString name, bool group, qlonglong lastMessageTime, QWidget *parent) {
    setParent(parent);
    this->lastMessageTime = lastMessageTime;
    this->name = name;
    this->id = id;
    this->group = group;
}

qlonglong UserPushButton::getId() {
    return id;
}

qlonglong UserPushButton::getLastMessageTime() {
    return lastMessageTime;
}

void UserPushButton::setLastMessageTime(qlonglong time) {
    lastMessageTime = time;
}

QString UserPushButton::getName() {
    return name;
}

bool UserPushButton::isGroup() {
    return group;
}
