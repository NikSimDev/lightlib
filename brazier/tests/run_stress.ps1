param(
    [int]$Runs = 20,
    [string]$Filter = "HttpsRoutingTest.MultipleRequests",
    [string]$Exe = "..\build\Release\brazier_tests.exe",
    [string]$LogDir = ".\stress_logs",
    [switch]$SaveAll
)

if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
}

Get-ChildItem "$LogDir\*.log" -ErrorAction SilentlyContinue | Remove-Item -Force

$passed = 0
$failed = 0
$failedRuns = @()

Write-Host "Starting $Runs runs of: $Filter" -ForegroundColor Cyan
Write-Host "Logs directory: $LogDir" -ForegroundColor Cyan
Write-Host "----------------------------------------" -ForegroundColor Cyan

for ($i = 1; $i -le $Runs; $i++) {
    $output = & $Exe --gtest_filter=$Filter 2>&1 | Out-String
    $exitCode = $LASTEXITCODE

    if ($exitCode -eq 0) {
        $passed++
        Write-Host ("Run {0,4} : OK" -f $i) -ForegroundColor Green

        if ($SaveAll) {
            $output | Out-File -FilePath "$LogDir\run_$("{0:D4}" -f $i)_ok.log" -Encoding utf8
        }
    } else {
        $failed++
        $failedRuns += $i
        Write-Host ("Run {0,4} : FAILED" -f $i) -ForegroundColor Red

        $logFile = "$LogDir\run_$("{0:D4}" -f $i)_failed.log"
        $output | Out-File -FilePath $logFile -Encoding utf8

        Write-Host "    Full log saved: $logFile" -ForegroundColor DarkYellow

        $keyLines = $output -split "`n" | Where-Object {
            $_ -match "FAILED|stream truncated|Connection reset|HTTP request failed|Failure|HTTPSC:|HttpClient: HTTPS write|HttpClient: HTTPS read|SSL handshake failed|TLS handshake failed|RST|broken pipe" 
        }
        
        if ($keyLines) {
            Write-Host "    --- Key lines ---" -ForegroundColor DarkYellow
            foreach ($line in $keyLines) {
                Write-Host "    $($line.Trim())" -ForegroundColor DarkRed
            }
        }

        $lastLines = ($output -split "`n" | Where-Object { $_.Trim() -ne "" }) | Select-Object -Last 10
        if ($lastLines) {
            Write-Host "    --- Last 10 lines ---" -ForegroundColor DarkYellow
            foreach ($line in $lastLines) {
                Write-Host "    $($line.Trim())" -ForegroundColor DarkGray
            }
        }
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Total: $Runs  |  Passed: $passed  |  Failed: $failed" -ForegroundColor Yellow

if ($failed -gt 0) {
    Write-Host "Failed runs: $($failedRuns -join ', ')" -ForegroundColor Red
    Write-Host "Full logs saved to: $LogDir" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To view a failed run:" -ForegroundColor Cyan
    Write-Host "  Get-Content $LogDir\run_0001_failed.log" -ForegroundColor White
    Write-Host "  (or use 0001..$("{0:D4}" -f $Runs) for the appropriate run number)" -ForegroundColor White
}
Write-Host "========================================" -ForegroundColor Cyan

if ($failed -gt 0) { exit 1 } else { exit 0 }