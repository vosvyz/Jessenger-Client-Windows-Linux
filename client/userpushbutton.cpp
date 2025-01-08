#include <userpushbutton.h>

/*
 * Этот класс реализует кнопку, способную хранить дополнительную информацию о некотором пользователе
 * Используется в пуле чатов
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
