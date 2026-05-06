@echo off
REM Build both DMG (.gb) and CGB (.gbc) variants.
REM
REM Usage:   build.bat            (build both)
REM          build.bat dmg        (DMG only)
REM          build.bat gbc        (GBC only)

setlocal

if "%GBDK_DIR%"=="" set GBDK_DIR=%~dp0gbdk

if not exist "%GBDK_DIR%\bin\lcc.exe" (
    echo ERROR: GBDK-2020 not found at "%GBDK_DIR%".
    exit /b 1
)

set CFLAGS=-Wf-MMD
REM Cartridge: MBC3+TIMER+RAM+BATTERY (0x10), 1 RAM bank (8KB), 2 ROM banks
REM SGB flag (-Wm-ys) goes through makebin, not the linker.
set LFLAGS=-Wl-yt0x10 -Wl-ya1 -Wl-yo2
set MFLAGS=-Wm-ys

set SRCS=src\main.c src\sha1.c src\hmac.c src\base32.c src\rtc.c src\storage.c src\totp.c src\input.c src\ui.c

set TARGET=%1
if "%TARGET%"=="" set TARGET=both

if "%TARGET%"=="dmg" goto build_dmg
if "%TARGET%"=="gbc" goto build_gbc

:build_dmg
echo [DMG] Building totp-gb.gb ...
"%GBDK_DIR%\bin\lcc.exe" %CFLAGS% %LFLAGS% %MFLAGS% -o totp-gb.gb %SRCS%
if errorlevel 1 goto fail
if "%TARGET%"=="dmg" goto ok

:build_gbc
echo [GBC] Building totp-gbc.gbc ...
REM -Wm-yc = CGB-compatible mode (header byte 0x143 = 0x80) ??? runs in
REM          full colour on CGB, falls back to DMG mode on SGB so the
REM          SGB palette flag still works there.
REM -DGBC_BUILD enables CGB palette setup in main.c
"%GBDK_DIR%\bin\lcc.exe" %CFLAGS% %LFLAGS% %MFLAGS% -Wm-yc -Wf-DGBC_BUILD -o totp-gbc.gbc %SRCS%
if errorlevel 1 goto fail

:ok
echo.
echo Build OK.
endlocal
exit /b 0

:fail
echo.
echo Build FAILED.
endlocal
exit /b 1
