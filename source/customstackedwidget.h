#pragma once

#include "qstackedwidget.h"

class CustomStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    explicit CustomStackedWidget(QWidget *parent = nullptr);
    QString getCurrentPageName();
    void setPage(QString pageName);

private:
    QString currentPageName;
    QMap<QString, int> pageNameToIndexMap;
};
