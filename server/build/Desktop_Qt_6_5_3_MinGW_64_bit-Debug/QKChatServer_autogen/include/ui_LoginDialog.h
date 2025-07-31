/********************************************************************************
** Form generated from reading UI file 'LoginDialog.ui'
**
** Created by: Qt User Interface Compiler version 6.5.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGINDIALOG_H
#define UI_LOGINDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_LoginDialog
{
public:
    QVBoxLayout *mainLayout;
    QLabel *titleLabel;
    QFormLayout *formLayout;
    QLabel *usernameLabel;
    QLineEdit *usernameLineEdit;
    QLabel *passwordLabel;
    QLineEdit *passwordLineEdit;
    QCheckBox *rememberCheckBox;
    QLabel *statusLabel;
    QLabel *lockoutLabel;
    QHBoxLayout *buttonLayout;
    QPushButton *themeButton;
    QSpacerItem *horizontalSpacer;
    QPushButton *cancelButton;
    QPushButton *loginButton;

    void setupUi(QDialog *LoginDialog)
    {
        if (LoginDialog->objectName().isEmpty())
            LoginDialog->setObjectName("LoginDialog");
        LoginDialog->resize(380, 280);
        LoginDialog->setModal(true);
        mainLayout = new QVBoxLayout(LoginDialog);
        mainLayout->setSpacing(15);
        mainLayout->setObjectName("mainLayout");
        mainLayout->setContentsMargins(25, 25, 25, 25);
        titleLabel = new QLabel(LoginDialog);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setAlignment(Qt::AlignCenter);

        mainLayout->addWidget(titleLabel);

        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        formLayout->setHorizontalSpacing(8);
        formLayout->setVerticalSpacing(12);
        usernameLabel = new QLabel(LoginDialog);
        usernameLabel->setObjectName("usernameLabel");

        formLayout->setWidget(0, QFormLayout::LabelRole, usernameLabel);

        usernameLineEdit = new QLineEdit(LoginDialog);
        usernameLineEdit->setObjectName("usernameLineEdit");
        usernameLineEdit->setMinimumSize(QSize(180, 32));

        formLayout->setWidget(0, QFormLayout::FieldRole, usernameLineEdit);

        passwordLabel = new QLabel(LoginDialog);
        passwordLabel->setObjectName("passwordLabel");

        formLayout->setWidget(1, QFormLayout::LabelRole, passwordLabel);

        passwordLineEdit = new QLineEdit(LoginDialog);
        passwordLineEdit->setObjectName("passwordLineEdit");
        passwordLineEdit->setMinimumSize(QSize(180, 32));
        passwordLineEdit->setEchoMode(QLineEdit::Password);

        formLayout->setWidget(1, QFormLayout::FieldRole, passwordLineEdit);


        mainLayout->addLayout(formLayout);

        rememberCheckBox = new QCheckBox(LoginDialog);
        rememberCheckBox->setObjectName("rememberCheckBox");

        mainLayout->addWidget(rememberCheckBox);

        statusLabel = new QLabel(LoginDialog);
        statusLabel->setObjectName("statusLabel");
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setWordWrap(true);
        statusLabel->setMinimumSize(QSize(0, 40));

        mainLayout->addWidget(statusLabel);

        lockoutLabel = new QLabel(LoginDialog);
        lockoutLabel->setObjectName("lockoutLabel");
        lockoutLabel->setAlignment(Qt::AlignCenter);
        lockoutLabel->setVisible(false);

        mainLayout->addWidget(lockoutLabel);

        buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(8);
        buttonLayout->setObjectName("buttonLayout");
        themeButton = new QPushButton(LoginDialog);
        themeButton->setObjectName("themeButton");
        themeButton->setMaximumSize(QSize(40, 40));

        buttonLayout->addWidget(themeButton);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        buttonLayout->addItem(horizontalSpacer);

        cancelButton = new QPushButton(LoginDialog);
        cancelButton->setObjectName("cancelButton");
        cancelButton->setMinimumSize(QSize(70, 32));

        buttonLayout->addWidget(cancelButton);

        loginButton = new QPushButton(LoginDialog);
        loginButton->setObjectName("loginButton");
        loginButton->setMinimumSize(QSize(70, 32));

        buttonLayout->addWidget(loginButton);


        mainLayout->addLayout(buttonLayout);


        retranslateUi(LoginDialog);

        loginButton->setDefault(true);


        QMetaObject::connectSlotsByName(LoginDialog);
    } // setupUi

    void retranslateUi(QDialog *LoginDialog)
    {
        LoginDialog->setWindowTitle(QCoreApplication::translate("LoginDialog", "QK Chat \346\234\215\345\212\241\345\231\250\347\256\241\347\220\206\347\231\273\345\275\225", nullptr));
        titleLabel->setText(QCoreApplication::translate("LoginDialog", "QK Chat \346\234\215\345\212\241\345\231\250\347\256\241\347\220\206", nullptr));
        titleLabel->setStyleSheet(QCoreApplication::translate("LoginDialog", "font-size: 16px; font-weight: bold; margin-bottom: 8px;", nullptr));
        usernameLabel->setText(QCoreApplication::translate("LoginDialog", "\347\224\250\346\210\267\345\220\215:", nullptr));
        passwordLabel->setText(QCoreApplication::translate("LoginDialog", "\345\257\206\347\240\201:", nullptr));
        rememberCheckBox->setText(QCoreApplication::translate("LoginDialog", "\350\256\260\344\275\217\347\224\250\346\210\267\345\220\215", nullptr));
        statusLabel->setText(QString());
        lockoutLabel->setText(QString());
        lockoutLabel->setStyleSheet(QCoreApplication::translate("LoginDialog", "color: red; font-weight: bold;", nullptr));
        themeButton->setText(QCoreApplication::translate("LoginDialog", "\360\237\214\231", nullptr));
#if QT_CONFIG(tooltip)
        themeButton->setToolTip(QCoreApplication::translate("LoginDialog", "\345\210\207\346\215\242\344\270\273\351\242\230", nullptr));
#endif // QT_CONFIG(tooltip)
        cancelButton->setText(QCoreApplication::translate("LoginDialog", "\345\217\226\346\266\210", nullptr));
        loginButton->setText(QCoreApplication::translate("LoginDialog", "\347\231\273\345\275\225", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LoginDialog: public Ui_LoginDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGINDIALOG_H
