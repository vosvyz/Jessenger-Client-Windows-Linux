#include "clickableframe.h"
#include <QStyle>
#include <QDebug>

ClickableFrame::ClickableFrame()
    : QFrame() {}

void ClickableFrame::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        emit clicked();
    }
}
