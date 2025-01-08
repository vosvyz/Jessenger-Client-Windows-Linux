#include <userpushbutton.h>

/*
 * This class implements a button capable of storing additional information about a particular user
 * Used in the pool of chats
 */

UserPushButton::UserPushButton(qlonglong id, QString name, bool group, QWidget *parent) {
    setParent(parent);
    this->name = name;
    this->id = id;
    this->group = group;
}

qlonglong UserPushButton::getId() {
    return id;
}

QString UserPushButton::getName() {
    return name;
}

bool UserPushButton::isGroup() {
    return group;
}
