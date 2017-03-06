# NVIDIA Tegra2 "Harmony" development system

# setup NVFLASH ODM Data
if [ "$ODMDATA_OVERRIDE" ]; then
    export NVFLASH_ODM_DATA=$ODMDATA_OVERRIDE
else
    export NVFLASH_ODM_DATA=0x300d8011
fi

# setup FASTBOOT VENDOR ID
export FASTBOOT_VID=0x955
