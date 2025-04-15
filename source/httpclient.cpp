#include "httpclient.h"

HttpClient::HttpClient() {

}

void HttpClient::checkRefreshToken() {
    QMap<QString, QString> body;
    body.insert("refresh", ramTokenStorage->getRefreshToken());
    try {
        refreshToken(body);
        emit refreshTokenVerified();
        getUserChatsProxy();
    }
    catch (const std::runtime_error& e) {
        QString what = e.what();
        if (what == "Wrong or exp") {
            emit unauthorized();
        }
        else if (what == "Has no internet connection") {
            checkRefreshToken();
        }
    }
}

void HttpClient::sign(QString endpoint, QMap<QString, QString> requestBody)
{
    QNetworkAccessManager *networkManager = new QNetworkAccessManager();
    QNetworkRequest request = formHttpRequest(endpoint);
    QNetworkReply *reply = networkManager->post(request, getParamsAsUrlUnencoded(requestBody));
    auto success = QSharedPointer<bool>::create(false);
    auto statusChecked = QSharedPointer<bool>::create(false);
    auto dataAsArray = QSharedPointer<QByteArray>::create(QByteArray());
    QObject::connect(reply, &QNetworkReply::readyRead, this, [success, statusChecked, dataAsArray, reply, this]() {
        if (!*statusChecked) {
            *statusChecked = true;
            QString status = QString::fromUtf8(reply->readAll()).simplified();
            status.replace("data:", "");
            if (status == "Not Found") {
                emit signError("User not found!");
                return;
            }
            else if (status == "Forbidden") {
                emit signError("Wrong password!");
                return;
            }
            else if (status == "Conflict") {
                emit signError("User already exists!");
                return;
            }
            else if (status == "Unprocessable Entity") {
                emit signError("Something went wrong, try again!");
                return;
            }
            else {
                *success = true;
                emit shouldConfirmEmail();
            }
        }
        else {
            QByteArray data = reply->readAll();
            dataAsArray->append(data);
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, this, [success, dataAsArray, reply, networkManager, this]() {
        if (reply->error() == QNetworkReply::HostNotFoundError || reply->error() == QNetworkReply::ConnectionRefusedError) {
            emit signError("We are experiencing some issues on our server! Try again, please.");
        }
        if (*success) {
            QByteArray nonPointerData = *dataAsArray;
            nonPointerData = nonPointerData.replace("data:", "");
            nonPointerData = nonPointerData.simplified();
            QJsonDocument dataAsDocument = QJsonDocument::fromJson(nonPointerData);
            QJsonObject data = dataAsDocument.object();
            ramTokenStorage->setAccessToken(data["access"].toString());
            ramTokenStorage->setRefreshToken(data["refresh"].toString());
            emit signProcessed();
            getUserChatsProxy();
        }
        reply->deleteLater();
        networkManager->deleteLater();
    });
}

void HttpClient::getUserChatsProxy() {
    getUserChats(0);
}

void HttpClient::getUserChats(int failCounter) {
    QNetworkAccessManager *networkManager = new QNetworkAccessManager();
    QNetworkRequest request = formHttpRequest("api/chats");
    try {
        setAuthorizationHeader(request);
    }
    catch (const std::runtime_error& e) {
        QString what = e.what();
        if (what == "Wrong or exp") {
            emit unauthorized();
        }
        else if (what == "Has no internet connection") {
            ++failCounter;
            if (failCounter == 3) {
                emit getUserChatsFailed();
            }
            else {
                getUserChats(failCounter);
            }
        }
        delete networkManager;
        return ;
    }
    QNetworkReply *reply = networkManager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [reply, networkManager, &failCounter, this]() {
        if (reply->error() == QNetworkReply::HostNotFoundError || reply->error() == QNetworkReply::ConnectionRefusedError) {
            ++failCounter;
            if (failCounter == 3) {
                emit getUserChatsFailed();
            }
            else {
                getUserChats(failCounter);
            }
        }
        else {
            QByteArray dataAsArray = reply->readAll();
            QJsonDocument dataAsDocument = QJsonDocument::fromJson(dataAsArray);
            QJsonObject data = dataAsDocument.object();
            data["your chats"] = true;
            emit getUserChatsProcessed(data);
        }
        reply->deleteLater();
        networkManager->deleteLater();
    });
}

void HttpClient::findChatsProxy(QMap<QString, QString> body) {
    findChats(body, 0);
}

void HttpClient::findChats(QMap<QString, QString> body, int failCounter) {
    QNetworkAccessManager *networkManager = new QNetworkAccessManager();
    QNetworkRequest request = formHttpRequest("api/find", body);
    try {
        setAuthorizationHeader(request);
    }
    catch (const std::runtime_error& e) {
        QString what = e.what();
        if (what == "Wrong or exp") {
            emit unauthorized();
        }
        else if (what == "Has no internet connection") {
            ++failCounter;
            if (failCounter == 3) {
                emit findChatsFailed();
            }
            else {
                findChats(body, failCounter);
            }
        }
        delete networkManager;
        return ;
    }
    QNetworkReply *reply = networkManager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [body, reply, networkManager, &failCounter, this]() {
        if (reply->error() == QNetworkReply::HostNotFoundError || reply->error() == QNetworkReply::ConnectionRefusedError) {
            ++failCounter;
            if (failCounter == 3) {
                emit findChatsFailed();
            }
            else {
                findChats(body, failCounter);
            }
        }
        else {
            QByteArray dataAsArray = reply->readAll();
            QJsonDocument dataAsDocument = QJsonDocument::fromJson(dataAsArray);
            QJsonObject data = dataAsDocument.object();
            data["filter"] = body["filter"];
            data["your chats"] = false;
            emit findChatsProcessed(data);
        }
        reply->deleteLater();
        networkManager->deleteLater();
    });
}

void HttpClient::getMessagesProxy(QString endpoint, QMap<QString, QString> body) {
    getMessages(endpoint, body, 0);
}

void HttpClient::getMessages(QString endpoint, QMap<QString, QString> body, int failCounter) {
    QNetworkAccessManager *networkManager = new QNetworkAccessManager();
    QNetworkRequest request = formHttpRequest(endpoint, body);
    try {
        setAuthorizationHeader(request);
    }
    catch (const std::runtime_error& e) {
        QString what = e.what();
        if (what == "Wrong or exp") {
            emit unauthorized();
        }
        else if (what == "Has no internet connection") {
            ++failCounter;
            if (failCounter == 3) {
                emit getMessagesFailed();
            }
            else {
                getMessages(endpoint, body, failCounter);
            }
        }
        delete networkManager;
        return ;
    }
    QNetworkReply *reply = networkManager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [endpoint, body, reply, networkManager, &failCounter, this]() {
        if (reply->error() == QNetworkReply::HostNotFoundError || reply->error() == QNetworkReply::ConnectionRefusedError) {
            ++failCounter;
            if (failCounter == 3) {
                emit getMessagesFailed();
            }
            else {
                getMessages(endpoint, body, failCounter);
            }
        }
        else {
            QByteArray dataAsArray = reply->readAll();
            QJsonDocument dataAsDocument = QJsonDocument::fromJson(dataAsArray);
            QJsonObject data = dataAsDocument.object();
            data["chatId"] = body["chatId"].toLongLong();
            data["just opened"] = !body.contains("lastMessageId");
            endpoint.endsWith("dialogue")
                    ? data["group"] = false
                    : data["group"] = true;
            emit getMessagesProcessed(data);
        }
        reply->deleteLater();
        networkManager->deleteLater();
    });
}

void HttpClient::createGroupProxy(QMap<QString, QString> body) {
    createGroup(body, 0);
}

void HttpClient::createGroup(QMap<QString, QString> body, int failCounter) {
    QNetworkAccessManager *networkManager = new QNetworkAccessManager();
    QNetworkRequest request = formHttpRequest("create/group");
    try {
        setAuthorizationHeader(request);
    }
    catch (const std::runtime_error& e) {
        QString what = e.what();
        if (what == "Wrong or exp") {
            emit unauthorized();
        }
        else if (what == "Has no internet connection") {
            ++failCounter;
            if (failCounter == 3) {
                emit createGroupError("Has no internet connection!");
            }
            else {
                createGroup(body, failCounter);
            }
        }
        delete networkManager;
        return ;
    }
    QNetworkReply *reply = networkManager->post(request, getParamsAsUrlUnencoded(body));
    QObject::connect(reply, &QNetworkReply::finished, this, [body, reply, networkManager, &failCounter, this]() {
        if (reply->error() == QNetworkReply::HostNotFoundError || reply->error() == QNetworkReply::ConnectionRefusedError) {
            ++failCounter;
            if (failCounter == 3) {
                emit createGroupError("Has no internet connection!");
            }
            else {
                createGroup(body, failCounter);
            }
        }
        else {
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == 409) {
                emit createGroupError("The group already exists!");
            }
            else {
                QByteArray dataAsArray = reply->readAll();
                QJsonDocument dataAsDocument = QJsonDocument::fromJson(dataAsArray);
                QJsonObject data = dataAsDocument.object();
                emit createGroupProcessed(data);
            }
        }
        reply->deleteLater();
        networkManager->deleteLater();
    });
}
