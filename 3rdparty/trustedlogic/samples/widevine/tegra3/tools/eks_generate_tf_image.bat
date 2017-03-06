@echo off
setlocal
title Generate TS ref integration

set PRODUCT_ROOT=C:\tl\tegra3
rem set path=C:\tl\rvct\win_32-pentium;%path%
rem set ARMLMD_LICENSE_FILE=C:\Program Files\ARM\rvds.dat
set RVCT40INC=C:\Program Files\ARM\RVCT\Data\4.0\400\include\windows
set RVCT40LIB=C:\Program Files\ARM\RVCT\Data\4.0\400\lib
set RVCT40BIN=C:\tl\rvct\win_32-pentium

REM Check environment variable used...
if not "%PRODUCT_ROOT%" == "" goto SET_PATH
echo wrong path PRODUCT_ROOT:[%PRODUCT_ROOT%]
goto END

:SET_PATH
set TS_PRODUCT_ROOT=%~dp0..
echo TS_PRODUCT_ROOT %TS_PRODUCT_ROOT%

REM Mandatory drivers (DO NOT REMOVE)
set TF_DRIVERS_DIR=%PRODUCT_ROOT%\tegra3_secure_world_integration_kit\sddk\drivers
set TF_DRV_CRYPTO=%TF_DRIVERS_DIR%\sdrv_crypto\build\arm_rvct\release\sdrv_crypto.sdrv
set TF_DRV_L2CC=%TF_DRIVERS_DIR%\sdrv_system\build\arm_rvct\release\sdrv_system.sdrv
set TF_DRV_OTF=%TF_DRIVERS_DIR%\sdrv_otf\build\arm_rvct\release\sdrv_otf.sdrv
set TF_DRV_TRACE=%TF_DRIVERS_DIR%\sdrv_trace\build\arm_rvct\release\sdrv_trace.sdrv
set TF_DRV_RSA=%TF_DRIVERS_DIR%\sdrv_rsa\build\arm_rvct\release\sdrv_rsa.sdrv
set TF_DRV_NVSI=%TS_PRODUCT_ROOT%\..\..\..\..\..\vendor\nvidia\tegra\secureos\nvsi\nvsi.sdrv
REM OEMCRYPTO service
set OEMCRYPTO_SERVICE=%TS_PRODUCT_ROOT%\..\..\..\samples\widevine\tegra3\build\release\oemcrypto_secure_service.srvx
set HDCP_SECURE_SERVICE=%TS_PRODUCT_ROOT%\..\..\..\samples\hdcp\tegra3\build\arm_rvct\release\secure_hdcp_service.srvx

:CHECKFORSWITCHES
REM Read script param
IF "%1"=="--help" goto HELP
IF "%1"=="" goto BEGIN

:BEGIN

goto POSTLINKER
@if ERRORLEVEL 1 goto END

:POSTLINKER
REM before regenerating it, we rename the old
move %TS_PRODUCT_ROOT%\tf_ref_integration.bin %TS_PRODUCT_ROOT%\tf_ref_integration.bin.OLD
move %TS_PRODUCT_ROOT%\tf_include.h %TS_PRODUCT_ROOT%\tf_include.h.OLD

%PRODUCT_ROOT%\tegra3_secure_world_integration_kit\tools\ix86_win32\tf_postlinker ^
   -c %PRODUCT_ROOT%\unsupported\tegra3_ref_integration\system_cfg.ini --output %TS_PRODUCT_ROOT%\tf_ref_integration.bin ^
   %PRODUCT_ROOT%\tegra3_secure_world_integration_kit\bin\tf_core.bin ^
   %TF_DRV_CRYPTO% ^
   %TF_DRV_L2CC% ^
   %OEMCRYPTO_SERVICE% ^
   %HDCP_SECURE_SERVICE% ^
   %TF_DRV_OTF% ^
   %TF_DRV_NVSI% ^
   %TF_DRV_RSA%

REM find addresses and put them into Trusted Foundations binary code.
echo.
echo -------------------------------------------------------------
echo    Find Addresses and modify TF binary
echo -------------------------------------------------------------
echo.

call %PRODUCT_ROOT%\unsupported\python\ix86_win32\python.bat eks_address_finder.py ^
   --tf=%TS_PRODUCT_ROOT%\tf_ref_integration.bin

REM Generate include file who will contain Trusted Foundations binary code.
echo.
echo -------------------------------------------------------------
echo    Generate Trusted Foundations include file
echo -------------------------------------------------------------
echo.

call %PRODUCT_ROOT%\tegra3_secure_world_integration_kit\tools\ix86_win32\tf_gen_include.bat ^
   %TS_PRODUCT_ROOT%\tf_ref_integration.bin

move %PRODUCT_ROOT%\tegra3_secure_world_integration_kit\tools\ix86_win32\tf_include.h .\tf_include.h

goto END


:HELP
echo usage generate_tf_image.bat


:END
pause 