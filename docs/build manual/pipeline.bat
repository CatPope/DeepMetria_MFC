@echo off
REM docs\build manual\pipeline.bat - wrapper for pipeline.py
REM Usage:
REM   docs\"build manual"\pipeline.bat                   (full: build + test + analyze)
REM   docs\"build manual"\pipeline.bat --build-only
REM   docs\"build manual"\pipeline.bat --test-only
REM   docs\"build manual"\pipeline.bat --config Release
REM   docs\"build manual"\pipeline.bat --suite file_load

setlocal
set ROOT=%~dp0..\..
pushd "%ROOT%"
python pipeline.py %*
set RC=%ERRORLEVEL%
popd
exit /b %RC%
