#include "networkclient.h"

void NetworkClient::setRamTokenStorage(RamTokenStorage *ramTokenStorage) {
    this->ramTokenStorage = ramTokenStorage;
}

QNetworkRequest NetworkClient::formHttpRequest(QString endpoint)
{
    QString requestPath = "http://localhost:8080/" + endpoint;
    QUrl url(requestPath);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    return request;
}

QNetworkRequest NetworkClient::formHttpRequest(QString endpoint, QMap<QString, QString> body)
{
    QString requestPath = "http://localhost:8080/" + endpoint + "?" + getParamsAsUrlUnencoded(body);
    QUrl url(requestPath);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    return request;
}

QNetworkRequest NetworkClient::formWsRequest(QString endpoint)
{
    QString requestPath = "ws://localhost:8080/" + endpoint;
    QUrl url(requestPath);
    QNetworkRequest request(url);
    return request;
}

QByteArray NetworkClient::getParamsAsUrlUnencoded(QMap<QString, QString> params)
{
    QByteArray result;
    for (QMap<QString, QString>::iterator iter = params.begin(); iter != params.end(); ++iter) {
        QString key = iter.key();
        QString value = iter.value();
        result.append(QString(key + '=' + value + '&').toUtf8());
    }
    return result;
}

void NetworkClient::setAuthorizationHeader(QNetworkRequest &request)
{
    QMap<QString, QString> body;
    body.insert("refresh", ramTokenStorage->getRefreshToken());
    refreshToken(body);
    QString accessToken = ramTokenStorage->getAccessToken();
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
}

void NetworkClient::refreshToken(QMap<QString, QString> body) {
    if (body["refresh"].isEmpty()) {
        throw std::runtime_error("Wrong or exp");
    }
    QNetworkAccessManager manager;
    QNetworkRequest request = formHttpRequest("sign/refresh");
    QNetworkReply *reply = manager.post(request, getParamsAsUrlUnencoded(body));
    QEventLoop loop; // A loop is used to force a method to wait for its completion and throw an exception if something goes wrong
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() == QNetworkReply::HostNotFoundError ||
        reply->error() == QNetworkReply::ConnectionRefusedError) {
        reply->deleteLater();
        throw std::runtime_error("Has no internet connection");
    }
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 401) {
        reply->deleteLater();
        throw std::runtime_error("Wrong or exp");
    }
    QByteArray data = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    ramTokenStorage->setAccessToken(jsonDoc.object()["access"].toString());
    reply->deleteLater();
}
