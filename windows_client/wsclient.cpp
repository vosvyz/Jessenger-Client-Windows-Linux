#include "wsclient.h"

WsClient::WsClient() {

}

void WsClient::initialize() {
    socket = new QWebSocket();
    resendPendingMessagesTimer = new QTimer();
    resendPendingMessagesTimer->setInterval(10000);
    socketState = QAbstractSocket::UnconnectedState;
    connectSignals();
    connect();
}

void WsClient::connectSignals() {
    QObject::connect(socket, QWebSocket::textMessageReceived, this, WsClient::messageReceived);
    QObject::connect(socket, QWebSocket::stateChanged, this, WsClient::socketStateChanged);
    QObject::connect(resendPendingMessagesTimer, QTimer::timeout, this, WsClient::resendPendingMessages);
}

void WsClient::socketStateChanged(QAbstractSocket::SocketState state) {
    if (state == QAbstractSocket::ConnectedState) {
        resendPendingMessagesTimer->start();
        emit socketConnected();
    }
    else if (state == QAbstractSocket::UnconnectedState){
        resendPendingMessagesTimer->stop();
        emit socketDisconnected();
        connect();
    }
    socketState = state;
}

void WsClient::connect() {
    QNetworkRequest request = formWsRequest("websocket/connect");
    try {
        setAuthorizationHeader(request);
    }
    catch (const std::runtime_error& e) {
        QString what = e.what();
        if (what == "Wrong or exp") {
            emit unauthorized();
        }
        else if (what == "Has no internet connection") {
            connect();
        }
        return ;
    }
    socket->open(request);
    QTimer::singleShot(5000, this, [this]() {
        if (socketState != QAbstractSocket::ConnectedState) {
           socket->abort();
           connect();
        }
    });
}

void WsClient::messageReceived(QString message) {
    QByteArray dataAsArray = message.toUtf8();
    QJsonDocument dataAsDocument = QJsonDocument::fromJson(dataAsArray);
    QJsonObject data = dataAsDocument.object();
    if (data["method"] == "acknowledged") { // Acknowledged is set when the server processed the message
        QString tempId = data["tempId"].toString();
        for (int i = 0; i < pendingMessages.size(); ++i) {
            QJsonObject currentMessage = pendingMessages.at(i);
            QString currentMessageTempId = currentMessage["tempId"].toString();
            if (currentMessageTempId == tempId) {
                pendingMessages.remove(i);
                break;
            }
        }
    }
    else {
        emit socketMessageReceived(data);
    }
}

void WsClient::sendMessage(QJsonObject messageData) {
    pendingMessages.push_back(messageData);
    if (socketState == QAbstractSocket::ConnectedState) {
        socket->sendTextMessage(QJsonDocument(messageData).toJson());
    }
}

void WsClient::resendPendingMessages() {
    QDateTime utcDateTime = QDateTime::currentDateTimeUtc();
    qlonglong msecs = utcDateTime.toMSecsSinceEpoch();
    for (int i = 0; i < pendingMessages.size(); ++i) {
        QJsonObject currentMessage = pendingMessages.at(i);
        qlonglong currentMessageTime = currentMessage["time"].toVariant().toLongLong();
        if (currentMessageTime + 10000 < msecs) {
            if (socketState == QAbstractSocket::ConnectedState) {
                socket->sendTextMessage(QJsonDocument(currentMessage).toJson());
            }
        }
    }
}
