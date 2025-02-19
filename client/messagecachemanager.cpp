#include "messagecachemanager.h"

MessageCacheManager::MessageCacheManager()
{
    QDir *homeDirectory = new QDir(QDir::homePath());
    if (!homeDirectory->exists("Jessenger/cache")) {
        homeDirectory->mkdir("Jessenger/cache");
    }
    cacheToGetChatsStandardMap["time"] = "lastMessageTime";
    cacheToGetChatsStandardMap["text"] = "lastMessageText";
    cacheToGetChatsStandardMap["chat_id"] = "chatId";
    cacheToGetChatsStandardMap["to_group"] = "group";
    cacheToGetChatsStandardMap["sender_name"] = "lastMessageSenderName";
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(homeDirectory->path() + "/Jessenger/cache/cache.db");
    queryExecutor = QSqlQuery(db);
    db.open();
    queryExecutor.exec("CREATE TABLE IF NOT EXISTS chat_associations (chat_id INTEGER PRIMARY KEY, chat_name TEXT, is_group INTEGER) WITHOUT ROWID;");
    queryExecutor.exec("CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY, to_group INTEGER, chat_id INTEGER, text TEXT, sender_name TEXT, time INTEGER) WITHOUT ROWID    ;");
    db.close();
}

void MessageCacheManager::createChatAssociation(QVariant id, QVariant name, QVariant isGroup) {
    db.open();
    queryExecutor.prepare("INSERT OR REPLACE INTO chat_associations (chat_id, chat_name, is_group) VALUES (:id, :name, :isGroup)");
    queryExecutor.bindValue(":id", id);
    queryExecutor.bindValue(":name", name);
    queryExecutor.bindValue(":isGroup", isGroup);
    queryExecutor.exec();
    db.close();
}

void MessageCacheManager::createMessage(QJsonObject data, QVariant chatId, QVariant isChatGroup) {
    db.open();
    queryExecutor.prepare("INSERT OR REPLACE INTO messages (id, to_group, chat_id, text, sender_name, time) VALUES (:id, :toGroup, :chatId, :text, :senderName, :time)");
    queryExecutor.bindValue(":id", data["id"].toVariant());
    queryExecutor.bindValue(":text", data["text"].toVariant());
    queryExecutor.bindValue(":senderName", data["senderName"].toVariant());
    queryExecutor.bindValue(":time", data["time"].toVariant());
    queryExecutor.bindValue(":chatId", chatId);
    queryExecutor.bindValue(":toGroup", isChatGroup);
    queryExecutor.exec();
    db.close();
}

QJsonArray MessageCacheManager::getChatsWithLastMessageAsArray() {
    db.open();
    QJsonArray result;
    queryExecutor.exec("SELECT messages.*, ca.name AS name FROM messages JOIN chat_associations ca ON ca.id = messages.chat_id AND ca.is_group = 1 WHERE messages.id IN (SELECT MAX(id) FROM messages GROUP BY messages.group, messages.chat_id);");
    while (queryExecutor.next()) {
        QJsonObject jsonObject;
        for (int i = 0; i < queryExecutor.record().count(); ++i) {
            QString key = queryExecutor.record().fieldName(i);
            if (cacheToGetChatsStandardMap.contains(key)) {
                key = cacheToGetChatsStandardMap[key];
            }
            jsonObject.insert(key, queryExecutor.value(i).toString());
        }
        result.append(jsonObject);
    }
    db.close();
    return result;
}
