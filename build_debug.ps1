Set-ExecutionPolicy -Scope Process Bypass -Force
$vsDevShell = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Import-Module $vsDevShell
Enter-VsDevShell -VsInstallPath "C:\Program Files\Microsoft Visual Studio\18\Community" -SkipAutomaticLocation -DevCmdArguments "-arch=x64" | Out-Null
Set-Location d:\qtapp\MusicDesktop
Write-Host "=== qmake ==="
& D:\qt\5.14.2\msvc2017_64\bin\qmake.exe MusicDesktop.pro -spec win32-msvc "CONFIG+=debug"
if ($LASTEXITCODE -ne 0) { exit 1 }
Write-Host "=== jom build ==="
& D:\qt\Tools\QtCreator\bin\jom.exe -j8
exit $LASTEXITCODE
