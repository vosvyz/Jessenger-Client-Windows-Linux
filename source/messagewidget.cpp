#include "messagewidget.h"

MessageWidget::MessageWidget(qlonglong id, bool temp) {
    this->id = id;
    this->temp = temp;
}

qlonglong MessageWidget::getId() {
    return id;
}

bool MessageWidget::isTemp() {
    return temp;
}

void MessageWidget::setId(qlonglong id) {
    this->id = id;
}

void MessageWidget::setTemp(bool temp) {
    this->temp = temp;
}
