# QKChatApp 快速诊断脚本
# 检查邮箱验证码发送问题的所有可能原因

Write-Host "=== QKChatApp 邮箱验证问题诊断 ===" -ForegroundColor Green
Write-Host ""

# 1. 检查Redis服务状态
Write-Host "1. 检查Redis服务状态" -ForegroundColor Yellow
try {
    $redisProcess = Get-Process redis-server -ErrorAction SilentlyContinue
    if ($redisProcess) {
        Write-Host "   ✅ Redis服务正在运行 (PID: $($redisProcess.Id))" -ForegroundColor Green
    } else {
        Write-Host "   ❌ Redis服务未运行" -ForegroundColor Red
        Write-Host "   💡 请启动Redis服务: redis-server" -ForegroundColor Cyan
    }
} catch {
    Write-Host "   ❌ 无法检查Redis服务状态" -ForegroundColor Red
}

# 2. 测试Redis连接
Write-Host ""
Write-Host "2. 测试Redis连接" -ForegroundColor Yellow
try {
    $redisTest = redis-cli ping 2>$null
    if ($redisTest -eq "PONG") {
        Write-Host "   ✅ Redis连接正常" -ForegroundColor Green
    } else {
        Write-Host "   ❌ Redis连接失败" -ForegroundColor Red
    }
} catch {
    Write-Host "   ❌ 无法连接到Redis" -ForegroundColor Red
    Write-Host "   💡 请检查Redis是否在localhost:6379运行" -ForegroundColor Cyan
}

# 3. 检查SMTP配置
Write-Host ""
Write-Host "3. 检查SMTP配置" -ForegroundColor Yellow
$smtpHost = "smtp.qq.com"
$smtpPort = 587

try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $tcpClient.Connect($smtpHost, $smtpPort)
    if ($tcpClient.Connected) {
        Write-Host "   ✅ SMTP服务器连接正常 ($smtpHost:$smtpPort)" -ForegroundColor Green
        $tcpClient.Close()
    }
} catch {
    Write-Host "   ❌ 无法连接到SMTP服务器: $($_.Exception.Message)" -ForegroundColor Red
}

# 4. 检查网络连接
Write-Host ""
Write-Host "4. 检查网络连接" -ForegroundColor Yellow
try {
    $ping = Test-Connection -ComputerName "smtp.qq.com" -Count 1 -Quiet
    if ($ping) {
        Write-Host "   ✅ 网络连接正常" -ForegroundColor Green
    } else {
        Write-Host "   ❌ 网络连接异常" -ForegroundColor Red
    }
} catch {
    Write-Host "   ❌ 网络测试失败" -ForegroundColor Red
}

# 5. 检查服务端进程
Write-Host ""
Write-Host "5. 检查服务端进程" -ForegroundColor Yellow
try {
    $serverProcess = Get-Process QKChatServer -ErrorAction SilentlyContinue
    if ($serverProcess) {
        Write-Host "   ✅ 服务端正在运行 (PID: $($serverProcess.Id))" -ForegroundColor Green
    } else {
        Write-Host "   ❌ 服务端未运行" -ForegroundColor Red
    }
} catch {
    Write-Host "   ❌ 无法检查服务端状态" -ForegroundColor Red
}

# 6. 检查客户端进程
Write-Host ""
Write-Host "6. 检查客户端进程" -ForegroundColor Yellow
try {
    $clientProcess = Get-Process QKChatClient -ErrorAction SilentlyContinue
    if ($clientProcess) {
        Write-Host "   ✅ 客户端正在运行 (PID: $($clientProcess.Id))" -ForegroundColor Green
    } else {
        Write-Host "   ❌ 客户端未运行" -ForegroundColor Red
    }
} catch {
    Write-Host "   ❌ 无法检查客户端状态" -ForegroundColor Red
}

# 7. 检查日志文件
Write-Host ""
Write-Host "7. 检查日志文件" -ForegroundColor Yellow
$logPaths = @(
    "D:\QT_Learn\Projects\QKChatApp\logs\client\Qt Creator客户端控制台信息.md",
    "D:\QT_Learn\Projects\QKChatApp\logs\server\Qt Creator服务端控制台信息.md"
)

foreach ($logPath in $logPaths) {
    if (Test-Path $logPath) {
        $logSize = (Get-Item $logPath).Length
        Write-Host "   ✅ 日志文件存在: $(Split-Path $logPath -Leaf) ($logSize bytes)" -ForegroundColor Green
    } else {
        Write-Host "   ❌ 日志文件不存在: $(Split-Path $logPath -Leaf)" -ForegroundColor Red
    }
}

# 8. 建议的修复步骤
Write-Host ""
Write-Host "8. 建议的修复步骤" -ForegroundColor Yellow
Write-Host "   1. 确保Redis服务正在运行" -ForegroundColor Cyan
Write-Host "   2. 重启服务端和客户端" -ForegroundColor Cyan
Write-Host "   3. 检查服务端日志中的初始化信息" -ForegroundColor Cyan
Write-Host "   4. 测试邮箱验证码发送功能" -ForegroundColor Cyan
Write-Host "   5. 查看详细的错误日志" -ForegroundColor Cyan

Write-Host ""
Write-Host "=== 诊断完成 ===" -ForegroundColor Green
Write-Host "💡 如果问题仍然存在，请查看详细的服务端和客户端日志" -ForegroundColor Cyan
