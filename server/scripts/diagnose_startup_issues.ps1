# QK Chat 服务器启动问题诊断脚本
# 用于排查管理后台无响应和进程崩溃问题

Write-Host "=== QK Chat 服务器启动问题诊断 ===" -ForegroundColor Green
Write-Host "开始时间: $(Get-Date)" -ForegroundColor Yellow

# 1. 检查系统环境
Write-Host "`n1. 检查系统环境..." -ForegroundColor Cyan
Write-Host "操作系统: $($env:OS)"
Write-Host "系统版本: $(Get-WmiObject -Class Win32_OperatingSystem | Select-Object -ExpandProperty Version)"
Write-Host "可用内存: $([math]::Round((Get-WmiObject -Class Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 2)) GB"

# 2. 检查Qt环境
Write-Host "`n2. 检查Qt环境..." -ForegroundColor Cyan
$qtPath = "C:\Qt"
if (Test-Path $qtPath) {
    Write-Host "Qt安装路径存在: $qtPath" -ForegroundColor Green
    $qtVersions = Get-ChildItem $qtPath -Directory | Where-Object { $_.Name -match "^\d+\.\d+" }
    Write-Host "找到Qt版本: $($qtVersions.Name -join ', ')"
} else {
    Write-Host "Qt安装路径不存在: $qtPath" -ForegroundColor Red
}

# 3. 检查数据库连接
Write-Host "`n3. 检查数据库连接..." -ForegroundColor Cyan
$mysqlService = Get-Service -Name "MySQL*" -ErrorAction SilentlyContinue
if ($mysqlService) {
    Write-Host "MySQL服务状态: $($mysqlService.Status)" -ForegroundColor Green
    if ($mysqlService.Status -eq "Running") {
        Write-Host "MySQL服务正在运行" -ForegroundColor Green
    } else {
        Write-Host "MySQL服务未运行，尝试启动..." -ForegroundColor Yellow
        Start-Service $mysqlService.Name
        Start-Sleep -Seconds 5
        $mysqlService = Get-Service -Name $mysqlService.Name
        Write-Host "MySQL服务状态: $($mysqlService.Status)"
    }
} else {
    Write-Host "未找到MySQL服务" -ForegroundColor Red
}

# 4. 检查SSL证书
Write-Host "`n4. 检查SSL证书..." -ForegroundColor Cyan
$certPath = "server\certs"
if (Test-Path $certPath) {
    Write-Host "SSL证书目录存在: $certPath" -ForegroundColor Green
    $certFiles = Get-ChildItem $certPath -File
    Write-Host "证书文件:"
    foreach ($file in $certFiles) {
        Write-Host "  - $($file.Name)"
    }
} else {
    Write-Host "SSL证书目录不存在: $certPath" -ForegroundColor Red
}

# 5. 检查配置文件
Write-Host "`n5. 检查配置文件..." -ForegroundColor Cyan
$configFiles = @(
    "server\config\dev.conf",
    "server\config\database_config.sql",
    "server\config\ssl_config.cnf"
)

foreach ($configFile in $configFiles) {
    if (Test-Path $configFile) {
        Write-Host "配置文件存在: $configFile" -ForegroundColor Green
    } else {
        Write-Host "配置文件不存在: $configFile" -ForegroundColor Red
    }
}

# 6. 检查日志文件
Write-Host "`n6. 检查日志文件..." -ForegroundColor Cyan
$logPath = "logs"
if (Test-Path $logPath) {
    Write-Host "日志目录存在: $logPath" -ForegroundColor Green
    $logFiles = Get-ChildItem $logPath -File | Sort-Object LastWriteTime -Descending | Select-Object -First 5
    Write-Host "最近的日志文件:"
    foreach ($file in $logFiles) {
        Write-Host "  - $($file.Name) ($($file.LastWriteTime))"
    }
} else {
    Write-Host "日志目录不存在: $logPath" -ForegroundColor Red
}

# 7. 检查进程
Write-Host "`n7. 检查相关进程..." -ForegroundColor Cyan
$processes = @("QKChatServer", "mysql", "mysqld")
foreach ($process in $processes) {
    $runningProcesses = Get-Process -Name $process -ErrorAction SilentlyContinue
    if ($runningProcesses) {
        Write-Host "进程正在运行: $process" -ForegroundColor Green
        foreach ($proc in $runningProcesses) {
            Write-Host "  - PID: $($proc.Id), 内存: $([math]::Round($proc.WorkingSet64 / 1MB, 2)) MB"
        }
    } else {
        Write-Host "进程未运行: $process" -ForegroundColor Yellow
    }
}

# 8. 检查端口占用
Write-Host "`n8. 检查端口占用..." -ForegroundColor Cyan
$ports = @(8443, 3306, 6379)
foreach ($port in $ports) {
    $connections = netstat -an | findstr ":$port "
    if ($connections) {
        Write-Host "端口 $port 被占用:" -ForegroundColor Yellow
        Write-Host $connections
    } else {
        Write-Host "端口 $port 可用" -ForegroundColor Green
    }
}

# 9. 检查防火墙
Write-Host "`n9. 检查防火墙设置..." -ForegroundColor Cyan
$firewallRules = Get-NetFirewallRule -DisplayName "*QK*" -ErrorAction SilentlyContinue
if ($firewallRules) {
    Write-Host "找到QK相关防火墙规则:" -ForegroundColor Green
    foreach ($rule in $firewallRules) {
        Write-Host "  - $($rule.DisplayName) (状态: $($rule.Enabled))"
    }
} else {
    Write-Host "未找到QK相关防火墙规则" -ForegroundColor Yellow
}

# 10. 生成诊断报告
Write-Host "`n10. 生成诊断报告..." -ForegroundColor Cyan
$reportPath = "诊断报告_$(Get-Date -Format 'yyyyMMdd_HHmmss').txt"
$report = @"
QK Chat 服务器启动问题诊断报告
生成时间: $(Get-Date)

系统信息:
- 操作系统: $($env:OS)
- 系统版本: $(Get-WmiObject -Class Win32_OperatingSystem | Select-Object -ExpandProperty Version)
- 可用内存: $([math]::Round((Get-WmiObject -Class Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 2)) GB

Qt环境:
- Qt安装路径: $(if (Test-Path $qtPath) { "存在" } else { "不存在" })
- Qt版本: $(if ($qtVersions) { $qtVersions.Name -join ', ' } else { "未找到" })

数据库状态:
- MySQL服务: $(if ($mysqlService) { $mysqlService.Status } else { "未找到" })

SSL证书:
- 证书目录: $(if (Test-Path $certPath) { "存在" } else { "不存在" })

配置文件:
$(foreach ($configFile in $configFiles) {
    "- $configFile`: $(if (Test-Path $configFile) { "存在" } else { "不存在" })"
})

进程状态:
$(foreach ($process in $processes) {
    $runningProcesses = Get-Process -Name $process -ErrorAction SilentlyContinue
    "- $process`: $(if ($runningProcesses) { "运行中" } else { "未运行" })"
})

端口状态:
$(foreach ($port in $ports) {
    $connections = netstat -an | findstr ":$port "
    "- 端口 $port`: $(if ($connections) { "被占用" } else { "可用" })"
})
"@

$report | Out-File -FilePath $reportPath -Encoding UTF8
Write-Host "诊断报告已保存到: $reportPath" -ForegroundColor Green

Write-Host "`n=== 诊断完成 ===" -ForegroundColor Green
Write-Host "结束时间: $(Get-Date)" -ForegroundColor Yellow

# 提供建议
Write-Host "`n建议的排查步骤:" -ForegroundColor Cyan
Write-Host "1. 检查日志文件中的错误信息"
Write-Host "2. 确保MySQL服务正在运行"
Write-Host "3. 验证SSL证书文件存在且有效"
Write-Host "4. 检查配置文件中的连接参数"
Write-Host "5. 确保端口8443未被其他程序占用"
Write-Host "6. 尝试以管理员身份运行程序"
Write-Host "7. 检查防火墙设置是否阻止了程序" 