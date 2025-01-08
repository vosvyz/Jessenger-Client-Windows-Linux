#ifndef USERPUSHBUTTON_H
#define USERPUSHBUTTON_H

#include <QPushButton>

class UserPushButton : public QPushButton
{
Q_OBJECT

public:
    UserPushButton(qlonglong id, QString name, bool group, QWidget *parent = nullptr);
    qlonglong getId();
    bool isGroup();
    QString getName();

private:
    bool group;
    QString name;
    qlonglong id;
};

#endif // USERPUSHBUTTON_H
