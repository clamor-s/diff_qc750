SET TL_SDK_PATH=C:\android\TF_TEGRA3_AB01.03.32764\tf_sdk
SET RVCT_PATH=c:\android\rvct

call %RVCT_PATH%\rvct.bat

echo Linking the service
echo. > armlink_via.txt
echo --bpabi --fpu=vfp --pltgot=none --dll -o nvsi.sdrv nvsi.service.manifest.o nvsi.service.o nvsi.intrp.arm.arm.o >> armlink_via.txt
echo %TL_SDK_PATH%\lib\arm_rvct\libssdi.so >> armlink_via.txt
echo %TL_SDK_PATH%\..\tegra3_secure_world_integration_kit\sddk\lib\arm_rvct\libsddi.so >> armlink_via.txt
echo .\keybox\liboem_keybox.a >> armlink_via.txt
armlink  --via armlink_via.txt