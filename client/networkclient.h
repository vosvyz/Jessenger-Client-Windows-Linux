#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QVector>
#include <QUrl>
#include <QUuid>
#include <QTimer>
#include <QDebug>
#include <QObject>
#include <QThread>
#include <QException>
#include <QByteArray>
#include <QEventLoop>
#include <QWebSocket>
#include <QJsonArray>
#include <wsmessage.h>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <authorizationmanager.h>

class NetworkClient : public QObject
{
Q_OBJECT

public:
    NetworkClient(QObject *parent = nullptr);
    bool isWebSocketConnected();
    ~NetworkClient();

signals:
    void httpSignProcessed();
    void createGroupProcessed(QJsonObject object);
    void createGroupError(QString error);
    void getGroupMessagesProcessed(QJsonArray result, qlonglong chatId, bool shouldScrollDown);
    void findChatsProcessed(QJsonArray result);
    void getYourChatsProcessed(QJsonArray result);
    void getDialogueMessagesProcessed(QJsonArray result, qlonglong chatId, bool shouldScrollDown);
    void httpSignError(QString requestPath, QString error);
    void unauthorizedSignal();
    void initialized();
    void webSocketConnectedSignal();
    void webSocketDisconnectedSignal();
    void webSocketMessageReceived(QJsonObject data);

private:
    void webSocketMessageReceivedSlot(const QString& message);
    QVector<WsMessage*> *pendingWsMessages;
    QTimer *sendPendingWsMessagesTimer;
    QWebSocket *webSocket;
    bool webSocketConnecting;
    bool webSocketConnected;
    QNetworkAccessManager *networkManager;
    AuthorizationManager *authorizationManager;
    QNetworkRequest createHttpRequest(QString path);
    bool refresh(QMap<QString, QString> body);
    bool setAuthorizationHeader(QNetworkRequest &request);
    QString toUrlEncoded(QMap<QString, QString> body);
    QByteArray formContent(QMap<QString, QString> body);
    QNetworkRequest createHttpRequest(QMap<QString, QString> body, QString path);

public slots:
    void sendPendingWsMessages();
    void deleteThis();
    void checkRefreshToken();
    void connectWebSocket();
    void sendMessage(QString message);
    void createGroup(QMap<QString, QString> body);
    void getGroupMessages(QMap<QString, QString> body);
    void findChats(QMap<QString, QString> body);
    void getYourChats();
    void getDialogueMessages(QMap<QString, QString> body);
    void sign(QMap<QString, QString> body, QString path);
    void initialize();

private slots:
    void webSocketConnectedSlot();
    void webSocketDisconnected();
};

#endif // NETWORKCLIENT_H
