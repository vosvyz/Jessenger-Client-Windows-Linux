#pragma once

#include <QDebug>
#include <QObject>
#include <QException>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "networkclient.h"
#include <QNetworkAccessManager>
#include "ramtokenstorage.h"

/*
 * This class is responsible for communication with the server over HTTP
 * (typically, data is retrieved from the server, and one-time data is sent to the server via this protocol).
 */
class HttpClient : public NetworkClient
{
    Q_OBJECT

public:
    HttpClient();
signals:
    void checkRefreshTokenFailed();
    void refreshTokenVerified();
    void signProcessed();
    void signError(QString error);
    void shouldConfirmEmail();
    void getUserChatsFailed();
    void getUserChatsProcessed(QJsonObject data);
    void findChatsFailed();
    void findChatsProcessed(QJsonObject data);
    void getMessagesFailed();
    void getMessagesProcessed(QJsonObject data);
    void createGroupProcessed(QJsonObject data);
    void createGroupError(QString error);

public slots:
    void sign(QString endpoint, QMap<QString, QString> requestBody);
    void checkRefreshToken();
    void getUserChatsProxy();
    void findChatsProxy(QMap<QString, QString> body);
    void getMessagesProxy(QString endpoint, QMap<QString, QString> body);
    void createGroupProxy(QMap<QString, QString> body);
private:
    void getUserChats(int failCounter);
    void findChats(QMap<QString, QString> body, int failCounter);
    void getMessages(QString endpoint, QMap<QString, QString> body, int failCounter);
    void createGroup(QMap<QString, QString> body, int failCounter);
};
