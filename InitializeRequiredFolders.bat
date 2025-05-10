@echo off
:: Create 'Assets' directory in C:\Users\farhan\source\repos\VKModernRenderer
mkdir "%~dp0Assets"

:: Create 'Binary Scene Files' directory inside 'InitFiles' directory in current directory
mkdir "%~dp0InitFiles\Binary Scene Files"

echo Directories created successfully.
pause