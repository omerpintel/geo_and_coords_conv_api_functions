@echo off
REM ============================================================
REM GeoPoint CI Test Runner - Windows Batch Wrapper
REM ============================================================
REM Usage:
REM   run_tests.bat                 - Debug build, run all tests
REM   run_tests.bat Release         - Release build
REM   run_tests.bat --all-configs   - Both Debug and Release
REM   run_tests.bat Debug --asan    - With AddressSanitizer
REM ============================================================

setlocal

REM Check if first arg is --all-configs
if "%1"=="--all-configs" (
    python "%~dp0run_tests.py" --all-configs %2 %3 %4 %5
    goto :done
)

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

echo.
echo ============================================================
echo   GeoPoint CI Test Suite - Windows
echo   Configuration: %CONFIG%
echo ============================================================
echo.

python "%~dp0run_tests.py" --config %CONFIG% %2 %3 %4 %5

:done
if %ERRORLEVEL% EQU 0 (
    echo.
    echo [OK] All validations passed.
) else (
    echo.
    echo [ERROR] One or more validations failed. See test_report*.html
)

exit /b %ERRORLEVEL%
