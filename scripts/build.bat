@echo off
setlocal

set CONFIG=%1

if "%CONFIG%"=="" (
    echo Usage: build.bat [debug^|release]
    exit /b 1
)

if /i "%CONFIG%"=="debug" (
    set PRESET=windows-debug
) else if /i "%CONFIG%"=="release" (
    set PRESET=windows-release
) else (
    echo Invalid configuration: %CONFIG%
    echo Usage: build.bat [debug^|release]
    exit /b 1
)

REM Setup VS Developer Environment if cl.exe is not already available
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    )
)

REM Navigate to project root (parent of scripts/)
pushd "%~dp0.."

echo === Configuring %CONFIG% ===
cmake --preset %PRESET%
if %ERRORLEVEL% neq 0 (
    echo Configuration failed.
    popd
    exit /b %ERRORLEVEL%
)

echo === Building %CONFIG% ===
cmake --build --preset %PRESET%
if %ERRORLEVEL% neq 0 (
    echo Build failed.
    popd
    exit /b %ERRORLEVEL%
)

echo === Build complete: %CONFIG% ===
popd
