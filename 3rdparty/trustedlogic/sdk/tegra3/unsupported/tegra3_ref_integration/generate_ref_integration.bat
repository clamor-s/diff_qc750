@echo off
setlocal
title Generate TF ref integration

set PRODUCT_ROOT=%~dp0\..\..

REM Mandatory drivers (DO NOT REMOVE)
set TF_DRIVERS_DIR=%PRODUCT_ROOT%\tegra3_secure_world_integration_kit\sddk\drivers
set TF_DRV_CRYPTO=%TF_DRIVERS_DIR%\sdrv_crypto\build\arm_rvct\release\sdrv_crypto.sdrv
set TF_DRV_L2CC=%TF_DRIVERS_DIR%\sdrv_system\build\arm_rvct\release\sdrv_system.sdrv

REM Services provided as examples
REM set TF_EXAMPLES=%PRODUCT_ROOT%\tf_sdk\examples
REM TF_SRV_AES=%TF_EXAMPLES%\example_aes\build\arm_rvct\release\aes_service.srvx
REM TF_SRV_CERTSTORE=%TF_EXAMPLES%\example_certstore\build\arm_rvct\release\certstore_service.srvx
REM TF_SRV_CRYPTO_CAT=%TF_EXAMPLES%\example_cryptocatalogue\build\arm_rvct\release\cryptocatalogue_service.srvx
REM TF_SRV_DRM=%TF_EXAMPLES%\example_drm\build\arm_rvct\release\drm_service.srvx
REM TF_SRV_SDATE_A=%TF_EXAMPLES%\example_sdate\build\arm_rvct\release\sdate_service_a.srvx
REM TF_SRV_SDATE_B=%TF_EXAMPLES%\example_sdate\build\arm_rvct\release\sdate_service_b.srvx


REM Step 1: Postlink mandatory secure drivers with Trusted Foundations core binary.
echo.
echo -------------------------------------------------------------
echo    Postlink the Secure World binary
echo -------------------------------------------------------------
echo.

REM before regenerating it, we rename the old
move %PRODUCT_ROOT%\unsupported\tegra3_ref_integration\tf_ref_integration.bin %PRODUCT_ROOT%\unsupported\tegra3_ref_integration\tf_ref_integration.bin.OLD
move %PRODUCT_ROOT%\unsupported\tegra3_ref_integration\tf_include.h %PRODUCT_ROOT%\unsupported\tegra3_ref_integration\tf_include.h.OLD

%PRODUCT_ROOT%\tegra3_secure_world_integration_kit\tools\ix86_win32\tf_postlinker %* ^
   -c system_cfg.ini --output %PRODUCT_ROOT%\unsupported\tegra3_ref_integration\tf_ref_integration.bin ^
   %PRODUCT_ROOT%\tegra3_secure_world_integration_kit\bin\tf_core.bin ^
   %TF_DRV_CRYPTO% ^
   %TF_DRV_L2CC%
   
   
REM Step 2: Generate include file who will contain Trusted Foundations binary code.
echo.
echo -------------------------------------------------------------
echo    Generate Trusted Foundations include file
echo -------------------------------------------------------------
echo.

call %PRODUCT_ROOT%\tegra3_secure_world_integration_kit\tools\ix86_win32\tf_gen_include.bat ^
   %PRODUCT_ROOT%\unsupported\tegra3_ref_integration\tf_ref_integration.bin
   
move %PRODUCT_ROOT%\tegra3_secure_world_integration_kit\tools\ix86_win32\tf_include.h .\tf_include.h
