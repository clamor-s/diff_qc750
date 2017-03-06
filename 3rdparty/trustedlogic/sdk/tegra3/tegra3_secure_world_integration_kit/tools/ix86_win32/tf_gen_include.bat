@echo off
setlocal

cd %~dp0
title Generate TF include
echo.


REM Check optionnal env var.
if exist "%JAVA_HOME%" (
echo Using JAVA_HOME : %JAVA_HOME%
set PATH=%JAVA_HOME%;%PATH%
)

REM Check command line parameter, else display help message.
:CHECK_ARGS
if "%1" == "" goto HELP_MESSAGE
if NOT "%2" == "" goto HELP_MESSAGE
if exist "%1" goto RUN_CMD

:HELP_MESSAGE
echo This script will generate an include header file with inside the binary code of the Trusted Foundations.
echo Usage :
echo %0 [PATH_TO_TRUSTED_FOUNDATIONS_BINARY]
goto END



:RUN_CMD
set TF_BIN_NAME=trusted_foundations.bin
copy %1 %TF_BIN_NAME%
java -jar ..\bin2src.jar -verbose -c99 -header ..\bin2src_header.txt -o tf_include.h %TF_BIN_NAME%
del %TF_BIN_NAME%

:END
echo.

endlocal