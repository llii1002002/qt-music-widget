@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d d:\qtapp\MusicDesktop
set QT_MSVC=D:\qt\5.14.2\msvc2017_64

%QT_MSVC%\bin\qmake.exe MusicDesktop.pro -spec win32-msvc "CONFIG+=release"
if errorlevel 1 exit /b 1
D:\qt\Tools\QtCreator\bin\jom.exe -j8
if errorlevel 1 exit /b 1

REM ---- Deploy Qt DLLs and ANGLE (always from msvc2017_64, never mingw) ----
set BIN=bin
set QTSRC=%QT_MSVC%

copy /Y "%QTSRC%\bin\Qt5Core.dll"                %BIN%\  >nul
copy /Y "%QTSRC%\bin\Qt5Gui.dll"                %BIN%\  >nul
copy /Y "%QTSRC%\bin\Qt5Network.dll"            %BIN%\  >nul
copy /Y "%QTSRC%\bin\Qt5Qml.dll"                %BIN%\  >nul
copy /Y "%QTSRC%\bin\Qt5QmlModels.dll"          %BIN%\  >nul
copy /Y "%QTSRC%\bin\Qt5QmlWorkerScript.dll"    %BIN%\  >nul
copy /Y "%QTSRC%\bin\Qt5Quick.dll"              %BIN%\  >nul
copy /Y "%QTSRC%\bin\Qt5QuickControls2.dll"     %BIN%\  >nul
copy /Y "%QTSRC%\bin\Qt5QuickTemplates2.dll"    %BIN%\  >nul
copy /Y "%QTSRC%\bin\Qt5Svg.dll"                %BIN%\  >nul
copy /Y "%QTSRC%\bin\Qt5Widgets.dll"            %BIN%\  >nul
copy /Y "%QTSRC%\bin\libEGL.dll"                %BIN%\  >nul
copy /Y "%QTSRC%\bin\libGLESv2.dll"             %BIN%\  >nul

REM ---- OpenSSL (for online lyric HTTPS lookup) ----
copy /Y "D:\BaiduNetdisk\module\ImageViewer\libssl-1_1-x64.dll"    %BIN%\  >nul
copy /Y "D:\BaiduNetdisk\module\ImageViewer\libcrypto-1_1-x64.dll" %BIN%\  >nul

REM ---- Deploy plugins ----
xcopy /Y /I /Q "%QTSRC%\plugins\platforms\qwindows.dll"       %BIN%\platforms\      >nul
xcopy /Y /I /Q "%QTSRC%\plugins\iconengines\qsvgicon.dll"      %BIN%\iconengines\   >nul
xcopy /Y /I /Q "%QTSRC%\plugins\imageformats\*"                %BIN%\imageformats\  >nul
xcopy /Y /I /Q "%QTSRC%\plugins\styles\qwindowsvistastyle.dll" %BIN%\styles\        >nul
xcopy /Y /I /Q "%QTSRC%\plugins\bearer\qgenericbearer.dll"     %BIN%\bearer\        >nul

REM ---- Deploy QML plugins ----
REM Robocopy is additive and keeps going when a file is locked by a
REM running instance: never rmdir+xcopy here, a locked DLL would make
REM xcopy abort and leave modules without qmldir (app then exits with
REM "module QtQuick.Window is not installed"). Exit codes < 8 are OK.
robocopy "%QTSRC%\qml\QtQuick.2"          %BIN%\QtQuick.2          /E /XF *d.dll *.pdb /NFL /NDL /NJH /NJS /NP /R:1 /W:1
if errorlevel 8 exit /b 1
robocopy "%QTSRC%\qml\QtGraphicalEffects" %BIN%\QtGraphicalEffects /E /XF *d.dll *.pdb /NFL /NDL /NJH /NJS /NP /R:1 /W:1
if errorlevel 8 exit /b 1
robocopy "%QTSRC%\qml\QtQuick"            %BIN%\QtQuick            /E /XF *d.dll *.pdb /NFL /NDL /NJH /NJS /NP /R:1 /W:1
if errorlevel 8 exit /b 1

echo Deploy complete.
endlocal
exit /b 0
