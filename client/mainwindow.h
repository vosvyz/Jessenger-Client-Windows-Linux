#pragma once

#include <cmath>
#include <QDebug>
#include <QLabel>
#include <QMovie>
#include <QThread>
#include <QScrollBar>
#include <stdexcept>
#include <QEventLoop>
#include <QJsonArray>
#include <QMainWindow>
#include <QStackedWidget>
#include <networkclient.h>
#include <userpushbutton.h>
#include <messagewidget.h>

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void connectWebSocket();
    void checkRefreshToken();
    void deleteNetworkClient();
    void sendMessage(QString message);
    void createGroup(QMap<QString, QString> body);
    void getGroupMessages(QMap<QString, QString> body);
    void findChats(QMap<QString, QString> body);
    void getYourChats();
    void getDialogueMessages(QMap<QString, QString> body);
    void sign(QMap<QString, QString> body, QString path);

private slots:
    void shouldConfirmEmailSlot();
    void findChatsProcessed(QJsonArray result);
    void httpSignProcessed();
    void createGroupProcessed(QJsonObject object);
    void createGroupError(QString error);
    void getGroupMessagesProcessed(QJsonArray result, qlonglong chatId, bool shouldScrollDown);
    void getYourChatsProcessed(QJsonArray result);
    void getDialogueMessagesProcessed(QJsonArray result, qlonglong chatId, bool shouldScrollDown);
    void httpSignError(QString error);
    void openChatSlot();
    void unauthorizedSignal();
    void socketConnected();
    void socketDisconnected();
    void messageReceived(QJsonObject data);
    void start();
    void messagesScrolled(int value);
    void on_toSignInButton_clicked();
    void on_toSignUpButton_clicked();
    void on_signInButton_clicked();
    void on_signUpButton_clicked();
    void on_findUserLineEdit_textEdited(const QString &arg1);
    void on_sendMessageButton_clicked();
    void on_createGroupButton_clicked();
    void on_toCreateGroupPageButton_clicked();
    void on_createGroupGoBackButton_clicked();
    void on_messageLineEdit_returnPressed();
    void emailConfirmTokenExpired();
    void on_backToSignInButton_clicked();

private:
    int calculateLineCount(const QString& text, const QFontMetrics& metrics, int labelWidth);
    QString chosenTypeOfLogin;
    QTimer* emailConfirmTokenExpiredTimer;
    QLabel* createStyledLabel(const QString &text, const QString &style);
    QVector <qlonglong> *fullyLoadedChats;
    QVector<UserPushButton*> *yourChats;
    void connectSignals();
    void configureChatButton(UserPushButton *button, int neededHeight);
    void setHomePage();
    Ui::MainWindow *ui;
    bool isCurrentChatGroup;
    qlonglong currentChatId;
    QString currentChatName;
    NetworkClient *networkClient;
    QThread *networkClientThread;
    QStackedWidget *stackedWidget;
    void clearMessages();
    void clearChats();
    void setPageByName(const QString& pageName);
};
