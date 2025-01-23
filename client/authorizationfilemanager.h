#pragma once

#include <QFile>
#include <QDir>
#include <QDebug>

using namespace std;

class AuthorizationFileManager {

public:
    AuthorizationFileManager();
    QString getAccessToken();
    QString getRefreshToken();
    void persistTokens(QString accessToken, QString refreshToken);

private:
    QFile *accessTokenFile;
    QFile *refreshTokenFile;
};
