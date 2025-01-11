#ifndef USERPUSHBUTTON_H
#define USERPUSHBUTTON_H

#include <QPushButton>

class UserPushButton : public QPushButton
{
Q_OBJECT

public:
    UserPushButton(qlonglong id, QString name, bool group, qlonglong lastMessageTime, QWidget *parent = nullptr);
    qlonglong getId();
    bool isGroup();
    QString getName();
    void setLastMessageTime(qlonglong time);
    qlonglong getLastMessageTime();

private:
    bool group;
    QString name;
    qlonglong id;
    qlonglong lastMessageTime;
};

#endif // USERPUSHBUTTON_H
