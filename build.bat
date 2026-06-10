@echo off
REM build.bat - DeepMetria MFC incremental build (Debug default, Release as arg)
REM Pins MSVC v14.44.35207 explicitly because v14.43 lacks the MFC component.

setlocal EnableDelayedExpansion

set CONFIG=%~1
if "%CONFIG%"=="" set CONFIG=Debug

set VS_DEV=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VS_DEV%" (
    echo [ERROR] vcvars64.bat not found: "%VS_DEV%"
    exit /b 2
)

call "%VS_DEV%" -vcvars_ver=14.44 >nul
if errorlevel 1 (
    echo [ERROR] vcvars64.bat failed. Check MSVC v14.44 installation.
    exit /b 2
)

REM Pin VCToolsVersion so MSBuild picks 14.44 (which has MFC) not 14.43.
set VCToolsVersion=14.44.35207

where msbuild >nul 2>&1
if errorlevel 1 (
    echo [ERROR] MSBuild.exe not found in PATH after vcvars.
    exit /b 2
)

echo === DeepMetria build (%CONFIG% x64, VCToolsVersion=%VCToolsVersion%) ===
msbuild DeepMetria.vcxproj /nologo /m /p:Configuration=%CONFIG% /p:Platform=x64 /p:PlatformToolset=v143 /p:VCToolsVersion=14.44.35207 /v:minimal
set RC=%ERRORLEVEL%

if %RC%==0 (
    echo === Build OK: x64\%CONFIG%\DeepMetria.exe ===
) else (
    echo === Build FAILED (rc=%RC%) ===
)
exit /b %RC%