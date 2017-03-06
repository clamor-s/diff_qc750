@echo off
setlocal

SET DEBUG_OPTIONS=-O3 -DSSDI_NO_TRACE
SET CPU_OPTION=CORTEX-A9
SET VARIANT=release

title Tegra3 UART traces DRIVER

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
echo    Tegra traces DRIVER
echo      debug option = %DEBUG_OPTIONS%
echo      cpu option = %CPU_OPTION%
echo -------------------------------------------------------------
echo.

mkdir %VARIANT%
pushd %VARIANT%
echo. > armcc_via.txt

set OPTIONS=%DEBUG_OPTIONS% --cpu=%CPU_OPTION% --fpu=none

echo %OPTIONS% >> armcc_via.txt


echo -I ..\..\..\..\..\include >> armcc_via.txt
echo -c ..\..\..\src\sdrv_trace_uart.c >> armcc_via.txt

echo Compile sdrv_trace.c

armcc --via armcc_via.txt
@if ERRORLEVEL 1 goto build_error

echo Compile the resources
..\..\..\..\..\tools\ix86_win32\tf_resc.exe  --output sdrv_trace.manifest.o --target elf --manifest ..\..\..\src\sdrv_trace_uart.manifest.txt
@if ERRORLEVEL 1 goto build_error

echo Linking the driver
echo. > armlink_via.txt
echo --callgraph --bpabi --fpu=none --pltgot=none --dll -o sdrv_trace.sdrv sdrv_trace.manifest.o sdrv_trace_uart.o >> armlink_via.txt
echo ..\..\..\..\..\lib\arm_rvct\libsddi.so >> armlink_via.txt
armlink  --via armlink_via.txt
@if ERRORLEVEL 1 goto build_error

goto :EOF

:build_error
echo BUILD ERROR!
