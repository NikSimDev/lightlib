:: SPDX-License-Identifier: LGPL-3.0-or-later
@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

set PROJECT_NAME=lightlib
set BUILD_DIR=build
set SOURCE_DIR=%~dp0
set CONFIG=Release
if defined VCPKG_ROOT (
    set VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
) else (
    set VCPKG_TOOLCHAIN=C:\Tools\vcpkg\scripts\buildsystems\vcpkg.cmake
)

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

cd /d "%BUILD_DIR%"

if exist "%PROJECT_NAME%.exe" (
    ren "%PROJECT_NAME%.exe" "%PROJECT_NAME%_OLD.exe"
)

cmake -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%" %SOURCE_DIR%
cmake --build . --config %CONFIG%
if %errorlevel% neq 0 (
    echo CMake build failed.
    exit /b %errorlevel%
)

if exist "%SOURCE_DIR%.env" (
    copy "%SOURCE_DIR%.env" "%CONFIG%\" >nul
    echo File .env copied to %CONFIG%
) else (
    echo Warning: File .env not found in source directory.
)

if exist "%CONFIG%\%PROJECT_NAME%.exe" (
    echo Running %PROJECT_NAME%.exe...
    "%CONFIG%\%PROJECT_NAME%.exe"
) else (
    echo The file %PROJECT_NAME%.exe was not found.
)

cd /d "%SOURCE_DIR%"