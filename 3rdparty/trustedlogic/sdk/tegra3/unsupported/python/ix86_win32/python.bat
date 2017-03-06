@echo off
setlocal
set PYTHONHOME=%~dp0
set PYTHONPATH=%PYTHONHOME%..\lib\python25.zip;%PYTHONHOME%..\DLLs;%PYTHONHOME%..\..\pywin32\Lib\win32;%PYTHONHOME%..\..\pywin32\Lib\win32\lib.zip;%PYTHONHOME%..\..\pywin32\Lib;%PYTHONHOME%..\..\pywin32\Lib\win32comext;%PYTHONHOME%..\..\pyserial-2.2\serial.zip;
set PATH=%PYTHONHOME%;%PYTHONHOME%..\..\pywin32;%PATH%
%~dp0python.exe %*
