#ifndef MESSAGECACHEMANAGER_H
#define MESSAGECACHEMANAGER_H

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>

class MessageCacheManager
{
public:
    MessageCacheManager();
    QSqlDatabase db;
    QSqlQuery queryExecutor;
    void createMessage(QJsonObject data, QVariant chatId, QVariant isChatGroup);
    void createChatAssociation(QVariant id, QVariant name, QVariant isGroup); // chat associations is a table in the cache that wires id of the chat and it's name
    QJsonArray getChatsWithLastMessageAsArray();
    QMap<QString, QString> cacheToGetChatsStandardMap; // This map translates names of DB columns to the names that are known by MainWindow::getYourChatsProcessed
};

#endif // MESSAGECACHEMANAGER_H
