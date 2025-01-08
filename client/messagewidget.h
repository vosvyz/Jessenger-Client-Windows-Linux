#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

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

#endif // MESSAGEWIDGET_H
