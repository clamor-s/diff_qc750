# NVIDIA Tegra3 "Aruba2" development system
echo "DEBUG: Entering $TOP/vendor/nvidia/build/aruba2/aruba2.sh"

OUTDIR=$(get_build_var PRODUCT_OUT)
echo "DEBUG: PRODUCT_OUT = $OUTDIR"

# setup FASTBOOT VENDOR ID
export FASTBOOT_VID=0x955

if [ ! "$FPGA_HAS_LPDDR2" ]
then
    echo "Setting up NvFlash BCT for Aruba2 with DDR3 SDRAM......"
    cp $TEGRA_TOP/customers/nvidia/aruba2/nvflash/aruba2_13Mhz_H5TQ1G83BFR-H9C_13Mhz_1GB_emmc_H26M42001EFR_x8.bct $TOP/$OUTDIR/flash.bct
    _NVFLASH_ODM_DATA=0x40080000
else
    echo "Setting up NvFlash BCT for Aruba2 with LPDDR2 SDRAM......"
    cp $TEGRA_TOP/customers/nvidia/aruba2/nvflash/aruba2_13Mhz_H8TBR00Q0MLR_13Mhz_256MB_emmc_H26M42001EFR_x8.bct  $TOP/$OUTDIR/flash.bct
    _NVFLASH_ODM_DATA=0x10080000
fi

if [ "$ODMDATA_OVERRIDE" ]; then
    export NVFLASH_ODM_DATA=$ODMDATA_OVERRIDE
else
    export NVFLASH_ODM_DATA=$_NVFLASH_ODM_DATA
fi

echo "DEBUG: Leaving $TOP/vendor/nvidia/build/aruba2/aruba2.sh"

