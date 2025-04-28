#pragma once

#include <QFrame>
#include <QMouseEvent>

class ClickableFrame : public QFrame
{
    Q_OBJECT

public:
    ClickableFrame();

signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent *event) override;
};
