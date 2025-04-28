#include "customstackedwidget.h"

CustomStackedWidget::CustomStackedWidget(QWidget *parent)
    : QStackedWidget() {
    setParent(parent);
    pageNameToIndexMap["signPage"] = 0;
    pageNameToIndexMap["signInFormPage"] = 1;
    pageNameToIndexMap["signUpFormPage"] = 2;
    pageNameToIndexMap["homePage"] = 3;
    pageNameToIndexMap["confirmEmailPage"] = 4;
    pageNameToIndexMap["createGroupPage"] = 5;
}

QString CustomStackedWidget::getCurrentPageName() {
    return currentPageName;
}

void CustomStackedWidget::setPage(QString pageName) {
    currentPageName = pageName;
    int index = pageNameToIndexMap[pageName];
    setCurrentIndex(index);
}
