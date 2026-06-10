@echo off
REM rebuild.bat - Clean + Debug x64 full rebuild. Full log: build_rebuild.log.

setlocal EnableDelayedExpansion

set VS_DEV=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VS_DEV%" (
    echo [ERROR] vcvars64.bat not found: "%VS_DEV%"
    exit /b 2
)

call "%VS_DEV%" -vcvars_ver=14.44 >nul
if errorlevel 1 (
    echo [ERROR] vcvars64.bat failed.
    exit /b 2
)

set LOG=%~dp0build_rebuild.log

echo === DeepMetria rebuild (Debug x64) ===
echo Full log: %LOG%

(
    echo === %DATE% %TIME% : rebuild start ===
    msbuild DeepMetria.vcxproj /nologo /t:Clean /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=v143 /v:minimal
    msbuild DeepMetria.vcxproj /nologo /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=v143 /v:normal
) > "%LOG%" 2>&1
set RC=%ERRORLEVEL%

powershell -NoProfile -Command "Get-Content -LiteralPath '%LOG%' -Tail 200"

if %RC%==0 (
    echo === Rebuild OK: x64\Debug\DeepMetria.exe ===
) else (
    echo === Rebuild FAILED (rc=%RC%). See %LOG% ===
)
exit /b %RC%