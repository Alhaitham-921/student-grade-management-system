@echo off
cd /d "%~dp0"
mode con cols=140 lines=40 >nul 2>&1
"%~dp0x64\Debug\zuoyezuihou.exe"
pause

