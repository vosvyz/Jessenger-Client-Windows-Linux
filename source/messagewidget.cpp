#include "messagewidget.h"

MessageWidget::MessageWidget(qlonglong id, bool temp, bool sentByUser, QString text, QString sender, qlonglong time) {
    this->id = id;
    this->temp = temp;
    this->sentByUser = sentByUser;
    this->text = text;
    this->sender = sender;
    this->time = time;
}

qlonglong MessageWidget::getId() {
    return id;
}

bool MessageWidget::isTemp() {
    return temp;
}

bool MessageWidget::isSentByUser() {
    return sentByUser;
}

QString MessageWidget::getText() {
    return text;
}

QString MessageWidget::getSender() {
    return sender;
}

qlonglong MessageWidget::getTime() {
    return time;
}

void MessageWidget::setId(qlonglong id) {
    this->id = id;
}

void MessageWidget::setTemp(bool temp) {
    this->temp = temp;
}

void MessageWidget::setText(QString text) {
    this->text = text;
}
