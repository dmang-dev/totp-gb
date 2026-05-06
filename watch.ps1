# Watch src\ for changes and rebuild on save.
# Polls every second; runs build.bat once when any tracked file's
# LastWriteTime advances. Includes a debounce so a burst of edits
# (e.g. an editor saving multiple files) only fires one rebuild.

param(
    [string]$Target = ""   # passed through to build.bat: "", "dmg", or "gbc"
)

$ErrorActionPreference = "Stop"
$root      = $PSScriptRoot
$watchDir  = Join-Path $root "src"
$buildBat  = Join-Path $root "build.bat"
$debounceS = 1

if (-not (Test-Path $watchDir)) {
    Write-Host "ERROR: $watchDir not found." -ForegroundColor Red
    exit 1
}

function Get-MaxMtime {
    (Get-ChildItem $watchDir -Recurse -File -Include *.c, *.h |
        Measure-Object -Property LastWriteTimeUtc -Maximum).Maximum
}

function Invoke-Build {
    Write-Host ""
    Write-Host ("[{0}] change detected -> rebuilding..." -f (Get-Date -Format HH:mm:ss)) -ForegroundColor Cyan
    & cmd /c "`"$buildBat`" $Target"
    if ($LASTEXITCODE -eq 0) {
        Write-Host ("[{0}] OK" -f (Get-Date -Format HH:mm:ss)) -ForegroundColor Green
    } else {
        Write-Host ("[{0}] FAILED (exit {1})" -f (Get-Date -Format HH:mm:ss), $LASTEXITCODE) -ForegroundColor Red
    }
}

Write-Host "Watching $watchDir ... (Ctrl+C to stop)" -ForegroundColor Yellow
$last = Get-MaxMtime

# Initial build so the user starts from a known-good state
Invoke-Build

while ($true) {
    Start-Sleep -Seconds $debounceS
    $now = Get-MaxMtime
    if ($now -gt $last) {
        # Debounce: wait until writes stop arriving before rebuilding
        do {
            $last = $now
            Start-Sleep -Seconds $debounceS
            $now = Get-MaxMtime
        } while ($now -gt $last)

        Invoke-Build
        $last = Get-MaxMtime
    }
}
