# ============================================================================
# Flight Simulator Launcher Script
# ============================================================================
# This script launches all required applications for flight simulation
# It checks if apps are already running and only starts them if needed
# ============================================================================

# CONFIGURATION - Edit this section to enable/disable apps
# Comment out (add # at start) any app you don't want to launch
$appsToLaunch = @(
    "nginx"
    "MsfsApiServer"
    "AddLvarTrayapp"
    "MobiFlight Connector"
    "Navigraph Charts - kopie"
    "Microsoft Flight Simulator 2024"
)

# Delay in seconds between launching each app (adjust if needed)
$delayBetweenApps = 3

# Delay before launching MSFS (to let support apps initialize)
$delayBeforeMSFS = 5

# ============================================================================
# DO NOT EDIT BELOW THIS LINE (unless you know what you're doing)
# ============================================================================

$desktopPath = [Environment]::GetFolderPath("Desktop")
$shell = New-Object -ComObject WScript.Shell

Write-Host "`n================================================================" -ForegroundColor Cyan
Write-Host "        Flight Simulator Launcher v1.2                          " -ForegroundColor Cyan
Write-Host "================================================================`n" -ForegroundColor Cyan

function Get-ProcessNameFromPath {
    param([string]$path)
    if ($path -and (Test-Path $path)) {
        return [System.IO.Path]::GetFileNameWithoutExtension($path)
    }
    return $null
}

function Is-ProcessRunning {
    param([string]$processName)
    if ([string]::IsNullOrEmpty($processName)) { return $false }
    $process = Get-Process -Name $processName -ErrorAction SilentlyContinue
    return $null -ne $process
}

function Start-AppFromShortcut {
    param(
        [string]$shortcutName,
        [int]$delay,
        [bool]$isMSFS = $false
    )
    
    $shortcutPath = Join-Path $desktopPath "$shortcutName.lnk"
    
    if (-not (Test-Path $shortcutPath)) {
        Write-Host "  X Shortcut not found: $shortcutName" -ForegroundColor Red
        Write-Host "    Expected: $shortcutPath" -ForegroundColor DarkGray
        return $false
    }
    
    try {
        $shortcut = $shell.CreateShortcut($shortcutPath)
        $targetPath = $shortcut.TargetPath
        $workingDir = $shortcut.WorkingDirectory
        $arguments = $shortcut.Arguments
        
        # Check if this is a UWP/Microsoft Store app (no target path)
        $isUWPApp = [string]::IsNullOrEmpty($targetPath)
        
        if ($isUWPApp) {
            # For UWP apps, just launch the shortcut directly
            Write-Host "  > Starting (UWP app): $shortcutName" -ForegroundColor Green
            Write-Host "    Launching via shortcut..." -ForegroundColor DarkGray
            
            # Launch detached using cmd.exe
            Start-Process "cmd.exe" -ArgumentList "/c start `"`" `"$shortcutPath`"" -WindowStyle Hidden
            
            # Delay before next app (longer for MSFS)
            if ($delay -gt 0) {
                $actualDelay = if ($isMSFS) { $delayBeforeMSFS } else { $delay }
                if ($actualDelay -gt 0) {
                    Write-Host "    Waiting $actualDelay seconds..." -ForegroundColor DarkGray
                    Start-Sleep -Seconds $actualDelay
                }
            }
            
            return $true
        }
        
        # Regular app with executable path
        $processName = Get-ProcessNameFromPath $targetPath
        
        if (-not $processName) {
            Write-Host "  ! Cannot determine process name: $shortcutName" -ForegroundColor Yellow
            Write-Host "    Target: $targetPath" -ForegroundColor DarkGray
            return $false
        }
        
        # Check if already running
        if (Is-ProcessRunning $processName) {
            Write-Host "  O Already running: $shortcutName" -ForegroundColor DarkYellow
            Write-Host "    Process: $processName" -ForegroundColor DarkGray
            return $true
        }
        
        # Start the application
        Write-Host "  > Starting: $shortcutName" -ForegroundColor Green
        Write-Host "    Process: $processName" -ForegroundColor DarkGray
        
        $startParams = @{
            FilePath = $targetPath
        }
        
        if ($arguments) {
            $startParams.ArgumentList = $arguments
        }
        
        if ($workingDir -and (Test-Path $workingDir)) {
            $startParams.WorkingDirectory = $workingDir
        }
        
        # Launch detached from PowerShell console using cmd.exe to prevent apps from closing when PS window closes
        # This is especially important for apps like Navigraph Charts that write to console
        $cmdArgs = "/c start `"`" `"$targetPath`""
        if ($arguments) {
            $cmdArgs += " $arguments"
        }
        Start-Process "cmd.exe" -ArgumentList $cmdArgs -WindowStyle Hidden -WorkingDirectory $(if ($workingDir -and (Test-Path $workingDir)) { $workingDir } else { (Get-Location).Path })
        
        # Delay before next app (longer for MSFS)
        if ($delay -gt 0) {
            $actualDelay = if ($isMSFS) { $delayBeforeMSFS } else { $delay }
            if ($actualDelay -gt 0) {
                Write-Host "    Waiting $actualDelay seconds..." -ForegroundColor DarkGray
                Start-Sleep -Seconds $actualDelay
            }
        }
        
        return $true
    }
    catch {
        Write-Host "  X Error starting $shortcutName" -ForegroundColor Red
        Write-Host "    Error: $($_.Exception.Message)" -ForegroundColor DarkGray
        return $false
    }
}

# Main execution
Write-Host "Starting support applications...`n" -ForegroundColor White

$successCount = 0
$totalApps = $appsToLaunch.Count

for ($i = 0; $i -lt $totalApps; $i++) {
    $appName = $appsToLaunch[$i]
    $isMSFS = $appName -like "*Microsoft Flight Simulator*"
    $isLast = ($i -eq ($totalApps - 1))
    
    Write-Host "[$($i+1)/$totalApps] " -NoNewline -ForegroundColor Cyan
    
    if (Start-AppFromShortcut -shortcutName $appName -delay $delayBetweenApps -isMSFS $isMSFS) {
        $successCount++
    }
    
    Write-Host ""
}

# Summary
Write-Host "`n================================================================" -ForegroundColor Cyan
Write-Host "  Launch Summary                                               " -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  Successfully started: $successCount / $totalApps apps" -ForegroundColor $(if ($successCount -eq $totalApps) { "Green" } else { "Yellow" })

if ($successCount -lt $totalApps) {
    Write-Host "`n  ! Some applications failed to start." -ForegroundColor Yellow
    Write-Host "    Check the messages above for details." -ForegroundColor DarkGray
}

Write-Host "`n  Press any key to close..." -ForegroundColor DarkGray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
