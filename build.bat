@echo off
REM Fallback build without make. Requires g++ (MinGW-w64) on PATH.
REM Usage: build.bat            -> write-protected release build
REM        build.bat WRITE      -> build with disk writing enabled

setlocal
set WRITEFLAG=
if /I "%1"=="WRITE" set WRITEFLAG=-DQEDITNA_ENABLE_WRITE

if not exist build mkdir build

set "GXX=C:\msys64\ucrt64\bin\g++.exe"
if not exist "%GXX%" (
    echo.
    echo ERROR: MinGW g++ not found at %GXX%
    echo Install MSYS2 first: winget install MSYS2.MSYS2
    exit /b 1
)

"%GXX%" -std=c++17 -municode -O2 -DNDEBUG %WRITEFLAG% -Wall -Wextra -static-libgcc -static-libstdc++ ^
    src\main.cpp ^
    src\editor\editor_core.cpp ^
    src\editor\command_engine.cpp ^
    src\io\file_io.cpp ^
    src\ui\window.cpp ^
    src\ui\status_bar.cpp ^
    src\ui\command_menu.cpp ^
    src\ui\input_box.cpp ^
    src\ui\notice_box.cpp ^
    -o build\QEditNA.exe -municode -mwindows -lgdi32 -luser32 -lshell32

if errorlevel 1 (
    echo.
    echo BUILD FAILED
    exit /b 1
)
echo.
echo Built: build\QEditNA.exe
endlocal
