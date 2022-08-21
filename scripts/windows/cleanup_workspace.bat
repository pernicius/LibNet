@echo off


echo ===== Cleanup Visual Studio 2022 workspace...
cd ..\..
echo Removing folder: "./bin"
rmdir /s /q bin
echo Removing folder: "./build"
rmdir /s /q build


echo ===== Done.
pause