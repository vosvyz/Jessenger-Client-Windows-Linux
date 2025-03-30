#include "mainwindow.h"
#include <./ui_mainwindow.h>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->chats->setAlignment(Qt::AlignTop);
    ui->messages->setAlignment(Qt::AlignTop);
    ui->chatsContainer->setLayout(ui->chats);
    ui->messagesContainer->setLayout(ui->messages);

    connectSignals();

    fillPageNameToIndexMap();

    ui->confirmEmailPictureLabel->setPixmap(QPixmap("assets/confirm_email.svg"));

    confirmEmailExpiredTimer.setSingleShot(true);
    confirmEmailExpiredTimer.setInterval(300000);

    QMovie *loadingGif = new QMovie("assets/signing.gif");
    ui->signInWaitingLabel->setMovie(loadingGif);
    ui->signInWaitingLabel->hide();
    ui->signUpWaitingLabel->setMovie(loadingGif);
    ui->signUpWaitingLabel->hide();
    ui->createGroupWaitingLabel->setMovie(loadingGif);
    ui->createGroupWaitingLabel->hide();

    basicFont.setFamily("Segoe UI");
    basicFont.setPointSize(16);

    ramTokenStorage = new RamTokenStorage();

    networkThread.start();

    httpClient.setRamTokenStorage(ramTokenStorage);
    httpClient.moveToThread(&networkThread);

    wsClient.setRamTokenStorage(ramTokenStorage);
    wsClient.moveToThread(&networkThread);

    // load chats from cache
}

MainWindow::~MainWindow()
{
    delete ui;
    delete ramTokenStorage;
    networkThread.quit();
    networkThread.wait();
}

void MainWindow::connectSignals()
{
    QObject::connect(&networkThread, &QThread::started, &httpClient, &HttpClient::checkRefreshToken);

    QObject::connect(&httpClient, &HttpClient::signProcessed, &wsClient, &WsClient::initialize);
    QObject::connect(&httpClient, &HttpClient::refreshTokenVerified, &wsClient, &WsClient::initialize);

    QObject::connect(&wsClient, &WsClient::unauthorized, this, &MainWindow::unauthorized);
    QObject::connect(&httpClient, &HttpClient::unauthorized, this, &MainWindow::unauthorized);

    QObject::connect(&wsClient, &WsClient::socketConnected, this, &MainWindow::wsConnected);
    QObject::connect(&wsClient, &WsClient::socketDisconnected, this, &MainWindow::wsDisconnected);
    QObject::connect(&wsClient, &WsClient::socketMessageReceived, this, &MainWindow::messageReceived);
    QObject::connect(this, &MainWindow::sendMessage, &wsClient, &WsClient::sendMessage);

    QObject::connect(this, &MainWindow::sign, &httpClient, &HttpClient::sign);
    QObject::connect(&httpClient, &HttpClient::signError, this, &MainWindow::signError);
    QObject::connect(&httpClient, &HttpClient::signProcessed, this, &MainWindow::signProcessed);
    QObject::connect(&httpClient, &HttpClient::shouldConfirmEmail, this, &MainWindow::shouldConfirmEmail);

    QObject::connect(&confirmEmailExpiredTimer, &QTimer::timeout, this, &MainWindow::confirmEmailExpired);

    QObject::connect(this, &MainWindow::getUserChats, &httpClient, &HttpClient::getUserChatsProxy);
    QObject::connect(this, &MainWindow::findChats, &httpClient, &HttpClient::findChatsProxy);
    QObject::connect(this, &MainWindow::getMessages, &httpClient, &HttpClient::getMessagesProxy);
    QObject::connect(this, &MainWindow::createGroup, &httpClient, &HttpClient::createGroupProxy);

    QObject::connect(&httpClient, &HttpClient::findChatsProcessed, this, &MainWindow::showChats);
    QObject::connect(&httpClient, &HttpClient::getUserChatsProcessed, this, &MainWindow::showChats);
    QObject::connect(&httpClient, &HttpClient::getMessagesProcessed, this, &MainWindow::showMessages);
    QObject::connect(&httpClient, &HttpClient::createGroupProcessed, this, &MainWindow::groupCreated);

    QObject::connect(&httpClient, &HttpClient::createGroupError, this, &MainWindow::createGroupError);

    QObject::connect(ui->messagesScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::messagesScrolled);
}

void MainWindow::fillPageNameToIndexMap()
{
    pageNameToIndexMap["signPage"] = 0;
    pageNameToIndexMap["signInFormPage"] = 1;
    pageNameToIndexMap["signUpFormPage"] = 2;
    pageNameToIndexMap["homePage"] = 3;
    pageNameToIndexMap["confirmEmailPage"] = 4;
    pageNameToIndexMap["createGroupPage"] = 5;
}

void MainWindow::clearChats() {
    QLayoutItem *item;
    while ((item = ui->chats->takeAt(0)) != nullptr) {
        QWidget *widget = item->widget();
        widget->setParent(nullptr);
    }
    delete item;
}

void MainWindow::clearMessages() {
    QLayoutItem* item;
    while ((item = ui->messages->takeAt(0))) {
        if (QWidget* widget = item->widget()) {
            widget->hide();
            delete widget;
        } else {
            delete item;
        }
    }
}

int MainWindow::calculateLineCount(const QString& text, const QFontMetrics& metrics, int labelWidth) {
   QStringList words = text.split(' ');
   int lineCount = 1;
   QString currentLine;
   for (const QString& word : words) {
       if (metrics.horizontalAdvance(currentLine + (currentLine.isEmpty() ? "" : " ") + word) > labelWidth) {
           ++lineCount;
           currentLine = word;
       } else {
           currentLine += (currentLine.isEmpty() ? "" : " ") + word;
       }
   }
   return lineCount;
}

void MainWindow::openChat() {
    clearMessages();
    atChat = true;
    isCurrentChatFullyLoaded = false;
    QObject *chatAsObj = sender();
    ChatPushButton *chat = dynamic_cast<ChatPushButton*>(chatAsObj); // In this and prev. line the nullptr is not expected
    qlonglong chatId = chat->getId();
    bool isChatGroup = chat->isGroup();
    currentChatId = chatId;
    currentChatName = chat->getName();
    isCurrentChatGroup = isChatGroup;
    QMap<QString, QString> body;
    body["chatId"] = QString::number(chatId);
    isChatGroup
            ? getMessages("messages/group", body)
            : getMessages("messages/dialogue", body); // I didn't use emit keyword to use the ternary operator
}

void MainWindow::showChats(QJsonObject data) {
    bool areChatsYours = data["your chats"].toBool();
    QString filter = data["filter"].toString();
    QJsonArray arr = data["chats"].toArray();
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject object = arr[i].toObject();
        qlonglong chatId = object["chatId"].toVariant().toLongLong();
        QString name = object["chatName"].toString();
        bool group = object["group"].toBool();
        QString lastMessage = object["lastMessageText"].toString();
        qlonglong lastMessageTimeAsLong = object["lastMessageTime"].toVariant().toLongLong();
        QDateTime lastMessageTimeAsQDT = (QDateTime::fromMSecsSinceEpoch(lastMessageTimeAsLong, Qt::UTC)).toLocalTime();
        QString lastMessageTime = lastMessageTimeAsQDT.toString("dd.MM.yy, hh:mm");
        int neededHeight = 60; // The button height, px. Will be changed to 130 if chat has any messages to display as the last message

        ChatPushButton *button = new ChatPushButton(chatId, name, group, this);

        QLabel *nameLabel = new QLabel(button);
        nameLabel->setStyleSheet("border: none;");
        nameLabel->setText(name);
        nameLabel->setGeometry(10, 10, 580, 40);

        QLabel *lastMessageLabel = new QLabel(button);
        lastMessageLabel->setObjectName("lastMessageLabel");
        lastMessageLabel->setStyleSheet("font: 14pt \"Segoe UI\";"
                                        "color: rgb(180, 180, 180);"
                                        "border: none;");
        lastMessageLabel->setGeometry(10, 60, 580, 60);

        if (!lastMessage.isEmpty()) {
            neededHeight = 130;
            QString lastMessageSender = object["lastMessageSenderName"].toString();
            QString lastMessageInfo = lastMessageSender + ": " + lastMessage.left(30);
            if (lastMessage.size() > 30) {
                lastMessageInfo += "...";
            }
            lastMessageInfo += "\n" + lastMessageTime;
            lastMessageLabel->setText(lastMessageInfo);
        }

        button->setStyleSheet("font: 16pt \"Segoe UI\";"
                                  "border-bottom: 1px solid rgb(180, 180, 180);");
        button->setFixedSize(580, neededHeight);
        QObject::connect(button, &QPushButton::pressed, this, &MainWindow::openChat);
        if (areChatsYours) {
            yourChats.append(button);
        }
        if (ui->findUserLineEdit->text() == filter) {
            ui->chats->addWidget(button);
        }
    }
}

 // I know there are some repeats of the prev. method code, but it'll be hard to make a common logic so maybe I'll do it later
void MainWindow::updateChatsWithMessage(qlonglong chatId, QString chatName, bool group, QString fullText) {
    bool alreadyAtThisChat = false;
    for (int i = 0; i < yourChats.length(); ++i) {
        ChatPushButton *current = yourChats.at(i);
        if (current->getId() == chatId) {
            alreadyAtThisChat = true;
            current->setFixedHeight(130);
            current->findChild<QLabel*>("lastMessageLabel")->setText(fullText);
            yourChats.push_back(current);
            yourChats.remove(i);
            if (ui->findUserLineEdit->text().isEmpty()) {
                ui->chats->removeWidget(current);
                ui->chats->insertWidget(0, current);
            }
            break;
        }
    }
    if (!alreadyAtThisChat) {
        ChatPushButton *button = new ChatPushButton(chatId, chatName, group, this);
        QLabel *nameLabel = new QLabel(button);
        nameLabel->setStyleSheet("border: none;");
        nameLabel->setText(chatName);
        nameLabel->setGeometry(10, 10, 580, 40);
        QLabel *lastMessageLabel = new QLabel(button);
        lastMessageLabel->setObjectName("lastMessageLabel");
        lastMessageLabel->setStyleSheet("font: 14pt \"Segoe UI\";"
                                        "color: rgb(180, 180, 180);"
                                        "border: none;");
        lastMessageLabel->setText(fullText);
        lastMessageLabel->setGeometry(10, 60, 580, 60);
        button->setStyleSheet("font: 16pt \"Segoe UI\";"
                              "border-bottom: 1px solid rgb(180, 180, 180);");
        button->setFixedSize(580, 130);
        QObject::connect(button, &QPushButton::pressed, this, &MainWindow::openChat);
        if (ui->findUserLineEdit->text().isEmpty()) {
            ui->chats->insertWidget(0, button);
        }
        yourChats.append(button);
    }
}

void MainWindow::groupCreated(QJsonObject data) {
    ui->groupNameLineEdit->clear();
    ui->createGroupWaitingLabel->movie()->stop();
    ui->createGroupWaitingLabel->hide();
    ui->createGroupButton->setEnabled(true);

    qlonglong id = data["id"].toVariant().toLongLong();
    QString name = data["name"].toString();
    ChatPushButton *button = new ChatPushButton(id, name, true, this);
    QLabel *nameLabel = new QLabel(button);
    QLabel *lastMessageLabel = new QLabel(button);
    lastMessageLabel->setObjectName("lastMessageLabel");
    lastMessageLabel->setStyleSheet("font: 14pt \"Segoe UI\";"
                                    "color: rgb(180, 180, 180);"
                                    "border: none;");
    lastMessageLabel->setGeometry(10, 60, 580, 60);
    nameLabel->setStyleSheet("border: none;");
    nameLabel->setText(name);
    nameLabel->setGeometry(10, 10, 580, 40);
    button->setStyleSheet("font: 16pt \"Segoe UI\";"
                          "border-bottom: 1px solid rgb(180, 180, 180);");
    button->setFixedSize(580, 60);
    QObject::connect(button, &QPushButton::pressed, this, &MainWindow::openChat);
    yourChats.append(button);
    if (ui->findUserLineEdit->text().isEmpty()) {
        ui->chats->addWidget(button);
    }
    on_createGroupGoBackButton_clicked(); // Return to the home page
}

void MainWindow::messagesScrolled(int value) {
    if (value > 300) {
        return ;
    }
    if (isCurrentChatFullyLoaded) {
        return ;
    }
    QLayoutItem *item = ui->messages->takeAt(0);
    if (item == nullptr) {
        return ;
    }
    QWidget *widget = item->widget();
    if (widget == nullptr) {
        return ;
    }
    MessageWidget *messageWidget = qobject_cast<MessageWidget*>(widget);
    if (messageWidget == nullptr) {
        return ;
    }
    QMap<QString, QString> body;
    body.insert("lastMessageId", QString::number(messageWidget->getId()));
    body.insert("chatId", QString::number(currentChatId));
    isCurrentChatGroup
            ? getMessages("messages/group", body)
            : getMessages("messages/dialogue", body);
    ui->messagesScrollArea->verticalScrollBar()->setEnabled(false);
}

void MainWindow::showMessages(QJsonObject data) {
    qlonglong chatId = data["chatId"].toVariant().toLongLong();
    bool fromGroup = data["group"].toBool();
    bool justOpened = data["just opened"].toBool();
    QJsonArray arr = data["chats"].toArray();
    if (chatId != currentChatId || fromGroup != isCurrentChatGroup) {
        return ; // The user came to the other chat, we don't need these messages now
    }

    ui->currentChatName->setText(currentChatName);
    int initialHeightOfMessages = ui->messagesContainer->height();
    if (arr.size() < 20) {
        isCurrentChatFullyLoaded = true;
    }

    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject object = arr[i].toObject();
        addMessage(object);
    }
    if (justOpened) {
        QTimer::singleShot(100, this, [=]() {
            QScrollBar *bar = ui->messagesScrollArea->verticalScrollBar();
            bar->setValue(bar->maximum());
        });
    }
    else {
        QTimer::singleShot(100, this, [=]() {
            int finalHeightOfMessages = ui->messagesContainer->height();
            QScrollBar *bar = ui->messagesScrollArea->verticalScrollBar();
            bar->setValue(bar->value() + (finalHeightOfMessages - initialHeightOfMessages) + bar->singleStep());
        });
    }
    ui->messagesScrollArea->verticalScrollBar()->setEnabled(true);
}

void MainWindow::addMessage(QJsonObject data) {
    qlonglong id = data["id"].toVariant().toLongLong();
    QString text = data["text"].toString();
    QString sender = data["senderName"].toString();
    bool areYouSender = (sender == "You");
    qlonglong timeAsLong = data["time"].toVariant().toLongLong();
    QDateTime timeAsQDT = (QDateTime::fromMSecsSinceEpoch(timeAsLong, Qt::UTC)).toLocalTime();
    QString time = timeAsQDT.toString("dd.MM.yy, hh:mm");

    QString backgroundColor = areYouSender
                                  ? "background-color: rgb(62, 105, 120);"
                                  : "background-color: rgb(61, 59, 61);";
    QFont littleFont = basicFont;
    littleFont.setPointSize(12);
    QFont mediumFont = basicFont;
    mediumFont.setPointSize(14);
    QFontMetrics littleMetrics(littleFont);
    QFontMetrics mediumMetrics(littleFont);
    QFontMetrics messageMetrics(basicFont);

    int nameLabelWidth = mediumMetrics.horizontalAdvance(sender);
    int messageLabelWidth = messageMetrics.horizontalAdvance(text);
    int timeLabelWidth = littleMetrics.horizontalAdvance(time);
    int biggestElementWidth = max({messageLabelWidth, timeLabelWidth, nameLabelWidth}) + 20;
    int containerWidth = min(800, biggestElementWidth);

    MessageWidget *finalContainer = new MessageWidget(id);
    finalContainer->setFixedWidth(containerWidth);
    finalContainer->setStyleSheet("font: 16pt \"Segoe UI\";"
                                  "border-radius: 10px;"
                                  + backgroundColor);

    QVBoxLayout *layout = new QVBoxLayout(finalContainer);
    layout->setSpacing(0);
    layout->setContentsMargins(5, 3, 15, 3);

    QLabel *nameLabel = new QLabel();
    nameLabel->setText(sender);
    nameLabel->setStyleSheet("font: 14pt \"Segoe UI\"; color: rgb(188, 188, 225);");

    QLabel *messageLabel = new QLabel();
    messageLabel->setWordWrap(true);
    messageLabel->setText(text);
    messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QLabel *timeLabel = new QLabel();
    timeLabel->setText(time);
    timeLabel->setStyleSheet("font: 12pt \"Segoe UI\"; color: rgb(180, 180, 180);");

    layout->addWidget(nameLabel);
    layout->addWidget(messageLabel);
    layout->addWidget(timeLabel);

    data["add to the end"].toBool()
            ? ui->messages->addWidget(finalContainer)
            : ui->messages->insertWidget(0, finalContainer);

}

void MainWindow::on_findUserLineEdit_textEdited(const QString &arg1)
{
    clearChats();
    if (arg1.isEmpty()) {
        for (int i = 0; i < yourChats.size(); ++i) {
            ChatPushButton* button = yourChats[i];
            ui->chats->addWidget(button);
        }
        return ;
    }
    QMap<QString, QString> body;
    body["filter"] = arg1;
    emit findChats(body);
}

void MainWindow::on_sendMessageButton_clicked()
{
    QString text = ui->messageLineEdit->text();
    if (!atChat || text.isEmpty()) {
        return ;
    }
    ui->messageLineEdit->clear();
    QDateTime utcDateTime = QDateTime::currentDateTimeUtc();
    qlonglong msecs = utcDateTime.toMSecsSinceEpoch();
    qlonglong tempId = static_cast<qlonglong>(QRandomGenerator::global()->generate64());
    QJsonObject data;
    data["method"] = "create";
    data["time"] = msecs;
    data["text"] = text;
    data["chatId"] = currentChatId;
    data["toGroup"] = isCurrentChatGroup;
    data["tempId"] = QString::number(tempId); // A string
    emit sendMessage(data);
    data.remove("tempId");
    data["id"] = tempId; // A qlonglong
    data["senderName"] = "You";
    data["add to the end"] = true;
    addMessage(data);
    QTimer::singleShot(100, this, [=]() {
        QScrollBar *bar = ui->messagesScrollArea->verticalScrollBar();
        bar->setValue(bar->maximum());
    });
    if (text.size() > 30) {
        text = text.left(30);
        text.append("...");
    }
    QString time = QDateTime::currentDateTime().toString("dd.MM.yy, hh:mm");
    QString newChatText = "You: " + text + '\n' + time;
    updateChatsWithMessage(currentChatId, currentChatName, isCurrentChatGroup, newChatText);
}

void MainWindow::on_messageLineEdit_returnPressed()
{
    on_sendMessageButton_clicked();
}

void MainWindow::messageReceived(QJsonObject data) {
    qlonglong timeAsLong = data["time"].toVariant().toLongLong();
    QDateTime timeAsQDT = (QDateTime::fromMSecsSinceEpoch(timeAsLong, Qt::UTC)).toLocalTime();
    QString time = timeAsQDT.toString("dd.MM.yy, hh:mm");
    QString text = data["text"].toString();
    if (text.size() > 30) {
        text = text.left(30);
        text.append("...");
    }
    bool toGroup = data["toGroup"].toBool();
    qlonglong chatId = data["chatId"].toVariant().toLongLong();
    QString chatName = data["chatName"].toString();
    QString newChatText = data["senderName"].toString() + ": " + text + '\n' + time;
    updateChatsWithMessage(chatId, chatName, toGroup, newChatText);

    if (toGroup != isCurrentChatGroup || chatId != currentChatId) { // The user is in the other chat
        // create a notification here
        return ;
    }
    data["add to the end"] = true;
    addMessage(data);
    QTimer::singleShot(100, this, [=]() {
        QScrollBar *bar = ui->messagesScrollArea->verticalScrollBar();
        bar->setValue(bar->maximum());
    });
}

void MainWindow::on_createGroupButton_clicked()
{
    QString groupName = ui->groupNameLineEdit->text();
    QString warning;
    if (groupName.isEmpty()) {
        warning = "Empty name!";
    }
    if (groupName.length() > 16) {
        warning = "Too long name!";
    }
    if (!warning.isEmpty()) {
        displayCreateGroupWarning(warning);
        return ;
    }
    ui->createGroupButton->setEnabled(false);
    ui->createGroupWaitingLabel->show();
    ui->createGroupWaitingLabel->movie()->start();
    QMap<QString, QString> body;
    body.insert("name", groupName);
    emit createGroup(body);
}


void MainWindow::displayCreateGroupWarning(QString warning)
{
    QFontMetrics metrics(basicFont);
    int linesForWarning = calculateLineCount(warning, metrics, 320);
    ui->createGroupWrap->setGeometry(800, 405 - (20 * linesForWarning), 320, 200 + (40 * linesForWarning));
    ui->createGroupWaitingLabel->move(860, 205 - (20 * linesForWarning));
    ui->createGroupWarning->setText(warning);
}

void MainWindow::createGroupError(QString error) {
    ui->createGroupButton->setEnabled(true);
    ui->createGroupWaitingLabel->movie()->stop();
    ui->createGroupWaitingLabel->hide();
    displayCreateGroupWarning(error);
}

void MainWindow::on_createGroupGoBackButton_clicked()
{
    int index = pageNameToIndexMap["homePage"];
    ui->stackedWidget->setCurrentIndex(index);
}

void MainWindow::wsConnected() {
    ui->connectionStatement->setText("Connected!");
}

void MainWindow::wsDisconnected() {
    ui->connectionStatement->setText("Connecting...");
}

void MainWindow::signProcessed() {
    int homePageIndex = pageNameToIndexMap["homePage"];
    ui->stackedWidget->setCurrentIndex(homePageIndex);
}

void MainWindow::shouldConfirmEmail()
{
    if (authType == AuthorizationType::Login) {
        ui->signInWaitingLabel->movie()->stop();
        ui->signInWaitingLabel->hide();
        ui->signInFormWrap->setGeometry(810, 430, 300, 200);
        ui->signInWaitingLabel->move(860, 230);
        ui->signInButton->setEnabled(true);
    }
    if (authType == AuthorizationType::Registration) {
        ui->signUpWaitingLabel->movie()->stop();
        ui->signUpWaitingLabel->hide();
        ui->signUpFormWrap->setGeometry(810, 405, 300, 250);
        ui->signUpWaitingLabel->move(860, 200);
        ui->signUpButton->setEnabled(true);
    }
    confirmEmailExpiredTimer.start();
    int confirmEmailPageIndex = pageNameToIndexMap["confirmEmailPage"];
    ui->stackedWidget->setCurrentIndex(confirmEmailPageIndex);
}

void MainWindow::confirmEmailExpired()
{
    QString warning = "You didn't confirm your email!";
    if (authType == AuthorizationType::Login) {
        int signInFormPageIndex = pageNameToIndexMap["signInFormPage"];
        ui->stackedWidget->setCurrentIndex(signInFormPageIndex);
        displaySignInWarning(warning);
    }
    if (authType == AuthorizationType::Registration) {
        int signUpFormPageIndex = pageNameToIndexMap["signUpFormPage"];
        ui->stackedWidget->setCurrentIndex(signUpFormPageIndex);
        displaySignUpWarning(warning);
    }
}

void MainWindow::displaySignInWarning(QString warning)
{
    QFontMetrics metrics(basicFont);
    int linesForWarning = calculateLineCount(warning, metrics, 300);
    ui->signInFormWrap->setGeometry(810, 432 - (20 * linesForWarning), 300, 205 + (40 * linesForWarning));
    ui->signInWaitingLabel->move(860, 230 - (20 * linesForWarning));
    ui->signInWarning->setText(warning);
}

void MainWindow::displaySignUpWarning(QString warning)
{
    QFontMetrics metrics(basicFont);
    int linesForWarning = calculateLineCount(warning, metrics, 300);
    ui->signUpFormWrap->setGeometry(810, 402 - (20 * linesForWarning), 300, 255 + (40 * linesForWarning));
    ui->signUpWaitingLabel->move(860, 200 - (20 * linesForWarning));
    ui->signUpWarning->setText(warning);
}

void MainWindow::unauthorized() {
    int currentIndex = ui->stackedWidget->currentIndex();
    int signPageIndex = pageNameToIndexMap["signPage"];
    int signInFormPageIndex = pageNameToIndexMap["signInFormPage"];
    int signUpFormPageIndex = pageNameToIndexMap["signUpFormPage"];
    if (currentIndex != signPageIndex && currentIndex != signInFormPageIndex && currentIndex != signUpFormPageIndex) {
        ui->stackedWidget->setCurrentIndex(signPageIndex);
    }
}

void MainWindow::signError(QString error) {
    if (authType == AuthorizationType::Login) {
        ui->signInWaitingLabel->movie()->stop();
        ui->signInWaitingLabel->hide();
        ui->signInButton->setEnabled(true);
        displaySignInWarning(error);
    }
    if (authType == AuthorizationType::Registration) {
        ui->signUpWaitingLabel->movie()->stop();
        ui->signUpWaitingLabel->hide();
        ui->signUpButton->setEnabled(true);
        displaySignUpWarning(error);
    }
}

void MainWindow::on_goToSignInButton_clicked()
{
    int signInFormPageIndex = pageNameToIndexMap["signInFormPage"];
    ui->stackedWidget->setCurrentIndex(signInFormPageIndex);
}

void MainWindow::on_goToSignUpButton_clicked()
{
    int signUpFormPageIndex = pageNameToIndexMap["signUpFormPage"];
    ui->stackedWidget->setCurrentIndex(signUpFormPageIndex);
}

void MainWindow::on_signInButton_clicked()
{
    QString email = ui->signInEmailLineEdit->text();
    QString password = ui->signInPasswordLineEdit->text();

    if (email.isEmpty() || password.isEmpty()) {
        QString warning = "Fields are empty!";
        displaySignInWarning(warning);
        return;
    }

    authType = AuthorizationType::Login;

    ui->signInWaitingLabel->show();
    ui->signInWaitingLabel->movie()->start();
    ui->signInButton->setEnabled(false);

    QMap<QString, QString> requestBody;
    requestBody["email"] = email;
    requestBody["password"] = password;
    emit sign("sign/in", requestBody);
}

void MainWindow::on_signUpButton_clicked()
{
    QString warning;
    QString name = ui->signUpUsernameLineEdit->text();
    QString email = ui->signUpEmailLineEdit->text();
    QString password = ui->signUpPasswordLineEdit->text();

    if (name.isEmpty() || email.isEmpty() || password.isEmpty()) {
        warning = "Fields are empty!";
    }
    if (name.length() > 16) {
        warning = "Too long name!";
    }
    if (name == "You") {
        warning = "Sorry, this name is reserved!";
    }
    if (!warning.isEmpty()) {
        displaySignUpWarning(warning);
        return;
    }

    authType = AuthorizationType::Registration;

    ui->signUpWaitingLabel->show();
    ui->signUpWaitingLabel->movie()->start();
    ui->signUpButton->setEnabled(false);

    QMap<QString, QString> requestBody;
    requestBody["name"] = name;
    requestBody["email"] = email;
    requestBody["password"] = password;
    emit sign("sign/up", requestBody);
}

void MainWindow::on_backToSignButton_clicked()
{
    confirmEmailExpiredTimer.stop();
    if (authType == AuthorizationType::Login) {
        on_goToSignInButton_clicked(); // Redirect to log in page
    }
    if (authType == AuthorizationType::Registration) {
        on_goToSignUpButton_clicked(); // Redirect to sign up page
    }
}

void MainWindow::on_fromSignInToSignUpButton_clicked()
{
    int index = pageNameToIndexMap["signUpFormPage"];
    ui->stackedWidget->setCurrentIndex(index);
}

void MainWindow::on_fromSignUpToSignInButton_clicked()
{
    int index = pageNameToIndexMap["signInFormPage"];
    ui->stackedWidget->setCurrentIndex(index);
}

void MainWindow::on_goToCreateGroupPageButton_clicked()
{
    int index = pageNameToIndexMap["createGroupPage"];
    ui->stackedWidget->setCurrentIndex(index);
}
