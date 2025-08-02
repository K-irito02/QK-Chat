#include "EmailTemplate.h"

QString EmailTemplate::getRegisterVerificationEmail(const QString& username, 
                                                  const QString& verificationLink,
                                                  const QString& appName) {
    return getEmailHeader("邮箱验证") +
           QString(R"(
    <div style="padding: 20px;">
        <h2 style="color: #333;">欢迎注册 %1！</h2>
        <p style="font-size: 16px; color: #666;">
            尊敬的 %2，
        </p>
        <p style="font-size: 16px; color: #666;">
            感谢您注册 %1！请验证您的邮箱地址以完成注册流程。
        </p>
        <div style="text-align: center; margin: 30px 0;">
            <a href="%3" style="%4">
                验证邮箱地址
            </a>
        </div>
        <p style="font-size: 14px; color: #888;">
            如果按钮无法点击，请复制以下链接到浏览器地址栏打开：<br/>
            <a href="%3" style="color: #007bff;">%3</a>
        </p>
        <p style="font-size: 14px; color: #888;">
            此链接将在24小时后过期，请尽快完成验证。
        </p>
        <p style="font-size: 14px; color: #888;">
            如果您没有注册%1账户，请忽略此邮件。
        </p>
    </div>
)")
           .arg(appName, username, verificationLink, getButtonStyle()) +
           getEmailFooter(appName);
}

QString EmailTemplate::getPasswordResetEmail(const QString& username,
                                           const QString& resetLink,
                                           const QString& appName) {
    return getEmailHeader("密码重置") +
           QString(R"(
    <div style="padding: 20px;">
        <h2 style="color: #333;">密码重置请求</h2>
        <p style="font-size: 16px; color: #666;">
            尊敬的 %1，
        </p>
        <p style="font-size: 16px; color: #666;">
            我们收到了您重置%2账户密码的请求。请点击下面的按钮重置您的密码：
        </p>
        <div style="text-align: center; margin: 30px 0;">
            <a href="%3" style="%4">
                重置密码
            </a>
        </div>
        <p style="font-size: 14px; color: #888;">
            如果按钮无法点击，请复制以下链接到浏览器地址栏打开：<br/>
            <a href="%3" style="color: #007bff;">%3</a>
        </p>
        <p style="font-size: 14px; color: #888;">
            此链接将在1小时后过期，请尽快完成操作。
        </p>
        <p style="font-size: 14px; color: #888;">
            如果您没有申请重置密码，请忽略此邮件，您的密码将保持不变。
        </p>
    </div>
)")
           .arg(username, appName, resetLink, getButtonStyle()) +
           getEmailFooter(appName);
}

QString EmailTemplate::getEmailChangeEmail(const QString& username,
                                         const QString& oldEmail,
                                         const QString& newEmail,
                                         const QString& verificationLink,
                                         const QString& appName) {
    return getEmailHeader("邮箱变更验证") +
           QString(R"(
    <div style="padding: 20px;">
        <h2 style="color: #333;">邮箱变更验证</h2>
        <p style="font-size: 16px; color: #666;">
            尊敬的 %1，
        </p>
        <p style="font-size: 16px; color: #666;">
            您正在请求将您的%2账户邮箱从 <strong>%3</strong> 更改为 <strong>%4</strong>。
        </p>
        <p style="font-size: 16px; color: #666;">
            请点击下面的按钮确认此变更：
        </p>
        <div style="text-align: center; margin: 30px 0;">
            <a href="%5" style="%6">
                确认邮箱变更
            </a>
        </div>
        <p style="font-size: 14px; color: #888;">
            如果按钮无法点击，请复制以下链接到浏览器地址栏打开：<br/>
            <a href="%5" style="color: #007bff;">%5</a>
        </p>
        <p style="font-size: 14px; color: #888;">
            此链接将在24小时后过期，请尽快完成验证。
        </p>
        <p style="font-size: 14px; color: #888;">
            如果您没有申请变更邮箱，请忽略此邮件。
        </p>
    </div>
)")
           .arg(username, appName, oldEmail, newEmail, verificationLink, getButtonStyle()) +
           getEmailFooter(appName);
}

QString EmailTemplate::getEmailVerificationCodeEmail(const QString& username,
                                                   const QString& verificationCode,
                                                   const QString& appName) {
    return getEmailHeader("邮箱验证码") +
           QString(R"(
    <div style="padding: 20px;">
        <h2 style="color: #333;">邮箱验证码</h2>
        <p style="font-size: 16px; color: #666;">
            尊敬的 %1，
        </p>
        <p style="font-size: 16px; color: #666;">
            您的%2账户邮箱验证码是：
        </p>
        <div style="text-align: center; margin: 30px 0;">
            <div style="background-color: #f8f9fa; border: 2px solid #007bff; border-radius: 8px; padding: 20px; display: inline-block;">
                <span style="font-size: 32px; font-weight: bold; color: #007bff; letter-spacing: 8px;">%3</span>
            </div>
        </div>
        <p style="font-size: 14px; color: #888;">
            请在10分钟内完成验证，验证码过期后将无法使用。
        </p>
        <p style="font-size: 14px; color: #888;">
            如果您没有申请邮箱验证，请忽略此邮件。
        </p>
    </div>
)")
           .arg(username, appName, verificationCode) +
           getEmailFooter(appName);
}

QString EmailTemplate::getEmailHeader(const QString& title) {
    return QString(R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>%1</title>
    <style>
        %2
    </style>
</head>
<body>
    <div style="max-width: 600px; margin: 0 auto; background: white; border-radius: 10px; overflow: hidden; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);">
        <div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; text-align: center;">
            <h1 style="margin: 0; font-size: 28px;">%3</h1>
        </div>
)").arg(title, getBaseStyle(), title);
}

QString EmailTemplate::getEmailFooter(const QString& appName) {
    return QString(R"(
    </div>
    <div style="text-align: center; padding: 20px; color: #666; font-size: 12px;">
        <p>此邮件由%1系统自动发送，请勿直接回复。</p>
        <p>如果这不是您的操作，请忽略此邮件。</p>
        <p>&copy; 2024 %1. 保留所有权利。</p>
    </div>
</body>
</html>
)").arg(appName);
}

QString EmailTemplate::getButtonStyle() {
    return R"(
    display: inline-block;
    padding: 12px 30px;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    text-decoration: none;
    border-radius: 25px;
    font-weight: bold;
    font-size: 16px;
    box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
    transition: all 0.3s ease;
)";
}

QString EmailTemplate::getBaseStyle() {
    return R"(
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            line-height: 1.6;
            margin: 0;
            padding: 20px;
            background-color: #f5f5f5;
        }
        
        a {
            color: #007bff;
            text-decoration: none;
        }
        
        a:hover {
            text-decoration: underline;
        }
        
        .highlight {
            background-color: #fff3cd;
            border: 1px solid #ffeaa7;
            border-radius: 5px;
            padding: 10px;
            margin: 10px 0;
        }
        
        @media only screen and (max-width: 600px) {
            body {
                padding: 10px;
            }
            
            .email-container {
                margin: 0;
                border-radius: 0;
            }
        }
    )";
}