#pragma once

#include <QFrame>

class MessageWidget : public QFrame
{
Q_OBJECT

public:
    MessageWidget(qlonglong id, bool temp);
    qlonglong getId();
    bool isTemp();
    void setId(qlonglong id);
    void setTemp(bool temp);

private:
    qlonglong id;
    bool temp;
};
