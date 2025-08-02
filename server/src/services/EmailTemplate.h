#ifndef EMAILTEMPLATE_H
#define EMAILTEMPLATE_H

#include <QString>
#include <QMap>

class EmailTemplate {
public:
    static QString getRegisterVerificationEmail(const QString& username, 
                                              const QString& verificationLink,
                                              const QString& appName = "QKChat");
    
    static QString getPasswordResetEmail(const QString& username,
                                       const QString& resetLink,
                                       const QString& appName = "QKChat");
    
    static QString getEmailChangeEmail(const QString& username,
                                     const QString& oldEmail,
                                     const QString& newEmail,
                                     const QString& verificationLink,
                                     const QString& appName = "QKChat");
    
    static QString getEmailVerificationCodeEmail(const QString& username,
                                               const QString& verificationCode,
                                               const QString& appName = "QKChat");

private:
    static QString getEmailHeader(const QString& title);
    static QString getEmailFooter(const QString& appName);
    static QString getButtonStyle();
    static QString getBaseStyle();
};

#endif // EMAILTEMPLATE_H