<#!
.SYNOPSIS
    Watches this project and automatically commits and pushes changes.

.DESCRIPTION
    The watcher polls Git status so it does not react to Git's own files
    under .git. Changes are debounced, staged with git add --all, committed,
    and pushed through the configured upstream. It never force-pushes.
#>
[CmdletBinding()]
param(
    [int]$PollSeconds = 5,
    [int]$DebounceSeconds = 3,
    [int]$RetrySeconds = 30
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ProjectRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$LogDirectory = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'Impact_Resistance_Test'
$LogPath = Join-Path $LogDirectory 'auto-git-watch.log'
$MutexName = 'Local\Impact_Resistance_Test_AutoGit'

New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null

function Write-Log {
    param([Parameter(Mandatory)][string]$Message)

    $timestamp = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
    Add-Content -LiteralPath $LogPath -Value "$timestamp $Message" -Encoding UTF8
}

function Invoke-Git {
    param([Parameter(Mandatory)][string[]]$Arguments)

    $previousPromptSetting = $env:GIT_TERMINAL_PROMPT
    $previousInteractiveSetting = $env:GCM_INTERACTIVE
    $env:GIT_TERMINAL_PROMPT = '0'
    $env:GCM_INTERACTIVE = 'Never'
    try {
        $output = @(& git -C $ProjectRoot @Arguments 2>&1)
        return [pscustomobject]@{
            ExitCode = $LASTEXITCODE
            Output = $output
        }
    }
    catch {
        return [pscustomobject]@{
            ExitCode = 1
            Output = @($_.Exception.Message)
        }
    }
    finally {
        if ($null -eq $previousPromptSetting) {
            Remove-Item Env:GIT_TERMINAL_PROMPT -ErrorAction SilentlyContinue
        }
        else {
            $env:GIT_TERMINAL_PROMPT = $previousPromptSetting
        }
        if ($null -eq $previousInteractiveSetting) {
            Remove-Item Env:GCM_INTERACTIVE -ErrorAction SilentlyContinue
        }
        else {
            $env:GCM_INTERACTIVE = $previousInteractiveSetting
        }
    }
}

function Get-StatusLines {
    $result = Invoke-Git -Arguments @('status', '--porcelain=v1', '--untracked-files=all')
    if ($result.ExitCode -ne 0) {
        Write-Log "git status failed with exit code $($result.ExitCode)"
        return @()
    }

    return @($result.Output | Where-Object { $_ -is [string] -and $_.Trim().Length -gt 0 })
}

function Test-StagedChanges {
    $result = Invoke-Git -Arguments @('diff', '--cached', '--quiet')
    if ($result.ExitCode -eq 1) {
        return $true
    }
    if ($result.ExitCode -ne 0) {
        Write-Log "git diff check failed with exit code $($result.ExitCode)"
    }
    return $false
}

function Try-Push {
    $result = Invoke-Git -Arguments @('push')
    if ($result.ExitCode -eq 0) {
        Write-Log 'push succeeded'
        return $true
    }

    Write-Log "push failed with exit code $($result.ExitCode); local commit retained"
    return $false
}

$mutex = New-Object System.Threading.Mutex($false, $MutexName)
$mutexOwned = $false

try {
    if (-not $mutex.WaitOne(0)) {
        Write-Log 'another watcher is already running; exiting'
        exit 0
    }
    $mutexOwned = $true
    Write-Log "watcher started for $ProjectRoot"

    $pendingPush = $false

    while ($true) {
        $statusLines = Get-StatusLines

        if ($statusLines.Count -gt 0) {
            Start-Sleep -Seconds $DebounceSeconds
            $statusLines = Get-StatusLines

            if ($statusLines.Count -gt 0) {
                $addResult = Invoke-Git -Arguments @('add', '--all')
                if ($addResult.ExitCode -ne 0) {
                    Write-Log "git add failed with exit code $($addResult.ExitCode)"
                }
                elseif (Test-StagedChanges) {
                    $commitMessage = "auto: sync $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
                    $commitResult = Invoke-Git -Arguments @('commit', '-m', $commitMessage)
                    if ($commitResult.ExitCode -eq 0) {
                        Write-Log "commit created: $commitMessage"
                        $pendingPush = $true
                    }
                    else {
                        Write-Log "git commit failed with exit code $($commitResult.ExitCode)"
                    }
                }
            }
        }

        if ($pendingPush) {
            if (Try-Push) {
                $pendingPush = $false
            }
            else {
                Start-Sleep -Seconds $RetrySeconds
                continue
            }
        }

        Start-Sleep -Seconds $PollSeconds
    }
}
catch {
    Write-Log "watcher stopped unexpectedly: $($_.Exception.Message)"
    exit 1
}
finally {
    if ($mutexOwned) {
        $mutex.ReleaseMutex()
    }
    $mutex.Dispose()
}
