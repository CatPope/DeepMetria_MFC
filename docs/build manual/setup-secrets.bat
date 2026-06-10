@echo off
REM docs\build manual\setup-secrets.bat - register API keys via DPAPI to registry
REM Prereq: edit ^<root^>\secrets.json (copy of secrets.json.template)
REM On success, secrets.json is overwritten with zeros and deleted.

setlocal
set ROOT=%~dp0..\..
set SECRETS=%ROOT%\secrets.json

if not exist "%SECRETS%" (
    echo [ERROR] secrets.json not found at: %SECRETS%
    echo         Copy secrets.json.template to secrets.json, fill in keys, then retry.
    exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\scripts\setup-secrets.ps1"
set RC=%ERRORLEVEL%
exit /b %RC%
