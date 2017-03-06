#!/bin/bash

echo "Generate TF ref integration"

PRODUCT_ROOT=$(pwd)/../..

# Mandatory drivers (DO NOT REMOVE)
TF_DRIVERS_DIR=$PRODUCT_ROOT/tegra3_secure_world_integration_kit/sddk/drivers
TF_DRV_CRYPTO=$TF_DRIVERS_DIR/sdrv_crypto/build/arm_rvct/release/sdrv_crypto.sdrv
TF_DRV_L2CC=$TF_DRIVERS_DIR/sdrv_system/build/arm_rvct/release/sdrv_system.sdrv

#Step 1: Postlink mandatory secure drivers with Trusted Foundations core binary.
echo 
echo -------------------------------------------------------------
echo    Postlink the Secure World binary
echo -------------------------------------------------------------
echo 

# before regenerating it, we rename the old
mv -v $PRODUCT_ROOT/unsupported/tegra3_ref_integration/tf_ref_integration.bin $PRODUCT_ROOT/unsupported/tegra3_ref_integration/tf_ref_integration.bin.OLD
mv -v $PRODUCT_ROOT/unsupported/tegra3_ref_integration/tf_include.h $PRODUCT_ROOT/unsupported/tegra3_ref_integration/tf_include.h.OLD
   
chmod +x $PRODUCT_ROOT/tegra3_secure_world_integration_kit/tools/ix86_linux/tf_postlinker

$PRODUCT_ROOT/tegra3_secure_world_integration_kit/tools/ix86_linux/tf_postlinker \
   -c system_cfg.ini --output $PRODUCT_ROOT/unsupported/tegra3_ref_integration/tf_ref_integration.bin \
   $PRODUCT_ROOT/tegra3_secure_world_integration_kit/bin/tf_core.bin \
   $TF_DRV_CRYPTO \
   $TF_DRV_L2CC   
 
# Step 2: Generate include file who will contain Trusted Foundations binary code.
echo
echo -------------------------------------------------------------
echo    Generate Trusted Foundations include file
echo -------------------------------------------------------------
echo

chmod +x $PRODUCT_ROOT/tegra3_secure_world_integration_kit/tools/ix86_linux/tf_gen_include.sh

$PRODUCT_ROOT/tegra3_secure_world_integration_kit/tools/ix86_linux/tf_gen_include.sh \
   $PRODUCT_ROOT/unsupported/tegra3_ref_integration/tf_ref_integration.bin
  
mv -v $PRODUCT_ROOT/tegra3_secure_world_integration_kit/tools/ix86_linux/tf_include.h ./tf_include.h
