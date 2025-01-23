#pragma once

#include <QString>

class WsMessage {
public:
    WsMessage(qlonglong createdAt, QString message, QString tempId);
    qlonglong getCreatedAt();
    QString getMessage();
    QString getTempId();
private:
    qlonglong createdAt;
    QString tempId;
    QString message;
};
