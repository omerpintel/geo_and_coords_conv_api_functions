@echo off
REM ============================================================
REM GeoPoint CI Test Runner - Windows Batch Wrapper
REM ============================================================
REM Usage:
REM   run_tests.bat                 - Debug build, run all tests
REM   run_tests.bat Release         - Release build
REM   run_tests.bat Debug --asan    - With AddressSanitizer
REM ============================================================

setlocal

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

echo.
echo ============================================================
echo   GeoPoint CI Test Suite - Windows
echo   Configuration: %CONFIG%
echo ============================================================
echo.

python "%~dp0run_tests.py" --config %CONFIG% %2 %3 %4 %5

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [OK] All validations passed.
) else (
    echo.
    echo [ERROR] One or more validations failed. See test_report.html
)

exit /b %ERRORLEVEL%
