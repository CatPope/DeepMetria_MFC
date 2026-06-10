@echo off
REM docs\build manual\build.bat - wrapper for root build.bat
REM Usage: docs\"build manual"\build.bat [Debug^|Release]

setlocal
set ROOT=%~dp0..\..
pushd "%ROOT%"
call build.bat %*
set RC=%ERRORLEVEL%
popd
exit /b %RC%
