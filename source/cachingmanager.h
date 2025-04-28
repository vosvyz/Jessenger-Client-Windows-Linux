#pragma once

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>

class CachingManager
{
public:
    CachingManager();
    QSqlDatabase db;
    QSqlQuery queryExecutor;
    void createMessage(qlonglong id, QString text, QString sender, qlonglong time, qlonglong chatId, bool isChatGroup);
    void createChatAssociation(qlonglong id, QString name, bool isGroup); // chat associations is a table in the cache that wires id of the chat and it's name
    QJsonArray getChatsWithLastMessageAsArray();
    QMap<QString, QString> cacheToGetChatsStandardMap; // This map translates names of DB columns to the names that are known by MainWindow::getYourChatsProcessed
};
