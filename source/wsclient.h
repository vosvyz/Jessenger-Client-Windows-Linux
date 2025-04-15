#pragma once

#include <QDebug>
#include <QWebSocket>
#include <QTimer>
#include "networkclient.h"
#include "ramtokenstorage.h"

class WsClient : public NetworkClient
{
    Q_OBJECT

public:
    WsClient();
signals:
    void messageAcknowledged(QJsonObject data);
    void socketConnected();
    void socketDisconnected();
    void messageReceived(QJsonObject data);
public slots:
    void initialize();
    void sendMessage(QJsonObject messageData);
private slots:
    void messageReceivedSlot(QString message);
    void socketStateChanged(QAbstractSocket::SocketState state);
    void resendPendingMessages();
private:
    QAbstractSocket::SocketState socketState;
    void connect();
    void connectSignals();
    QWebSocket *socket;
    QTimer *resendPendingMessagesTimer;
    QVector<QJsonObject> pendingMessages;
};
