@echo off
REM DeepMetria MFC Rebuild Script (Clean + Build, Debug x64)
REM Usage: rebuild.bat
REM Output log: build_rebuild.log

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.44.35207 >nul 2>&1
cd /d "%~dp0"

echo.
echo === DeepMetria MFC Rebuild (Debug x64) ===
echo === Log: build_rebuild.log ===
echo.

"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" DeepMetria.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /p:VCToolsVersion=14.44.35207 /m /v:minimal > build_rebuild.log 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [FAILED] Rebuild failed with error code %ERRORLEVEL%
    echo          See build_rebuild.log for details.
    exit /b %ERRORLEVEL%
)

echo.
echo [OK] Rebuild succeeded: x64\Debug\DeepMetria.exe
