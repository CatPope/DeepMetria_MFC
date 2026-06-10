@echo off
REM docs\build manual\run.bat - launch built DeepMetria.exe
REM Usage: docs\"build manual"\run.bat [Debug^|Release]

setlocal
set CONFIG=%~1
if "%CONFIG%"=="" set CONFIG=Debug

set ROOT=%~dp0..\..
set EXE=%ROOT%\x64\%CONFIG%\DeepMetria.exe

if not exist "%EXE%" (
    echo [ERROR] Executable not found: %EXE%
    echo         Run docs\"build manual"\build.bat %CONFIG% first.
    exit /b 1
)

echo === DeepMetria run (%CONFIG% x64) ===
echo EXE: %EXE%
echo.

pushd "%ROOT%"
start "" "%EXE%"
popd
exit /b 0
