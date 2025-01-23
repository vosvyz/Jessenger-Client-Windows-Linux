#pragma once

#include <QFrame>

class MessageWidget : public QFrame
{
Q_OBJECT

public:
    MessageWidget(qlonglong id);
    qlonglong getId();

private:
    qlonglong id;
};
