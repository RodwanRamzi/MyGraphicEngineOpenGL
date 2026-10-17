@echo off
title Building Rodwan Engine (NMake)...

REM ============================================================
REM  مسار VS Build Tools
REM ============================================================
set "VSDEVCMD=F:\Microsoft Visual Studio 2022\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

REM ============================================================
REM  مسار المشروع
REM ============================================================
set "PROJECT_DIR=F:\MY C++ Projects\MyGraphicEngineOpenGL"
set "BUILD_DIR=%PROJECT_DIR%\build"

REM ============================================================
REM  تهيئة البيئة
REM ============================================================
echo ========================================
echo   Initializing MSVC environment...
echo ========================================
call "%VSDEVCMD%" -arch=x64 -host_arch=x64 >nul 2>&1

where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] cl.exe not found!
    pause
    exit /b 1
)

REM ============================================================
REM  الانتقال للمشروع
REM ============================================================
cd /d "%PROJECT_DIR%"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM ============================================================
REM  Configure (لو محتاج)
REM ============================================================
if not exist "CMakeCache.txt" (
    echo ========================================
    echo   CMake Configure...
    echo ========================================
    cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
    if %errorlevel% neq 0 (
        echo [ERROR] CMake configure failed!
        pause
        exit /b 1
    )
)

REM ============================================================
REM  Build
REM ============================================================
echo ========================================
echo   Building MyGraphicEngineOpenGL...
echo ========================================
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] BUILD FAILED!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   BUILD COMPLETE!
echo ========================================
echo.
echo Output:
if exist "%BUILD_DIR%\MyGraphicEngineOpenGL.exe" (
    echo   %BUILD_DIR%\MyGraphicEngineOpenGL.exe
) else (
    echo   [WARN] exe not found
)
pause