@echo off

rem Generates the NSIS installer (Resources\Install.nsi to dist\Copilot Key+ - Install.exe).
rem Standalone script, to be run manually (double-click or from the terminal).

setlocal

set "NSIS_ROOT=%ProgramFiles(x86)%\NSIS\Bin"
if not exist "%NSIS_ROOT%\makensis.exe" set "NSIS_ROOT=%ProgramFiles%\NSIS\Bin"

echo.
echo NSIS : "%NSIS_ROOT%\makensis.exe"
echo.
echo Please wait...
echo.

"%NSIS_ROOT%\makensis.exe" /V3 /PAUSE "%~dp0Resources\Install.nsi"

exit /b %ERRORLEVEL%
