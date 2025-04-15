#include "ramtokenstorage.h"

RamTokenStorage::RamTokenStorage()
{
    refreshToken = persistentTokenStorage.readRefreshToken();
}

RamTokenStorage::~RamTokenStorage()
{
    persistentTokenStorage.persistRefreshToken(refreshToken);
}

void RamTokenStorage::setAccessToken(QString accessToken)
{
    this->accessToken = accessToken;
}

void RamTokenStorage::setRefreshToken(QString refreshToken)
{
    this->refreshToken = refreshToken;
}

QString RamTokenStorage::getAccessToken() {
    return accessToken;
}

QString RamTokenStorage::getRefreshToken() {
    return refreshToken;
}

bool RamTokenStorage::isAccessTokenExpired()
{
    if (accessToken.isEmpty()) {
        return true;
    }
    qlonglong currentUtcTime = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(); // Tokens are issued at UTC+0, so their validity should also be checked according to this time zone.
    return (getExpiresAt(accessToken) < currentUtcTime);
}

bool RamTokenStorage::isRefreshTokenExpired()
{
    if (refreshToken.isEmpty()) {
        return true;
    }
    qlonglong currentUtcTime = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(); // Tokens are issued at UTC+0, so their validity should also be checked according to this time zone.
    return (getExpiresAt(refreshToken) < currentUtcTime);
}

qlonglong RamTokenStorage::getExpiresAt(QString token)
{
    QString payloadAsBase64 = getTokenPayload(token);
    QByteArray payloadAsByteArray = QByteArray::fromBase64(payloadAsBase64.toUtf8());
    QJsonDocument payloadAsJsonDocument = QJsonDocument::fromJson(payloadAsByteArray);
    QJsonObject payload = payloadAsJsonDocument.object();
    return payload["exp"].toVariant().toLongLong();
}

QString RamTokenStorage::getTokenPayload(QString token)
{
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
