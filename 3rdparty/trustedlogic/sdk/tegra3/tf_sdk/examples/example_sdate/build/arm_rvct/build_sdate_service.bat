echo OFF

SET DEBUG_OPTIONS=-O3 -DSSDI_NO_TRACE
SET CPU_OPTION=Cortex-A8
SET VARIANT=release

:CHECKFORSWITCHES
IF "%1"=="-debug" goto SET_DEBUG
IF "%1"=="-release" goto SET_RELEASE
IF "%1"=="-cpu" goto SET_CPU
goto BEGIN


:SET_DEBUG
SET VARIANT=debug
SET DEBUG_OPTIONS=-g -O0 -DSSDI_DEBUG
SHIFT
GOTO CHECKFORSWITCHES

:SET_RELEASE
SET VARIANT=release
SET DEBUG_OPTIONS=-O3 -DSSDI_NO_TRACE
SHIFT
GOTO CHECKFORSWITCHES

:SET_CPU
SET CPU_OPTION=%2
SHIFT
SHIFT
GOTO CHECKFORSWITCHES

:BEGIN

echo.
echo -------------------------------------------------------------
echo    SDATE SERVICE
echo      debug option = %DEBUG_OPTIONS%
echo      cpu option = %CPU_OPTION%
echo -------------------------------------------------------------
echo.

mkdir %VARIANT%
pushd %VARIANT%
echo. > armcc_via.txt

set OPTIONS=%DEBUG_OPTIONS% --cpu=%CPU_OPTION% --fpu=none --dllimport_runtime 

echo %OPTIONS% >> armcc_via.txt

echo -I ..\..\..\..\..\include >> armcc_via.txt
echo -c ..\..\..\src\sdate_service.c >> armcc_via.txt
echo --thumb >> armcc_via.txt

echo Compile sdate_service.c

armcc --via armcc_via.txt
@if ERRORLEVEL 1 goto build_error

echo Compile the resources (sdate_service_a)
..\..\..\..\..\tools\ix86_win32\tf_resc.exe --output sdate_service_a.manifest.o --target elf --manifest ..\..\..\src\sdate_service_a.manifest.txt

echo Compile the resources (sdate_service_b)
..\..\..\..\..\tools\ix86_win32\tf_resc.exe --output sdate_service_b.manifest.o --target elf --manifest ..\..\..\src\sdate_service_b.manifest.txt

echo Linking the first service
echo. > armlink_via.txt
echo --bpabi --fpu=none --pltgot=none --dll -o sdate_service_a.srvx sdate_service_a.manifest.o sdate_service.o >> armlink_via.txt
echo ..\..\..\..\..\lib\arm_rvct\libssdi.so >> armlink_via.txt
armlink  --via armlink_via.txt

echo Linking the second service
echo. > armlink_via.txt
echo --bpabi --fpu=none --pltgot=none --dll -o sdate_service_b.srvx sdate_service_b.manifest.o sdate_service.o >> armlink_via.txt
echo ..\..\..\..\..\lib\arm_rvct\libssdi.so >> armlink_via.txt
armlink  --via armlink_via.txt

popd
