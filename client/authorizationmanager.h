#ifndef TOKENMANAGERH_H
#define TOKENMANAGERH_H

#include <ctime>
#include <QDebug>
#include <QJsonObject>
#include <authorizationfilemanager.h>
#include <QJsonDocument>

using namespace std;

class AuthorizationManager {

public:
    AuthorizationManager();
    QString getAccessToken();
    QString getRefreshToken();
    void setAccessToken(QString token);
    void setBothTokens(QString accessToken, QString refreshToken);
    void setBothTokensFromFile();
    bool isAccessTokenExpired();
    bool isRefreshTokenExpired();
    QString getTokenPayload(QString token);
    ~AuthorizationManager();
private:
    QString accessToken;
    QString refreshToken;
    AuthorizationFileManager *fileManager;
};

#endif // TOKENMANAGERH_H
