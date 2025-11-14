@echo off
REM =============================================================================
REM LTRIM_ZERO Collation Build - Visual Studio
REM =============================================================================

echo.
echo ===================================================================
echo   Firebird LTRIM_ZERO Collation - Build Script
echo ===================================================================
echo.

REM Detect Visual Studio
set VS_YEAR=2022
set VS_EDITION=Community

if not exist "C:\Program Files\Microsoft Visual Studio\%VS_YEAR%\%VS_EDITION%\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_YEAR=2019
)

if not exist "C:\Program Files\Microsoft Visual Studio\%VS_YEAR%\%VS_EDITION%\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_EDITION=Professional
)

if not exist "C:\Program Files\Microsoft Visual Studio\%VS_YEAR%\%VS_EDITION%\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_EDITION=Enterprise
)

set VCVARS="C:\Program Files\Microsoft Visual Studio\%VS_YEAR%\%VS_EDITION%\VC\Auxiliary\Build\vcvars64.bat"

if not exist %VCVARS% (
    echo ERROR: Visual Studio not found!
    echo.
    echo Searched in:
    echo   C:\Program Files\Microsoft Visual Studio\2022\Community
    echo   C:\Program Files\Microsoft Visual Studio\2022\Professional
    echo   C:\Program Files\Microsoft Visual Studio\2022\Enterprise
    echo   C:\Program Files\Microsoft Visual Studio\2019\...
    echo.
    echo Install Visual Studio 2019 or 2022 with "Desktop development with C++"
    echo OR use compile_mingw.bat if you have MinGW installed
    pause
    exit /b 1
)

echo Using: Visual Studio %VS_YEAR% %VS_EDITION%
echo.

REM Set up environment
call %VCVARS% >nul 2>&1

REM Create directories
if not exist build mkdir build
if not exist dist mkdir dist

echo Compiling fbltrimzero.dll...
echo.

REM Compile
cl /nologo /LD /MD /O2 /EHsc ^
   /I include ^
   /D WIN32 /D NDEBUG /D _WINDOWS /D _USRDLL ^
   src\fbltrimzero.cpp ^
   /link /OUT:build\fbltrimzero.dll

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** COMPILATION ERROR! ***
    echo.
    pause
    exit /b 1
)

echo.
echo Copying files to dist\...

copy /Y build\fbltrimzero.dll dist\ >nul
copy /Y config\fbltrimzero.conf dist\ >nul

echo.
echo ===================================================================
echo   BUILD COMPLETED SUCCESSFULLY!
echo ===================================================================
echo.
echo Generated files in: dist\
echo   - fbltrimzero.dll
echo   - fbltrimzero.conf
echo.
echo Next steps:
echo   1. copy files manually to <firebird>\intl\
echo   2. Restart Firebird
echo.
pause
