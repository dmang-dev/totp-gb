@echo off
REM Build wrapper for totp-gba (devkitARM + libtonc).
REM
REM Avoids PowerShell's WSL hijacking of `make` by going through cmd.exe
REM with devkitPro's MSYS2 toolchain explicitly on PATH.

setlocal

if "%DEVKITPRO%"=="" set DEVKITPRO=C:/devkitPro
if "%DEVKITARM%"=="" set DEVKITARM=%DEVKITPRO%/devkitARM
set PATH=C:\devkitPro\msys2\usr\bin;C:\devkitPro\devkitARM\bin;%PATH%

if not exist "%DEVKITARM:/=\%\bin\arm-none-eabi-gcc.exe" (
    echo ERROR: devkitARM not found at "%DEVKITARM%".
    echo Install devkitPro from https://github.com/devkitPro/installer/releases
    exit /b 1
)

cd /d "%~dp0"

if "%1"=="clean" (
    make clean
    exit /b %ERRORLEVEL%
)

make %*
if errorlevel 1 (
    echo.
    echo Build FAILED.
    exit /b 1
)

REM Publish to ../artifacts/
if not exist "%~dp0..\artifacts" mkdir "%~dp0..\artifacts"
copy /Y totp-gba.gba "%~dp0..\artifacts\totp-gba.gba" >nul

echo.
echo Build OK. (ROM published to ..\artifacts\totp-gba.gba)
endlocal
