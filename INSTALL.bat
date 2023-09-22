@echo off

set old_dir=%cd%

cd %~dp0

REM create build directory
if not exist build (
	mkdir build
	if "%errorlevel%" NEQ "0" goto end
)

REM go to build
cd build
if "%errorlevel%" NEQ "0" goto end

REM cmake
set flagdebug="off"
if "%1%"=="debug" set flagdebug="on"
if "%2%"=="debug" set flagdebug="on"
if %flagdebug%=="on" (
	set flagdebug="-DCMAKE_BUILD_TYPE=Debug"
) else (
	set flagdebug="-DCMAKE_BUILD_TYPE=Release"
)

echo 'cmake .. -G "Ninja" %flagdebug%
cmake .. -G "Ninja" %flagdebug%
if "%errorlevel%" NEQ "0" goto end

REM make
ninja
if "%errorlevel%" NEQ "0" goto end

REM install
mkdir bin
copy src\hashahead\hashahead.exe bin\
copy src\hashahead\*.dll bin\

echo %%~dp0\hashahead.exe console %%* > bin\hashahead-cli.bat
echo %%~dp0\hashahead.exe -daemon %%* > bin\hashahead-server.bat

echo 'Installed to build\bin\'
echo ''
echo 'Usage:'
echo 'Run build\bin\hashahead.exe to launch hashahead'
echo 'Run build\bin\hashahead-cli.bat to launch hashahead RPC console'
echo 'Run build\bin\hashahead-server.bat to launch hashahead server on background'
echo 'Run build\test\test_big.exe to launch test program.'
echo 'Default `-datadir` is the same folder of hashahead.exe'

echo 'Installed to build\bin\'
echo ''
echo 'Usage:'
echo 'Run build\bin\hashahead.exe to launch hashahead'
echo 'Run build\bin\hashahead-console.bat to launch hashahead-console'
echo 'Run build\bin\hashahead-server.bat to launch hashahead server on background'
echo 'Run build\test\test_big.exe to launch test program.'

:end

cd %old_dir%
