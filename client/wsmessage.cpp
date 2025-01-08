#include <wsmessage.h>

WsMessage::WsMessage(qlonglong createdAt, QString message, QString tempId) {
    this->createdAt = createdAt;
    this->message = message;
    this->tempId = tempId;
}

qlonglong WsMessage::getCreatedAt() {
    return createdAt;
}

QString WsMessage::getMessage() {
    return message;
}

QString WsMessage::getTempId() {
    return tempId;
}
