@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d d:\qtapp\MusicDesktop
D:\qt\5.14.2\msvc2017_64\bin\qmake.exe MusicDesktop.pro -spec win32-msvc "CONFIG+=debug"
if errorlevel 1 exit /b 1
D:\qt\Tools\QtCreator\bin\jom.exe -j8
exit /b %errorlevel%
