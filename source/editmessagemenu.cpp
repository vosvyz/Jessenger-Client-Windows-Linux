#include "editmessagemenu.h"

EditMessageMenu::EditMessageMenu()
    : QFrame() {
    setStyleSheet("font: 14pt \"Segoe UI\";"
                                  "border-radius: 5px;"
                                  "background-color: rgb(28, 26, 28);"
                                  "text-align: left;");

    setFixedHeight(120);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setSpacing(0);
    layout->setContentsMargins(3, 0, 3, 0);

    QPushButton *goBackButton = new QPushButton();
    goBackButton->setText("Go back");
    goBackButton->setStyleSheet("border-bottom: 1px solid rgb(180, 180, 180);");
    goBackButton->setFixedHeight(40);

    QPushButton *editButton = new QPushButton();
    editButton->setText("Edit");
    editButton->setStyleSheet("border-bottom: 1px solid rgb(180, 180, 180);");
    editButton->setFixedHeight(40);

    QPushButton *deleteButton = new QPushButton();
    deleteButton->setText("Delete");
    deleteButton->setFixedHeight(40);

    layout->addWidget(goBackButton);
    layout->addWidget(editButton);
    layout->addWidget(deleteButton);

    QObject::connect(goBackButton, &QPushButton::clicked, this, &EditMessageMenu::goBackClicked);
    QObject::connect(editButton, &QPushButton::clicked, this, &EditMessageMenu::editClicked);
    QObject::connect(deleteButton, &QPushButton::clicked, this, &EditMessageMenu::deleteClicked);
}

void EditMessageMenu::setTargetMessageWidget(MessageWidget* targetMessageWidget) {
    this->targetMessageWidget = targetMessageWidget;
}

MessageWidget* EditMessageMenu::getTargetMessageWidget() {
    return targetMessageWidget;
}
