@echo off


echo ===== Generating Visual Studio 2022 workspace...
cd ..\..
vendor\premake5\windows\premake5.exe vs2022


echo ===== Done.
pause