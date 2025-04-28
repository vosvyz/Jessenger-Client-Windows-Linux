#pragma once

#include <QTimer>
#include <QScrollBar>
#include <QDebug>
#include <QRandomGenerator>
#include <QMap>
#include "editmessagemenu.h"
#include "messagewidget.h"
#include "chatpushbutton.h"
#include <QThread>
#include <QSharedPointer>
#include <QMovie>
#include <QJsonObject>
#include <QMainWindow>
#include "ramtokenstorage.h"
#include "httpclient.h"
#include "wsclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum class AuthorizationType {
        Login,
        Registration
    };

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void connectWsClient();
    void sendMessage(QJsonObject data);
    void sign(QString endpoint, QMap<QString, QString> requestBody);
    void getUserChats();
    void createGroup(QMap<QString, QString> data);
    void findChats(QMap<QString, QString> body);
    void getMessages(QString endpoint, QMap<QString, QString> body);

private slots:
    void hideEditMessageMenu();
    void editMessageClicked();
    void deleteMessageClicked();
    void messageRightClicked();
    void createGroupError(QString error);
    void groupCreated(QJsonObject data);
    void messagesScrolled(int value);
    void openChat();
    void messageReceived(QJsonObject data);
    void createMessageAcknowledged(QJsonObject data);
    void wsConnected();
    void wsDisconnected();
    void on_goToSignInButton_clicked();
    void on_goToSignUpButton_clicked();
    void on_signInButton_clicked();
    void on_signUpButton_clicked();
    void on_backToSignButton_clicked();
    void unauthorized();
    void signProcessed();
    void signError(QString error);
    void shouldConfirmEmail();
    void confirmEmailExpired();
    void showChats(QJsonObject data);
    void showMessages(QJsonObject data);
    void on_fromSignInToSignUpButton_clicked();
    void on_fromSignUpToSignInButton_clicked();
    void on_findUserLineEdit_textEdited(const QString &arg1);
    void on_sendMessageButton_clicked();
    void on_messageLineEdit_returnPressed();
    void on_goToCreateGroupPageButton_clicked();
    void on_createGroupButton_clicked();
    void on_createGroupGoBackButton_clicked();

    void on_cancelMessageEditButton_clicked();

private:
    QString messageOperationMode;
    QPointer<MessageWidget> editedMessage;
    EditMessageMenu* editMessageMenu;
    Ui::MainWindow *ui;
    QThread networkThread; // This thread contains both HttpClient and WsClient. The shared thread is needed to synchronize some job of these classes
    RamTokenStorage *ramTokenStorage;
    HttpClient httpClient;
    WsClient wsClient;
    QTimer confirmEmailExpiredTimer;
    AuthorizationType authType;
    bool atChat;
    bool isCurrentChatFullyLoaded;
    qlonglong currentChatId;
    QString currentChatName;
    bool isCurrentChatGroup;
    QVector<ChatPushButton*> yourChats;
    QFont basicFont; // The most usual font for this application -- Segoe UI, 16pt.
    void displayCreateGroupWarning(QString warning);
    void clearChats();
    void clearMessages();
    void connectSignals();
    QString createLastMessageInfo(QString text, QString sender, qlonglong time);
    ChatPushButton* createChatPushButton(qlonglong chatId, QString chatName, bool isChatGroup, qlonglong lastMessageId = 0,
                                         QString lastMessageText = "", QString lastMessageSender = "", qlonglong lastMessageTime = 0);
    MessageWidget* createMessageWidget(qlonglong id, QString text, QString sender, qlonglong time, bool isTemp);
    ChatPushButton* findChatButton(qlonglong chatId, bool group);
    void updateChatsWithCreateMessage(qlonglong chatId, QString chatName, bool group, qlonglong messageId, QString text, QString sender, qlonglong time);
    void updateChatsWithEditMessage(qlonglong chatId, bool group, qlonglong editedMessageId, QString text);
    void updateChatsWithDeleteMessage(qlonglong chatId, bool group, qlonglong deletedMessageId,
                                      qlonglong newMessageId = 0, QString text = "", QString sender = "", qlonglong time = 0);
    void displaySignInWarning(QString warning);
    void displaySignUpWarning(QString warning);
    int calculateLineCount(const QString& text, const QFontMetrics& metrics, int labelWidth);
};
