#include "mainwindow.h"
#include <./ui_mainwindow.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->chats->setAlignment(Qt::AlignTop);
    ui->messages->setAlignment(Qt::AlignTop);

    ui->chatsContainer->setLayout(ui->chats);
    ui->messagesContainer->setLayout(ui->messages);

    messageOperationMode = "create";
    editMessageMenu = new EditMessageMenu();
    editMessageMenu->setParent(this);
    editMessageMenu->hide();

    connectSignals();

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

    ui->stackedWidget->setPage("homePage");

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
    QObject::connect(&wsClient, &WsClient::messageReceived, this, &MainWindow::messageReceived);
    QObject::connect(&wsClient, &WsClient::createMessageAcknowledged, this, &MainWindow::createMessageAcknowledged);
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

    QObject::connect(editMessageMenu, &EditMessageMenu::goBackClicked, this, &MainWindow::hideEditMessageMenu);
    QObject::connect(editMessageMenu, &EditMessageMenu::editClicked, this, &MainWindow::editMessageClicked);
    QObject::connect(editMessageMenu, &EditMessageMenu::deleteClicked, this, &MainWindow::deleteMessageClicked);

    QObject::connect(&httpClient, &HttpClient::createGroupError, this, &MainWindow::createGroupError);

    QObject::connect(ui->messagesScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::messagesScrolled);
}

void MainWindow::hideEditMessageMenu() {
    editMessageMenu->hide();
}

void MainWindow::editMessageClicked() {
    editMessageMenu->hide();
    messageOperationMode = "edit";
    if (editedMessage) {
        editedMessage->setStyleSheet(editedMessage->styleSheet() + "background-color: rgb(62, 105, 120);");
    }
    editedMessage = editMessageMenu->getTargetMessageWidget();
    editedMessage->setStyleSheet(editedMessage->styleSheet() + "background-color: rgb(75, 80, 100);");
    ui->messageLineEdit->setFixedWidth(1190); // show cancel button
    ui->messageLineEdit->setText(editedMessage->getText());
}

void MainWindow::deleteMessageClicked() {
    editMessageMenu->hide();
    MessageWidget *target = editMessageMenu->getTargetMessageWidget();
    int targetIndex = ui->messages->indexOf(target);
    qlonglong tempId = static_cast<qlonglong>(QRandomGenerator::global()->generate64()); // temp id for the message
    qlonglong targetMessageId = target->getId();
    QJsonObject data;
    data["method"] = "delete";
    data["tempId"] = tempId;
    data["targetMessageId"] = targetMessageId;
    data["chatId"] = currentChatId;
    data["toGroup"] = isCurrentChatGroup;
    delete target;
    if (targetIndex == ui->messages->count()) {
        if (!ui->messages->isEmpty()) {
            MessageWidget* prev = qobject_cast<MessageWidget*>(ui->messages->itemAt(ui->messages->count() - 1)->widget());
            data["prevId"] = prev->getId(); // Other clients need to get data about prev. message when they get "delete" event
            data["prevText"] = prev->getText();
            data["prevSender"] = prev->getSender();
            data["prevTime"] = prev->getTime();
            updateChatsWithDeleteMessage(currentChatId, isCurrentChatGroup, targetMessageId, prev->getId(), prev->getText(), prev->getSender(), prev->getTime());
        }
        else {
            updateChatsWithDeleteMessage(currentChatId, isCurrentChatGroup, targetMessageId);
        }
    }
    emit sendMessage(data);
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
    editMessageMenu->hide();
    ui->messageLineEdit->setFixedWidth(1240);
    messageOperationMode = "create";
    ui->messageLineEdit->clear();
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

ChatPushButton* MainWindow::createChatPushButton(qlonglong chatId, QString chatName, bool isChatGroup, qlonglong lastMessageId,
                                                 QString lastMessageText, QString lastMessageSender, qlonglong lastMessageTime) {
    QString lastMessageTimeStr = (QDateTime::fromMSecsSinceEpoch(lastMessageTime, Qt::UTC))
                                     .toLocalTime().toString("dd.MM.yy, hh:mm");
    int neededHeight = 60;
    ChatPushButton* button = new ChatPushButton(chatId, chatName, isChatGroup, lastMessageId, lastMessageSender, lastMessageTime);
    QLabel *nameLabel = new QLabel(button);
    nameLabel->setStyleSheet("border: none;");
    nameLabel->setText(chatName);
    nameLabel->setGeometry(10, 10, 580, 40);

    QLabel *lastMessageLabel = new QLabel(button);
    lastMessageLabel->setObjectName("lastMessageLabel");
    lastMessageLabel->setStyleSheet("font: 14pt \"Segoe UI\";"
                                    "color: rgb(180, 180, 180);"
                                    "border: none;");
    lastMessageLabel->setGeometry(10, 60, 580, 60);

    if (!lastMessageText.isEmpty()) {
        neededHeight = 130;
        QString lastMessageInfo = createLastMessageInfo(lastMessageText, lastMessageSender, lastMessageTime);
        lastMessageLabel->setText(lastMessageInfo);
    }

    button->setStyleSheet("font: 16pt \"Segoe UI\";"
                          "border-bottom: 1px solid rgb(180, 180, 180);");
    button->setFixedSize(580, neededHeight);
    QObject::connect(button, &QPushButton::clicked, this, &MainWindow::openChat);
    return button;
}

void MainWindow::showChats(QJsonObject data) {
    QString filter = data["filter"].toString();
    bool areChatsYours = filter.isEmpty();
    QJsonArray arr = data["chats"].toArray();
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject object = arr[i].toObject();
        ChatPushButton *button = createChatPushButton(object["chatId"].toInteger(), object["chatName"].toString(),
                                                      object["group"].toBool(), object["messageId"].toInteger(),
                                                      object["text"].toString(), object["senderName"].toString(), object["time"].toInteger());
        if (areChatsYours) {
            yourChats.append(button);
        }
        if (ui->findUserLineEdit->text() == filter) {
            ui->chats->addWidget(button);
        }
    }
}

void MainWindow::groupCreated(QJsonObject data) {
    ui->groupNameLineEdit->clear();
    ui->createGroupWaitingLabel->movie()->stop();
    ui->createGroupWaitingLabel->hide();
    ui->createGroupButton->setEnabled(true);

    qlonglong id = data["id"].toInteger();
    QString name = data["name"].toString();
    ChatPushButton *button = createChatPushButton(id, name, true);
    if (ui->findUserLineEdit->text().isEmpty()) {
        ui->chats->addWidget(button);
    }
    yourChats.append(button);
    on_createGroupGoBackButton_clicked(); // Return to the home page
}

void MainWindow::messagesScrolled(int value) {
    if (value > 300) {
        return ;
    }
    if (isCurrentChatFullyLoaded) {
        return ;
    }
    QLayoutItem *item = ui->messages->itemAt(0);
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

void MainWindow::messageRightClicked() { // notice that messages of other users don't get connected to this slot (createMessageWidget)
    QObject *messageButtonAsObj = sender();
    MessageWidget *message = dynamic_cast<MessageWidget*>(messageButtonAsObj);
    if (message->isTemp()) {
        return ;
    }
    int x = cursor().pos().x();
    int y = cursor().pos().y();
    editMessageMenu->setTargetMessageWidget(message);
    editMessageMenu->move(x, y);
    editMessageMenu->show();
}

void MainWindow::showMessages(QJsonObject data) {
    qlonglong chatId = data["chatId"].toInteger();
    bool fromGroup = data["group"].toBool();
    bool justOpened = data["justOpened"].toBool();
    QJsonArray arr = data["messages"].toArray();
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
        MessageWidget* message = createMessageWidget(object["id"].toInteger(), object["text"].toString(), object["senderName"].toString(),
                            object["time"].toInteger(), false);
        ui->messages->insertWidget(0, message);
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

MessageWidget* MainWindow::createMessageWidget(qlonglong id, QString text, QString sender, qlonglong time, bool isTemp) {
    bool areYouSender = (sender == "You");
    QString timeStr = (QDateTime::fromMSecsSinceEpoch(time, Qt::UTC))
                                     .toLocalTime().toString("dd.MM.yy, hh:mm");

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
    int timeLabelWidth = littleMetrics.horizontalAdvance(timeStr);
    int biggestElementWidth = std::max({messageLabelWidth, timeLabelWidth, nameLabelWidth}) + 20;
    int containerWidth = std::min(800, biggestElementWidth);

    MessageWidget *finalContainer = new MessageWidget(id, isTemp, areYouSender, text, sender, time);
    finalContainer->setFixedWidth(containerWidth);
    finalContainer->setStyleSheet("font: 16pt \"Segoe UI\";"
                                  "border-radius: 10px;"
                                  + backgroundColor);
    finalContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    QVBoxLayout *layout = new QVBoxLayout(finalContainer);
    layout->setSpacing(0);
    layout->setContentsMargins(5, 3, 15, 3);

    QLabel *nameLabel = new QLabel();
    nameLabel->setText(sender);
    nameLabel->setStyleSheet("font: 14pt \"Segoe UI\"; color: rgb(188, 188, 225);");

    QLabel *messageLabel = new QLabel();
    messageLabel->setObjectName("messageLabel");
    messageLabel->setWordWrap(true);
    messageLabel->setText(text);
    messageLabel->setContextMenuPolicy(Qt::NoContextMenu);
    messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QLabel *timeLabel = new QLabel();
    timeLabel->setText(timeStr);
    timeLabel->setStyleSheet("font: 12pt \"Segoe UI\"; color: rgb(180, 180, 180);");

    layout->addWidget(nameLabel);
    layout->addWidget(messageLabel);
    layout->addWidget(timeLabel);

    if (areYouSender) {
        QObject::connect(finalContainer, &MessageWidget::clicked, this, &MainWindow::messageRightClicked);
    }

    return finalContainer;
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

void MainWindow::on_sendMessageButton_clicked() // look for deletion of a message logic in the deleteMessageClicked method
{
    QString text = ui->messageLineEdit->text();
    if (!atChat || text.isEmpty()) {
        return ;
    }
    ui->messageLineEdit->clear();
    QJsonObject data;
    data["method"] = messageOperationMode;
    qlonglong tempId = static_cast<qlonglong>(QRandomGenerator::global()->generate64()); // temp id for the message
    if (messageOperationMode == "create") {
        QDateTime utcDateTime = QDateTime::currentDateTimeUtc();
        qlonglong msecs = utcDateTime.toMSecsSinceEpoch();
        data["time"] = msecs;
        data["text"] = text;
        data["chatId"] = currentChatId;
        data["toGroup"] = isCurrentChatGroup;
        data["tempId"] = tempId;
        emit sendMessage(data);
        ui->messages->addWidget(createMessageWidget(tempId, text, "You", msecs, true));
        QTimer::singleShot(100, this, [=]() {
            QScrollBar *bar = ui->messagesScrollArea->verticalScrollBar();
            bar->setValue(bar->maximum());
        });
        updateChatsWithCreateMessage(currentChatId, currentChatName, isCurrentChatGroup, tempId, text, "You", msecs);
    }
    if (messageOperationMode == "edit") {
        data["text"] = text;
        data["tempId"] = tempId;
        data["chatId"] = currentChatId;
        data["toGroup"] = isCurrentChatGroup;
        data["targetMessageId"] = editedMessage->getId();
        emit sendMessage(data);
        ui->messageLineEdit->setFixedWidth(1240);
        int targetIndex = ui->messages->indexOf(editedMessage);
        MessageWidget* newMW = createMessageWidget(editedMessage->getId(), text,
                                                   editedMessage->getSender(), editedMessage->getTime(), editedMessage->isTemp());
        delete editedMessage;
        ui->messages->insertWidget(targetIndex, newMW);
        if (targetIndex == ui->messages->count() - 1) {
            updateChatsWithEditMessage(currentChatId, isCurrentChatGroup, newMW->getId(), newMW->getText());
        }
    }
    messageOperationMode = "create";
}

void MainWindow::messageReceived(QJsonObject data) {
    QString method = data["method"].toString();
    if (method == "create") {
        qlonglong timeLong = data["time"].toInteger(); // some parameters are added on the server side
        qlonglong id = data["id"].toInteger();
        QString text = data["text"].toString();
        bool toGroup = data["toGroup"].toBool();
        qlonglong chatId = data["chatId"].toInteger();
        QString chatName = data["chatName"].toString();
        QString sender = data["senderName"].toString();
        updateChatsWithCreateMessage(chatId, chatName, toGroup, id, text, sender, timeLong);
        if (toGroup != isCurrentChatGroup || chatId != currentChatId) { // The user is in the other chat
            // create a notification here
            return ;
        }
        ui->messages->addWidget(createMessageWidget(id, text, sender, timeLong, false));
        QTimer::singleShot(100, this, [=]() {
            QScrollBar *bar = ui->messagesScrollArea->verticalScrollBar();
            bar->setValue(bar->maximum());
        });
    }
    if (method == "edit") {
        QString text = data["text"].toString();
        bool toGroup = data["toGroup"].toBool();
        qlonglong chatId = data["chatId"].toInteger();
        qlonglong targetMessageId = data["targetMessageId"].toInteger();
        if (toGroup == isCurrentChatGroup && chatId == currentChatId) {
            for (int i = 0; i < ui->messages->count(); ++i) {
                MessageWidget* curr = qobject_cast<MessageWidget*>(ui->messages->itemAt(i)->widget());
                if (targetMessageId == curr->getId()) {
                    curr->setText(text);
                    curr->findChild<QLabel*>("messageLabel")->setText(text);
                    break;
                }
            }
        }
        updateChatsWithEditMessage(chatId, toGroup, targetMessageId, text);
    }
    if (method == "delete") {
        qlonglong targetMessageId = data["targetMessageId"].toInteger();
        bool toGroup = data["toGroup"].toBool();
        qlonglong chatId = data["chatId"].toInteger();
        qlonglong prevId = data["prevId"].toInteger();
        QString prevText = data["prevText"].toString();
        QString prevSender = data["prevSender"].toString();
        qlonglong prevTime = data["prevTime"].toInteger();
        if (toGroup == isCurrentChatGroup && chatId == currentChatId) {
            for (int i = 0; i < ui->messages->count(); ++i) {
                MessageWidget* curr = qobject_cast<MessageWidget*>(ui->messages->itemAt(i)->widget());
                if (targetMessageId == curr->getId()) {
                    delete curr;
                    break;
                }
            }
        }
        updateChatsWithDeleteMessage(chatId, toGroup, targetMessageId, prevId, prevText, prevSender, prevTime);
    }
}

void MainWindow::on_messageLineEdit_returnPressed()
{
    on_sendMessageButton_clicked();
}

ChatPushButton* MainWindow::findChatButton(qlonglong chatId, bool group) {
    for (int i = 0; i < yourChats.length(); ++i) {
        ChatPushButton *current = yourChats.at(i);
        if (current->getId() == chatId && group == current->isGroup()) {
            return current;
        }
    }
    return nullptr; // Button not found
}

void MainWindow::updateChatsWithCreateMessage(qlonglong chatId, QString chatName, bool group, qlonglong messageId, QString text, QString sender, qlonglong time) {
    ChatPushButton *button = findChatButton(chatId, group);
    if (button) {
        button->setLastMessageId(messageId);
        button->setLastMessageSenderName(sender);
        button->setLastMessageTime(time);
        button->setFixedHeight(130);
        button->findChild<QLabel*>("lastMessageLabel")->setText(createLastMessageInfo(text, sender, time));
        yourChats.push_back(button);
        yourChats.removeOne(button);
        if (ui->findUserLineEdit->text().isEmpty()) {
            ui->chats->removeWidget(button);
            ui->chats->insertWidget(0, button);
        }
    }
    else {
        button = createChatPushButton(chatId, chatName, group, messageId, text, sender, time);
        if (ui->findUserLineEdit->text().isEmpty()) {
            ui->chats->insertWidget(0, button);
        }
        yourChats.append(button);
    }
}

void MainWindow::updateChatsWithEditMessage(qlonglong chatId, bool group, qlonglong editedMessageId, QString text) {
    ChatPushButton *button = findChatButton(chatId, group);
    if (button && editedMessageId == button->getLastMessageId()) {
        button->findChild<QLabel*>("lastMessageLabel")->setText(createLastMessageInfo(text,
                                                                                       button->getLastMessageSenderName(),
                                                                                       button->getLastMessageTime()));
    }
}

void MainWindow::updateChatsWithDeleteMessage(qlonglong chatId, bool group, qlonglong deletedMessageId,
                                              qlonglong newMessageId, QString text, QString sender, qlonglong time) {
    ChatPushButton *button = findChatButton(chatId, group);
    if (button && deletedMessageId == button->getLastMessageId()) {
        button->setLastMessageId(newMessageId);
        button->setLastMessageSenderName(sender);
        button->setLastMessageTime(time);
        if (text.isEmpty()) {
            button->setFixedHeight(60);
        }
        button->findChild<QLabel*>("lastMessageLabel")->setText(createLastMessageInfo(text, sender, time));
    }
}

QString MainWindow::createLastMessageInfo(QString text, QString sender, qlonglong time) {
    QString lastMessageTimeStr = (QDateTime::fromMSecsSinceEpoch(time, Qt::UTC))
    .toLocalTime().toString("dd.MM.yy, hh:mm");
    QString lastMessageInfo = sender + ": " + text.left(30);
    if (text.size() > 30) {
        lastMessageInfo += "...";
    }
    lastMessageInfo += "\n" + lastMessageTimeStr;
    return lastMessageInfo;
}

void MainWindow::createMessageAcknowledged(QJsonObject data) {
    qlonglong tempId = data["tempId"].toInteger();
    qlonglong newId = data["newId"].toInteger();
    qlonglong chatId = data["chatId"].toInteger();
    bool toGroup = data["toGroup"].toBool();
    for (int i = 0; i < ui->messages->count(); ++i) {
        MessageWidget *messageWidget = qobject_cast<MessageWidget*>(ui->messages->itemAt(i)->widget());
            if (messageWidget->isTemp() && messageWidget->getId() == tempId) {
                messageWidget->setTemp(false);
                messageWidget->setId(newId);
                break;
        }
    }
    for (int i = 0; i < ui->chats->count(); ++i) {
        ChatPushButton *chat = qobject_cast<ChatPushButton*>(ui->chats->itemAt(i)->widget());
        if (chat->getId() == chatId && chat->isGroup() == toGroup) {
            chat->setLastMessageId(newId);
            break;
        }
    }
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

    ui->stackedWidget->setPage("homePage");
}

void MainWindow::wsConnected() {
    ui->connectionStatement->setText("Connected!");
}

void MainWindow::wsDisconnected() {
    ui->connectionStatement->setText("Connecting...");
}

void MainWindow::signProcessed() {
    ui->stackedWidget->setPage("homePage");
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
    ui->stackedWidget->setPage("confirmEmailPage");
}

void MainWindow::confirmEmailExpired()
{
    QString warning = "You didn't confirm your email!";
    if (authType == AuthorizationType::Login) {

        ui->stackedWidget->setPage("signInFormPage");
        displaySignInWarning(warning);
    }
    if (authType == AuthorizationType::Registration) {
        ui->stackedWidget->setPage("signUpFormPage");
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
    editMessageMenu->hide();
    QString currentPage = ui->stackedWidget->getCurrentPageName();
    if (currentPage != "signPage" && currentPage != "signInFormPage" && currentPage != "signUpFormPage") {
        ui->stackedWidget->setPage("signPage");
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
    ui->stackedWidget->setPage("signInFormPage");
}

void MainWindow::on_goToSignUpButton_clicked()
{
    ui->stackedWidget->setPage("signUpFormPage");
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
    ui->stackedWidget->setPage("signUpFormPage");
}

void MainWindow::on_fromSignUpToSignInButton_clicked()
{
    ui->stackedWidget->setPage("signInFormPage");
}

void MainWindow::on_goToCreateGroupPageButton_clicked()
{
    editMessageMenu->hide();
    ui->stackedWidget->setPage("createGroupPage");
}

void MainWindow::on_cancelMessageEditButton_clicked()
{
    ui->messageLineEdit->setFixedWidth(1240);
    messageOperationMode = "create";
}

