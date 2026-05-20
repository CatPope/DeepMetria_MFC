@echo off
REM DeepMetria MFC Build Script
REM Usage: build.bat [Debug|Release]

set CONFIG=%~1
if "%CONFIG%"=="" set CONFIG=Debug

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.44.35207 >nul 2>&1
cd /d "%~dp0"

echo.
echo === DeepMetria MFC Build (%CONFIG% x64) ===
echo.

set EXTRA=%~2
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" DeepMetria.vcxproj /p:Configuration=%CONFIG% /p:Platform=x64 /p:VCToolsVersion=14.44.35207 /m /v:minimal %EXTRA%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [FAILED] Build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo [OK] Build succeeded: x64\%CONFIG%\DeepMetria.exe
