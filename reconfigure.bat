@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
rmdir /s /q "C:\Users\Administrator\source\repos\tdesktop\out\CMakeFiles"
del /q "C:\Users\Administrator\source\repos\tdesktop\out\CMakeCache.txt" 2>nul
set QT=C:\Qt\5.15.1\msvc2019_64
cd /d C:\Users\Administrator\source\repos\tdesktop\out
cmake .. -G "Visual Studio 17 2022" -A x64 -T host=x64
