@echo off
setlocal
set PATH=%~dp0..\..\bin;%PATH%

%~dp0Debug\tfctrl.exe %1 %2 %3 %4 %5 %6 %7 %8 %9

