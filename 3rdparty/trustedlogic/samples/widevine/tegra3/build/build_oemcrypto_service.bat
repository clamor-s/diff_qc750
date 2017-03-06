@echo off
setlocal

set PRODUCT_ROOT=C:\tl\tegra3
rem set path=C:\tl\rvct\win_32-pentium;%path%
rem set ARMLMD_LICENSE_FILE=C:\Program Files\ARM\rvds.dat
set RVCT40INC=C:\Program Files\ARM\RVCT\Data\4.0\400\include\windows
set RVCT40LIB=C:\Program Files\ARM\RVCT\Data\4.0\400\lib
set RVCT40BIN=C:\tl\rvct\win_32-pentium

cd /d %~dp0

REM Check environment variable used...
if not "%PRODUCT_ROOT%" == "" goto SET_CONFIG
echo wrong path PRODUCT_ROOT:[%PRODUCT_ROOT%]
exit /B 1

REM Set default compilation options (release and cortex A9)
:SET_CONFIG
SET OUTPUT_DIR="release"
SET DEFAULT_OPTIONS=-DARM --fpu=none -O3 -Ospace -DSSDI_NO_TRACE -DNDEBUG
SET DEBUG_OPTIONS=-DARM --fpu=none -g -O3 -DSSDI_DEBUG
SET CPU_OPTION=Cortex-A9
REM Obsolette CPU_OPTION was : ARM1176JZF-S
goto CHECKFORSWITCHES

:CHECKFORSWITCHES
REM Read script param
IF "%1"=="-debug" goto SET_DEBUG
IF "%1"=="-release" goto SET_RELEASE
IF "%1"=="-cpu" goto SET_CPU
IF "%1"=="" goto BEGIN
SHIFT
GOTO CHECKFORSWITCHES

:SET_DEBUG
SET OUTPUT_DIR="debug"
SET DEFAULT_OPTIONS=%DEBUG_OPTIONS%
@echo Building Service in debug mode
SHIFT
GOTO CHECKFORSWITCHES

:SET_RELEASE
REM In release, we do nothing, we simply keep the default options
@echo Building Service in release mode
SHIFT
GOTO CHECKFORSWITCHES

:SET_CPU
SET CPU_OPTION=%2
SHIFT
SHIFT
GOTO CHECKFORSWITCHES

REM Print environment and start build...
:BEGIN
echo.
echo -------------------------------------------------------------
echo      OEMCrypto SECURE SERVICE
echo      PRODUCT_ROOT = %PRODUCT_ROOT%
echo      compile option = %DEFAULT_OPTIONS%
echo      cpu option   = %CPU_OPTION%
echo -------------------------------------------------------------
echo.

mkdir %OUTPUT_DIR%
pushd %OUTPUT_DIR%

echo. > armcc_via.txt
set OPTIONS=%DEFAULT_OPTIONS% --cpu=%CPU_OPTION% --fpu=none --dllimport_runtime
echo %OPTIONS% >> armcc_via.txt

echo -I %PRODUCT_ROOT%\tf_sdk\include >> armcc_via.txt
echo -I ..\..\..\..\..\..\..\vendor\nvidia\tegra\core\include >> armcc_via.txt
echo -I ..\..\..\..\..\..\..\vendor\nvidia\tegra\multimedia-partner\nvmm\include >> armcc_via.txt

echo Compile oemcrypto_secure_service sources
echo -c --multifile >> armcc_via.txt
echo ..\..\src\oemcrypto_secure_service.c >> armcc_via.txt
echo --thumb -o thumb_objects.o >> armcc_via.txt
armcc --via armcc_via.txt
@if ERRORLEVEL 1 goto BUILD_ERROR

echo Compile the resources
%PRODUCT_ROOT%\tf_sdk\tools\ix86_win32\tf_resc.exe --output ../../tools/oemcrypto_secure_service.manifest.o --target elf --manifest ../../tools/oemcrypto_secure_service.manifest.txt
@if ERRORLEVEL 1 goto BUILD_ERROR

echo Linking the service oemcrypto_secure_service
echo. > armlink_via.txt
echo --bpabi --fpu=none --pltgot=none --dll --callgraph --feedback linkfeed.txt -o oemcrypto_secure_service_no_feedback.srvx ../../tools/oemcrypto_secure_service.manifest.o thumb_objects.o >> armlink_via.txt
echo %PRODUCT_ROOT%\tf_sdk\lib\arm_rvct\libssdi.so %PRODUCT_ROOT%\tf_sdk\lib\arm_rvct\libAES_CTS.a >> armlink_via.txt
armlink --via armlink_via.txt
fromelf --text -cdezasv --output oemcrypto_secure_service_no_feedback.srvx.txt oemcrypto_secure_service_no_feedback.srvx
@if ERRORLEVEL 1 goto BUILD_ERROR

echo Compile with feedback
armcc --via armcc_via.txt --feedback linkfeed.txt
@if ERRORLEVEL 1 goto BUILD_ERROR

echo Link the service oemcrypto_secure_service (after feedback)
echo. > armlink_via.txt
echo --bpabi --fpu=none --pltgot=none --dll --callgraph -o oemcrypto_secure_service.srvx ../../tools/oemcrypto_secure_service.manifest.o thumb_objects.o >> armlink_via.txt
echo %PRODUCT_ROOT%\tf_sdk\lib\arm_rvct\libssdi.so %PRODUCT_ROOT%\tf_sdk\lib\arm_rvct\libAES_CTS.a >> armlink_via.txt
armlink  --via armlink_via.txt
fromelf --text -cdezasv --output oemcrypto_secure_service.srvx.txt oemcrypto_secure_service.srvx
@if ERRORLEVEL 1 goto BUILD_ERROR
pause
popd
endlocal
goto :EOF

:BUILD_ERROR
echo.
echo Script exited on error!
pause
popd
endlocal
exit /B 1

