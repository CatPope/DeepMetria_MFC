@echo off
REM docs\build manual\rebuild.bat - wrapper for root rebuild.bat (Debug x64 clean rebuild)
REM Build log: ^<root^>\build_rebuild.log

setlocal
set ROOT=%~dp0..\..
pushd "%ROOT%"
call rebuild.bat
set RC=%ERRORLEVEL%
popd
exit /b %RC%
