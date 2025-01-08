#include <authorizationmanager.h>

using namespace std;

/*
 * Этот класс отвечает за хранение JWT-токенов в RAM и их обработку
 * Хранение токенов в оперативной памяти позволяет не обращаться к файлу с токеном при каждом запросе к серверу
 * Инстанция AuthorizationManager создается лишь в NetworkClient
 */

AuthorizationManager::AuthorizationManager() {
    fileManager = new FileManager();
    setBothTokensFromFile();
}

QString AuthorizationManager::getAccessToken() {
    return accessToken;
}

QString AuthorizationManager::getRefreshToken() {
    return refreshToken;
}

void AuthorizationManager::setAccessToken(QString token) {
    accessToken = token;
}

void AuthorizationManager::setBothTokens(QString accessToken, QString refreshToken) {
    this->accessToken = accessToken;
    this->refreshToken = refreshToken;
}

bool AuthorizationManager::isAccessTokenExpired() {
    QString payloadAsBase64 = getTokenPayload(accessToken);
    QByteArray payloadAsByteArray = QByteArray::fromBase64(payloadAsBase64.toUtf8());
    QJsonDocument payloadAsJsonDocument = QJsonDocument::fromJson(payloadAsByteArray);
    QJsonObject payload = payloadAsJsonDocument.object();
    int exp = payload["exp"].toInt();
    return exp < QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(); // токены выдаются по UTC+0, поэтому проверять их актуальность следует по этому же поясу
}

bool AuthorizationManager::isRefreshTokenExpired() {
    QString payloadAsBase64 = getTokenPayload(refreshToken);
    QByteArray payloadAsByteArray = QByteArray::fromBase64(payloadAsBase64.toUtf8());
    QJsonDocument payloadAsJsonDocument = QJsonDocument::fromJson(payloadAsByteArray);
    QJsonObject payload = payloadAsJsonDocument.object();
    int exp = payload["exp"].toInt();
    return exp < QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(); // токены выдаются по UTC+0, поэтому проверять их актуальность следует по этому же поясу
}

QString AuthorizationManager::getTokenPayload(QString token) {
    bool hadPoint = false;
    QString payload = "";
    for (QString::iterator iter = token.begin(); iter != token.end(); ++iter) {
        QChar current = *iter;
        if (current == '.') {
            if (hadPoint) {
                break;
            }
            else {
                hadPoint = true;
                continue;
            }
        }
        if (hadPoint) {
            payload += current;
        }
    }
    return payload;
}

// Вызывается во время запуска приложения
void AuthorizationManager::setBothTokensFromFile() {
    accessToken = fileManager->getAccessToken();
    refreshToken = fileManager->getRefreshToken();
}

AuthorizationManager::~AuthorizationManager() {
    fileManager->persistTokens(accessToken, refreshToken);
}
