#include <authorizationmanager.h>

using namespace std;

/*
 * This class is responsible for storing JWT tokens in RAM and processing them
 * Storing tokens in memory allows us to avoid accessing the token file with each server request
 * An instance of AuthorizationManager is only created within NetworkClient
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
    return exp < QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(); // Tokens are issued at UTC+0, so their validity should also be checked according to this time zone.
}

bool AuthorizationManager::isRefreshTokenExpired() {
    QString payloadAsBase64 = getTokenPayload(refreshToken);
    QByteArray payloadAsByteArray = QByteArray::fromBase64(payloadAsBase64.toUtf8());
    QJsonDocument payloadAsJsonDocument = QJsonDocument::fromJson(payloadAsByteArray);
    QJsonObject payload = payloadAsJsonDocument.object();
    int exp = payload["exp"].toInt();
    return exp < QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(); // Tokens are issued at UTC+0, so their validity should also be checked according to this time zone.
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

// Called during app initialization
void AuthorizationManager::setBothTokensFromFile() {
    accessToken = fileManager->getAccessToken();
    refreshToken = fileManager->getRefreshToken();
}

AuthorizationManager::~AuthorizationManager() {
    fileManager->persistTokens(accessToken, refreshToken);
}
