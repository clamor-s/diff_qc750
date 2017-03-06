@echo off
setlocal
set PATH=%~dp0..\..\bin;%~dp0..\..\..\tfapi_sdk\bin;%PATH%

%~dp0Debug\example_mtc.exe %1 %2 %3 %4 %5 %6 %7 %8 %9

PAUSE