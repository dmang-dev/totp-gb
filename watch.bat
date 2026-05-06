@echo off
REM Watch src\ for changes and rebuild both ROMs when anything changes.
REM Press Ctrl+C to stop.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0watch.ps1" %*
