#include <filemanager.h>

using namespace std;

/*
 * Этот класс обеспечивает сохранение токенов авторизации в файлах
 * Инстанция этого класса создаётся лишь в AuthorizationManager
 */

FileManager::FileManager() {
    QDir *homeDirectory = new QDir(QDir::homePath());
    if (!homeDirectory->exists("Jessenger/authentication")) {
        homeDirectory->mkdir("Jessenger/authentication");
    }
    accessTokenFile = new QFile(homeDirectory->path() + "/Jessenger/authentication/accesstoken.txt");
    refreshTokenFile = new QFile(homeDirectory->path() + "/Jessenger/authentication/refreshtoken.txt");
}

QString FileManager::getAccessToken() {
    accessTokenFile->open(QIODevice::ReadOnly);
    QString token = accessTokenFile->readLine();
    accessTokenFile->close();
    return token;
}

QString FileManager::getRefreshToken() {
    refreshTokenFile->open(QIODevice::ReadOnly);
    QString token = refreshTokenFile->readLine();
    refreshTokenFile->close();
    return token;
}

void FileManager::persistTokens(QString accessToken, QString refreshToken) {
    accessTokenFile->open(QIODevice::WriteOnly);
    refreshTokenFile->open(QIODevice::WriteOnly);
    accessTokenFile->write(accessToken.toStdString().c_str());
    refreshTokenFile->write(refreshToken.toStdString().c_str());
    accessTokenFile->close();
    refreshTokenFile->close();
}
