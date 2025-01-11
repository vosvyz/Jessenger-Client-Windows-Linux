#include <mainwindow.h>
#include <./ui_mainwindow.h>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Properties initialization
    ui->setupUi(this);
    ui->chatsContainer->setLayout(ui->chats);
    ui->messagesContainer->setLayout(ui->messages);
    stackedWidget = ui->stackedWidget;
    ui->chats->setAlignment(Qt::AlignTop);
    ui->messages->setAlignment(Qt::AlignTop);
    networkClient = new NetworkClient();
    fullyLoadedChats = new QVector<qlonglong>();
    networkClientThread = new QThread();
    yourChats = new QVector<UserPushButton*>();
    // Connecting signals and slots (the call must necessarily occur before starting the thread below, since for the correct launch of the application, the signal networkClientThread->started() must be triggered)
    connectSignals();
    networkClient->moveToThread(networkClientThread);
    networkClientThread->start();
    qApp->setStyleSheet("QLabel {selection-background-color: rgb(46, 112, 165);}");
}

void MainWindow::connectSignals() {
    QObject::connect(this, &MainWindow::createGroup, networkClient, NetworkClient::createGroup);
    QObject::connect(this, &MainWindow::findChats, networkClient, NetworkClient::findChats);
    QObject::connect(this, &MainWindow::getDialogueMessages, networkClient, NetworkClient::getDialogueMessages);
    QObject::connect(this, &MainWindow::getGroupMessages, networkClient, NetworkClient::getGroupMessages);
    QObject::connect(this, &MainWindow::getYourChats, networkClient, NetworkClient::getYourChats);
    QObject::connect(this, &MainWindow::deleteNetworkClient, networkClient, &NetworkClient::deleteThis);
    QObject::connect(this, &MainWindow::checkRefreshToken, networkClient, &NetworkClient::checkRefreshToken);
    QObject::connect(this, &MainWindow::sign, networkClient, &NetworkClient::sign);
    QObject::connect(this, &MainWindow::sendMessage, networkClient, &NetworkClient::sendMessage);
    QObject::connect(this, &MainWindow::connectWebSocket, networkClient, &NetworkClient::connectWebSocket);
    QObject::connect(networkClient, &NetworkClient::initialized, this, &MainWindow::start);
    QObject::connect(networkClient, &NetworkClient::httpSignError, this, &MainWindow::httpSignError);
    QObject::connect(networkClient, &NetworkClient::unauthorizedSignal, this, &MainWindow::unauthorizedSignal);
    QObject::connect(networkClient, &NetworkClient::createGroupError, this, &MainWindow::createGroupError);
    QObject::connect(networkClient, &NetworkClient::createGroupProcessed, this, &MainWindow::createGroupProcessed);
    QObject::connect(networkClient, &NetworkClient::findChatsProcessed, this, &MainWindow::findChatsProcessed);
    QObject::connect(networkClient, &NetworkClient::getDialogueMessagesProcessed, this, &MainWindow::getDialogueMessagesProcessed);
    QObject::connect(networkClient, &NetworkClient::getGroupMessagesProcessed, this, &MainWindow::getGroupMessagesProcessed);
    QObject::connect(networkClient, &NetworkClient::getYourChatsProcessed, this, &MainWindow::getYourChatsProcessed);
    QObject::connect(networkClient, &NetworkClient::httpSignProcessed, this, &MainWindow::httpSignProcessed);
    QObject::connect(networkClient, &NetworkClient::webSocketMessageReceived, this, &MainWindow::messageReceived);
    QObject::connect(networkClient, &NetworkClient::webSocketConnectedSignal, this, &MainWindow::socketConnected);
    QObject::connect(networkClient, &NetworkClient::webSocketDisconnectedSignal, this, &MainWindow::socketDisconnected);
    QObject::connect(networkClientThread, &QThread::started, networkClient, &NetworkClient::initialize);
    QObject::connect(ui->messagesScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::messagesScrolled);
}

MainWindow::~MainWindow()
{
    delete ui;
    emit deleteNetworkClient();
    networkClientThread->wait();
    delete networkClientThread;
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

void MainWindow::messagesScrolled(int value) {
    if (value > 300) {
        return ;
    }
    if (fullyLoadedChats->contains(currentChatId)) {
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
    if (isCurrentChatGroup) {
        body.insert("groupId", QString::number(currentChatId));
        emit getGroupMessages(body);
    }
    else {
        body.insert("otherId", QString::number(currentChatId));
        emit getDialogueMessages(body);
    }
    ui->messagesScrollArea->verticalScrollBar()->setEnabled(false);
}

void MainWindow::configureChatButton(UserPushButton *button, int neededHeight) {
    button->setStyleSheet("font: 16pt \"Segoe UI\";"
                          "border-bottom: 1px solid rgb(180, 180, 180);");
    button->setFixedSize(580, neededHeight);
    QObject::connect(button, &UserPushButton::pressed, this, &MainWindow::openChatSlot);
}

void MainWindow::setHomePage() {
    setPageByName("homePage");
    emit getYourChats();
}

void MainWindow::setPageByName(const QString& pageName) {
    for (int i = 0; i < stackedWidget->count(); ++i) {
        QWidget* page = stackedWidget->widget(i);
        if (page->objectName() == pageName) {
            stackedWidget->setCurrentWidget(page);
        }
    }
}

void MainWindow::start() {
    emit checkRefreshToken();
}

// Triggered when opening a chat upon clicking the UserPushButton
void MainWindow::openChatSlot() {
    QObject *senderObject = sender();
    UserPushButton *button = qobject_cast<UserPushButton*>(senderObject);
    if (button->getName() == currentChatName) {
        return ;
    }
    QMap<QString, QString> body;
    isCurrentChatGroup = button->isGroup();
    currentChatId = button->getId();
    currentChatName = button->getName();
    ui->sendMessageButton->setEnabled(true);
    ui->currentChat->setText(currentChatName);
    if (button->isGroup()) {
        body.insert("groupId", QString::number(button->getId()));
        emit getGroupMessages(body);
    }
    else {
        body.insert("otherId", QString::number(button->getId()));
        emit getDialogueMessages(body);
    }
    clearMessages();
}

void MainWindow::socketDisconnected() {
    ui->connectionStatement->setText("Connecting...");
}

void MainWindow::socketConnected() {
    ui->connectionStatement->setText("You are online!");
}

void MainWindow::httpSignError(QString requestPath, QString error) {
    if (requestPath == "sign/in") {
        ui->signInFormWrap->setGeometry(810, 410, 300, 260);
        ui->signInWarning->setText(error);
    }
    if (requestPath == "sign/up") {
        ui->signUpFormWrap->setGeometry(800, 385, 320, 310);
        ui->signUpWarning->setText(error);
    }
}

void MainWindow::unauthorizedSignal() {
    setPageByName("signPage");
}

void MainWindow::createGroupProcessed(QJsonObject object) {
    QString name = object["name"].toString();
    UserPushButton *button = new UserPushButton(object["id"].toVariant().toLongLong(), object["name"].toString(), true);
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
    configureChatButton(button, 60);
    yourChats->append(button);
    if (ui->findUserLineEdit->text().isEmpty()) {
        ui->chats->addWidget(button);
    }
    setPageByName("homePage");
}

void MainWindow::createGroupError(QString error) {
    ui->createGroupWrap->setGeometry(800, 410, 320, 260);
    ui->createGroupLabel->setText(error);
}

void MainWindow::findChatsProcessed(QJsonArray result) {
    if (!ui->findUserLineEdit->text().isEmpty()) {
        for (int i = 0; i < result.size(); ++i) {
            QJsonObject object = result[i].toObject();
            qlonglong id = object["id"].toVariant().toLongLong();
            QString name = object["name"].toString();
            bool isGroup = object["group"].toBool();
            UserPushButton *button = new UserPushButton(id, name, isGroup, this);
            configureChatButton(button, 40);
            button->setText(name);
            ui->chats->addWidget(button);
        }
    }
}

void MainWindow::getYourChatsProcessed(QJsonArray result) {
    for (int i = 0; i < result.size(); ++i) {
        int neededHeight = 60;
        QJsonObject object = result[i].toObject();
        QDateTime correctTime = (QDateTime::fromMSecsSinceEpoch(object["lastMessageTime"].toVariant().toLongLong(), Qt::UTC)).toLocalTime();
        QString lastMessage = object["lastMessageText"].toString();
        QString name = object["chatName"].toString();
        UserPushButton *button = new UserPushButton(object["chatId"].toVariant().toLongLong(), name, object["group"].toBool(), this);
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
        if (!lastMessage.isEmpty()) {
            QString lastMessageInfo = object["lastMessageSenderName"].toString() + ": " + lastMessage.left(30);
            if (lastMessage.size() > 30) {
                lastMessageInfo += "...";
            }
            lastMessageInfo += "\n" + correctTime.toString("dd.MM.yy, hh:mm");
            lastMessageLabel->setText(lastMessageInfo);
            neededHeight = 130;
        }
        configureChatButton(button, neededHeight);
        yourChats->append(button);
        if (ui->findUserLineEdit->text().isEmpty()) {
            ui->chats->addWidget(button);
        }
    }
}

void MainWindow::httpSignProcessed() {
    setHomePage();
    emit connectWebSocket();
}

void MainWindow::getDialogueMessagesProcessed(QJsonArray result, qlonglong chatId, bool shouldScrollDown) {
    int initialHeightOfMessages = ui->messagesContainer->height();
    if (result.size() < 20) {
        // Caching logic - TODO. I know that after this code some things can go wrong, but caching should fix it
        fullyLoadedChats->append(chatId);
    }
    if (chatId != currentChatId || isCurrentChatGroup) {
        return ;
    }
    for (int i = 0; i < result.size(); ++i) {
        QJsonObject object = result[i].toObject();
        QString sender = object["senderName"].toString();
        bool areYouSender = (sender == "You");
        QString backgroundColor = areYouSender
                                      ? "background-color: rgb(62, 105, 120);"
                                      : "background-color: rgb(61, 59, 61);";
        QString correctTime = (QDateTime::fromMSecsSinceEpoch(object["time"].toVariant().toLongLong(), Qt::UTC)).toLocalTime().toString("dd.MM.yy, hh:mm");
        MessageWidget *finalContainer = new MessageWidget(object["id"].toVariant().toLongLong());
        finalContainer->setMaximumWidth(800);
        finalContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        finalContainer->setStyleSheet("font: 16pt \"Segoe UI\";"
                                      "border-radius: 3px;"
                                      + backgroundColor);
        QVBoxLayout *layout = new QVBoxLayout(finalContainer);
        layout->setContentsMargins(5, 5, 5, 5);
        QLabel *messageLabel = createStyledLabel(object["text"].toString(), "");
        messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(messageLabel);
        QLabel *timeLabel = createStyledLabel(correctTime,
                                              "font: 14pt \"Segoe UI\"; color: rgb(180, 180, 180);");
        layout->addWidget(timeLabel);
        QFontMetrics metrics(timeLabel->font());
        int timeLabelWidth = metrics.horizontalAdvance(timeLabel->text());
        finalContainer->setMinimumWidth(qMax(300, timeLabelWidth));
        ui->messages->insertWidget(0, finalContainer);
    }
    if (shouldScrollDown) {
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

void MainWindow::getGroupMessagesProcessed(QJsonArray result, qlonglong chatId, bool shouldScrollDown) {
    int initialHeightOfMessages = ui->messagesContainer->height();
    if (result.size() < 20) {
        // Caching logic - TODO. I know that after this code some things can go wrong, but caching should fix it
        fullyLoadedChats->append(chatId);
    }
    if (chatId != currentChatId || !isCurrentChatGroup) {
        return ;
    }
    for (int i = 0; i < result.size(); ++i) {
        QJsonObject object = result[i].toObject();
        QString sender = object["senderName"].toString();
        bool areYouSender = (sender == "You");
        QString message = object["text"].toString();
        QString correctTime = (QDateTime::fromMSecsSinceEpoch(object["time"].toVariant().toLongLong(), Qt::UTC)).toLocalTime().toString("dd.MM.yy, hh:mm");
        QString backgroundColor = (areYouSender)
                                      ? "background-color: rgb(62, 105, 120);"
                                      : "background-color: rgb(61, 59, 61);";
        MessageWidget *finalContainer = new MessageWidget(object["id"].toVariant().toLongLong());
        finalContainer->setMaximumWidth(800);
        finalContainer->setStyleSheet("font: 16pt \"Segoe UI\";"
                                      "border-radius: 3px;"
                                      + backgroundColor);
        finalContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QVBoxLayout *layout = new QVBoxLayout(finalContainer);
        layout->setContentsMargins(5, 5, 5, 5);
        QLabel *messageLabel = createStyledLabel(message, "");
        QLabel *timeLabel = createStyledLabel(correctTime,
                                              "font: 14pt \"Segoe UI\"; color: rgb(180, 180, 180);");
        messageLabel->setMaximumWidth(800);
        messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        if (!areYouSender) {
            QLabel *senderNameLabel = createStyledLabel(sender, "color: rgb(51, 124, 192); font: 14pt \"Segoe UI\";");
            layout->addWidget(senderNameLabel);
        }
        layout->addWidget(messageLabel);
        layout->addWidget(timeLabel);
        ui->messages->insertWidget(0, finalContainer);
    }
    if (shouldScrollDown) {
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

QLabel* MainWindow::createStyledLabel(const QString &text, const QString &style) {
    QLabel *label = new QLabel();
    label->setText(text);
    label->setStyleSheet(style);
    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return label;
}

// Receiving message by WS-protocol
void MainWindow::messageReceived(QJsonObject data) {
    QDateTime correctTime = QDateTime::currentDateTime();
    QString timeAsString = correctTime.toString("dd.MM.yy, hh:mm");
    bool toGroup = data["toGroup"].toBool();
    QString senderName = data["senderName"].toString();
    QString initialText = data["messageText"].toString();
    QString chatText; // < Contains text for displaying the message as the last one in the QVBoxLayout with chats
    QString identifyBy; // < Contains a parameter (username/group name) by which the chat needs to be identified, from which the message came, in order to update it in ui->chats
    if (!toGroup) {
        identifyBy = senderName;
        if (senderName == currentChatName || (senderName == "You" && data["otherId"] == currentChatId && !isCurrentChatGroup)) {
            QWidget *finalContainer = new QWidget();
            QString backgroundColor = (senderName == "You")
                                          ? "background-color: rgb(62, 105, 120);"
                                          : "background-color: rgb(61, 59, 61);";
            finalContainer->setMaximumWidth(800);
            finalContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            finalContainer->setStyleSheet("font: 16pt \"Segoe UI\";"
                                          "border-radius: 3px;"
                                          + backgroundColor);
            QVBoxLayout *layout = new QVBoxLayout(finalContainer);
            layout->setContentsMargins(5, 5, 5, 5);
            QLabel *messageLabel = createStyledLabel(initialText, "");
            messageLabel->setMaximumWidth(800);
            messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            layout->addWidget(messageLabel);
            QLabel *timeLabel = createStyledLabel(timeAsString,
                                                  "font: 14pt \"Segoe UI\"; color: rgb(180, 180, 180);");
            layout->addWidget(timeLabel);
            QFontMetrics metrics(timeLabel->font());
            int timeLabelWidth = metrics.horizontalAdvance(timeLabel->text());
            finalContainer->setMinimumWidth(qMax(300, timeLabelWidth));
            ui->messages->addWidget(finalContainer);
        }
        else {
            // Create a notification
        }
        chatText = senderName + ": " + initialText.left(30);
        if (initialText.size() > 30) {
            chatText += "...";
        }
        chatText += "\n" + timeAsString;
    }
    else {
        QString groupName = data["groupName"].toString();
        identifyBy = groupName;
        if (groupName == currentChatName) {
            QWidget *finalContainer = new QWidget();
            QString backgroundColor = (senderName == "You")
                                          ? "background-color: rgb(62, 105, 120);"
                                          : "background-color: rgb(61, 59, 61);";
            finalContainer->setMaximumWidth(800);
            finalContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            finalContainer->setStyleSheet("font: 16pt \"Segoe UI\";"
                                          "border-radius: 3px;"
                                          + backgroundColor);
            QVBoxLayout *layout = new QVBoxLayout(finalContainer);
            layout->setContentsMargins(5, 5, 5, 5);
            QLabel *messageLabel = createStyledLabel(initialText, "");
            messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            QLabel *timeLabel = createStyledLabel(timeAsString,
                                                  "font: 14pt \"Segoe UI\"; color: rgb(180, 180, 180);");
            messageLabel->setMaximumWidth(800);
            if (senderName != "You") {
                QLabel *senderNameLabel = createStyledLabel(senderName, "color: rgb(51, 124, 192); font: 14pt \"Segoe UI\";");
                layout->addWidget(senderNameLabel);
            }
            layout->addWidget(messageLabel);
            layout->addWidget(timeLabel);
            ui->messages->addWidget(finalContainer);
        }
        else {
            // Create a notification
        }
        chatText = senderName + ": " + initialText.left(30);
        if (initialText.size() > 30) {
            chatText += "...";
        }
        chatText += "\n" + timeAsString;
    }
    if (!ui->findUserLineEdit->text().isEmpty()) {
        return ;
    }
    bool alreadyAtThisChat = false;
    for (int i = 0; i < yourChats->length(); ++i) {
        UserPushButton *current = yourChats->at(i);
        if (current->getName() == identifyBy) {
            alreadyAtThisChat = true;
            current->setFixedHeight(130);
            current->findChild<QLabel*>("lastMessageLabel")->setText(chatText);
            yourChats->push_back(current);
            yourChats->remove(i);
            if (ui->findUserLineEdit->text().isEmpty()) {
                ui->chats->removeWidget(current);
                ui->chats->insertWidget(0, current);
            }
            break;
        }
    }
    if (!alreadyAtThisChat) {
        UserPushButton *button = new UserPushButton(currentChatId, currentChatName, isCurrentChatGroup, this);
        QLabel *nameLabel = new QLabel(button);
        nameLabel->setStyleSheet("border: none;");
        nameLabel->setText(currentChatName);
        nameLabel->setGeometry(10, 10, 580, 40);
        QLabel *lastMessageLabel = new QLabel(button);
        lastMessageLabel->setObjectName("lastMessageLabel");
        lastMessageLabel->setStyleSheet("font: 14pt \"Segoe UI\";"
                                        "color: rgb(180, 180, 180);"
                                        "border: none;");
        lastMessageLabel->setText(chatText);
        lastMessageLabel->setGeometry(10, 60, 580, 60);
        configureChatButton(button, 130);
        if (ui->findUserLineEdit->text().isEmpty()) {
            ui->chats->insertWidget(0, button);
        }
        yourChats->append(button);
    }
}

// Sending a message to a dialog/group. Sent via WebSocket rather than HTTP due to the server architecture.
void MainWindow::on_sendMessageButton_clicked()
{
    QString text = ui->messageLineEdit->text();
    if (text.isEmpty()) {
        return ;
    }
    ui->messageLineEdit->clear();
    QDateTime correctTime = QDateTime::currentDateTime();
    QString timeAsString = correctTime.toString("dd.MM.yy, hh:mm");
    QWidget *finalContainer = new QWidget();
    finalContainer->setMaximumWidth(800);
    finalContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    finalContainer->setStyleSheet("font: 16pt \"Segoe UI\";"
                                  "border-radius: 3px;"
                                  "background-color: rgb(62, 105, 120);");
    QVBoxLayout *layout = new QVBoxLayout(finalContainer);
    layout->setContentsMargins(5, 5, 5, 5);
    QLabel *messageLabel = createStyledLabel(text, "");
    messageLabel->setMaximumWidth(800);
    messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(messageLabel);
    QLabel *timeLabel = createStyledLabel(timeAsString,
                                          "font: 14pt \"Segoe UI\"; color: rgb(180, 180, 180);");
    layout->addWidget(timeLabel);
    QFontMetrics metrics(timeLabel->font());
    int timeLabelWidth = metrics.horizontalAdvance(timeLabel->text());
    finalContainer->setMinimumWidth(qMax(300, timeLabelWidth));
    ui->messages->addWidget(finalContainer);
    QString message = "{\"method\": \"create\", "; // < Message to the server
    if (!isCurrentChatGroup) {
        message += "\"toGroup\": false, ";
    }
    else {
        message += "\"toGroup\": true, ";
    }
    message += "\"chatId\": " + QString::number(currentChatId) + ", \"messageText\": \"" + text + "\", "; // Do not close the curly brace, networkManager should attach a temporary ID to the message
    emit sendMessage(message);
    QTimer::singleShot(100, this, [=]() {
        QScrollBar *bar = ui->messagesScrollArea->verticalScrollBar();
        bar->setValue(bar->maximum());
    });
    QString chatText = "You: " + text.left(30); // < for displaying to oneself on the QVBoxLayout with chats
    if (text.size() > 30) {
        chatText += "...";
    }
    chatText += "\n" + timeAsString;
    bool alreadyAtThisChat = false;
    for (int i = 0; i < yourChats->length(); ++i) {
        UserPushButton *current = yourChats->at(i);
        if (current->getName() == currentChatName) {
            alreadyAtThisChat = true;
            current->setFixedHeight(130);
            current->findChild<QLabel*>("lastMessageLabel")->setText(chatText);
            yourChats->push_back(current);
            yourChats->remove(i);
            if (ui->findUserLineEdit->text().isEmpty()) {
                ui->chats->removeWidget(current);
                ui->chats->insertWidget(0, current);
            }
            break;
        }
    }
    if (!alreadyAtThisChat) {
        UserPushButton *button = new UserPushButton(currentChatId, currentChatName, isCurrentChatGroup, this);
        QLabel *nameLabel = new QLabel(button);
        nameLabel->setStyleSheet("border: none;");
        nameLabel->setText(currentChatName);
        nameLabel->setGeometry(10, 10, 580, 40);
        QLabel *lastMessageLabel = new QLabel(button);
        lastMessageLabel->setObjectName("lastMessageLabel");
        lastMessageLabel->setStyleSheet("font: 14pt \"Segoe UI\";"
                                        "color: rgb(180, 180, 180);"
                                        "border: none;");
        lastMessageLabel->setText(chatText);
        lastMessageLabel->setGeometry(10, 60, 580, 60);
        configureChatButton(button, 130);
        if (ui->findUserLineEdit->text().isEmpty()) {
            ui->chats->insertWidget(0, button);
        }
        yourChats->append(button);
    }
}

void MainWindow::on_findUserLineEdit_textEdited(const QString &arg1)
{
    clearChats();
    if (arg1.isEmpty()) {
        for (int i = 0; i < yourChats->length(); ++i) {
            UserPushButton *current = yourChats->at(i);
            ui->chats->addWidget(current);
        }
    }
    else {
        QMap<QString, QString> body;
        body.insert("filter", arg1);
        emit findChats(body);
    }
}

void MainWindow::on_createGroupButton_clicked()
{
    QString groupName = ui->createGroupLineEdit->text();
    if (groupName.isEmpty()) {
        ui->createGroupWrap->setGeometry(800, 410, 320, 260);
        ui->createGroupLabel->setText("Empty name!");
    }
    if (groupName.length() > 30) {
        ui->createGroupWrap->setGeometry(800, 410, 320, 260);
        ui->createGroupLabel->setText("Too long name!");
    }
    QMap<QString, QString> body;
    body.insert("name", groupName);
    emit createGroup(body);
}


void MainWindow::on_toCreateGroupPageButton_clicked()
{
    setPageByName("createGroupPage");
}


void MainWindow::on_createGroupGoBackButton_clicked()
{
    setPageByName("homePage");
}

void MainWindow::on_messageLineEdit_returnPressed()
{
    on_sendMessageButton_clicked();
}

void MainWindow::on_toSignInButton_clicked()
{
    setPageByName("signInFormPage");
}

void MainWindow::on_toSignUpButton_clicked()
{
    setPageByName("signUpFormPage");
}

void MainWindow::on_signInButton_clicked()
{
    QString email = ui->signInEmailLineEdit->text();
    QString password = ui->signInPasswordLineEdit->text();
    if (email.isEmpty() || password.isEmpty()) {
        ui->signInFormWrap->setGeometry(810, 410, 300, 260);
        ui->signInWarning->setText("Fields are empty!");
        return ;
    }
    QMap<QString, QString> body;
    body.insert("email", email);
    body.insert("password", password);
    emit sign(body, "sign/in");
}

void MainWindow::on_signUpButton_clicked()
{
    QString name = ui->signUpUsernameLineEdit->text();
    QString email = ui->signUpEmailLineEdit->text();
    QString password = ui->signUpPasswordLineEdit->text();
    if (name.isEmpty() || email.isEmpty() || password.isEmpty()) {
        ui->signUpFormWrap->setGeometry(800, 385, 320, 310);
        ui->signUpWarning->setText("Fields are empty!");
        return ;
    }
    if (name.length() > 30) {
        ui->signUpFormWrap->setGeometry(800, 385, 320, 310);
        ui->signUpWarning->setText("Too long name!");
        return ;
    }
    if (name == "You") {
        ui->signUpFormWrap->setGeometry(800, 385, 320, 310);
        ui->signUpWarning->setText("Sorry, this name is reserved!");
        return ;
    }
    QMap<QString, QString> body;
    body.insert("name", name);
    body.insert("email", email);
    body.insert("password", password);
    emit sign(body, "sign/up");
}
