#include "messagewidget.h"

MessageWidget::MessageWidget(qlonglong id) {
    this->id = id;
}

qlonglong MessageWidget::getId() {
    return id;
}
