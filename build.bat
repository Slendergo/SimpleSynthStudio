@echo off
setlocal

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Release

cmake -S "%~dp0." -B "%~dp0build" %CMAKE_ARGS%
if errorlevel 1 exit /b 1

cmake --build "%~dp0build" --config %CONFIG% --parallel
if errorlevel 1 exit /b 1

echo.
echo Built: %~dp0build\%CONFIG%\SimpleSynthStudio.exe