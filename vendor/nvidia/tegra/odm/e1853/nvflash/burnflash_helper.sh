# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

TargetType="e1853"
SysMem2GB=0
BoardKernArgs="usbcore.old_scheme_first=1"
BoardOptions="k:p:"
PartitionLayout=
UserConfigFile=
ConfigBootMedium=0
QNXFileName="ifs-nvidia-tegra3-mmx2.bin"
ValidSkus=( 8 )
SkuValid=0

BoardParseOptions()
{

    case $Option in
        k) ConfigFile=$OPTARG
           if [ ! -e $ConfigFile ]; then
               echo "Config file $ConfigFile does not exist"
               exit -1
           fi
           UserConfigFile=1
           ;;
        p) PartitionLayout=$OPTARG
           ;;
        *) echo "Invalid option $Option"
           Usage
           AbnormalTermination
           ;;
    esac
}

#functions needed by burnflash.sh

BoardHelpHeader()
{
    echo "Usage : `basename $0` [ -h ]  <-n <NFSPORT> | -r <ROOTDEVICE> > [ options ]"
}

BoardHelpOptions()
{
    echo "-S : Provide SKU id from amongst the following: `echo ${ValidSkus[@]} | awk -v var=', ' '{ gsub(/ /,var,$0); print }'` "
    echo "-k : Provide the nvflash config file to be used."
    echo "     Default value is quickboot_snor_linux.cfg for nor boot."
    echo "-p : Provide the partition Layout for the device in the mtdpart format."
    echo "     Partition start and size should be block size aligned"
    echo "     example: mtdparts:<DRIVER_NAME>:size@start(PART_NAME)"
    echo "              \"mtdparts=tegra-nor:384K@36992K(env),28160K@37376K(userspace) \""
    echo ""
#FIXME: Change this for DevAir
    echo "Supported <NFSPORT>    : usb0 : Ethernet connector J58 on E1888 (baseboard)"
    echo "                       : eth0 : USB-Ethernet adapter"
    echo "                       Supported USB-Ethernet adapters: D-Link DUB-E100, Linksys USB200M ver.2.1"
}

BoardValidateParams()
{
    # if SKU file is specified, extract sku id from the filename
    if [ -n "${ZFile}" ]; then
        SkuId=`echo $ZFile|cut -d '-' -f 3`
    fi

    if [ $SkuId -eq 0 ]; then
        echo "Missing -S option (mandatory)"
        Usage
        AbnormalTermination
    fi

    for Sku in ${ValidSkus[@]}
    do
        if [ $SkuId -eq $Sku ]; then
            SkuValid=1
        fi
    done

    if [ $SkuValid -eq 0 ]; then
        echo "Invalid SKU id"
        AbnormalTermination
    fi

    BCT=${NvFlashDir}/E1853_Hynix_2GB_H5TQ4G63MFR_625MHz_110519_NOR.bct
    let "ODMData = 0x80081105" # UART-A (Lower J7)

    if [ -z $ConfigFile ]; then
        if [ ! ${FlashQNX} -eq 1 ]; then
            if [ $Quickboot -eq 0 ] ; then
                ConfigFile=${NvFlashDir}/fastboot_snor_linux.cfg
            else
                ConfigFile=${NvFlashDir}/quickboot_snor_linux.cfg
            fi
            if [ -z ${PartitionLayout} ]; then
                PartitionLayout="mtdparts=tegra-nor:384K@17664K(env),47488K@18048K(userspace) "
            fi
        else
            if [ $Quickboot -eq 0 ] ; then
                ConfigFile=${NvFlashDir}/fastboot_snor_qnx.cfg
            else
                ConfigFile=${NvFlashDir}/quickboot_snor_qnx.cfg
            fi
        fi
    fi
    BoardKernArgs="${BoardKernArgs} ${PartitionLayout}"
}

SendWorkaroundFiles()
{
    :
}

#FIXME: Add instructions for DevAir
# called before nvflash is invoked
BoardPreflashSteps()
{
    echo "NOTE: All switches are on E1888 (base board)"
    echo "If board is not ON, power it by moving switch S3  to ON position."
    echo "ON position depends on whether 12V DC supply (J6) OR ATX power supply (J9)"
    echo "is used to power the board. If the board is switched ON, RED LED (DS2) will glow."
    echo ""
    echo "Put the board in force recovery by putting the switch S7 to ON position"
    echo "(close to the S7 marking) and reset the board by pressing and releasing S1"
    echo "Press Enter to continue"
    read
}


# called immediately after nvflash is done
BoardPostNvFlashSteps()
{
    :
}

#FIXME: Add instructions for DevAir
# called after the kernel was flashed with fastboot
BoardPostKernelFlashSteps()
{
    echo "Move the recovery switch on OFF position (away from S7 marking)"
    echo "To boot, reset the board"
}

# called if uboot was flashed
BoardUseUboot()
{
    :
}
