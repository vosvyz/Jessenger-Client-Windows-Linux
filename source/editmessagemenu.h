#pragma once

#include "messagewidget.h"
#include "qframe.h"
#include "qboxlayout.h"
#include "qpushbutton.h"
#include <QObject>

class EditMessageMenu : public QFrame
{
    Q_OBJECT
public:
    EditMessageMenu();
    void setTargetMessageWidget(MessageWidget* targetMessageWidget);
    MessageWidget* getTargetMessageWidget();
signals:
    void goBackClicked();
    void editClicked();
    void deleteClicked();
private:
    MessageWidget* targetMessageWidget;
};
