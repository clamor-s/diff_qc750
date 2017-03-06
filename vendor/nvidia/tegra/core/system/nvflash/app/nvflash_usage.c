/*
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvapputil.h"
#include "nvflash_usage.h"

void
nvflash_usage(void)
{
    char *cmds_list[]=
    {
    "\n==========================================================================================\n"
    "Basic commands/options:\n"
    "1)--help               : displays usage of all available nvflash commands\n",
    "2)--cmdhelp            : displays usage of specified nvflash command\n",
    "3)--bct                : used to specify bct/cfg file containing device specific parametrs\n",
    "4)--setbct             : used to download bct to IRAM\n",
    "5)--odmdata            : specifies the 32 bit odmdata integer\n",
    "6)--configfile         : used to specify configuration file\n",
    "7)--create             : initiates full intialization of targed device\n",
    "8)--bl                 : used to specify bootloader which will run 3pserver on device side\n",
    "9)--blob               : used for nvflash in odm secure devices\n",
    "10)--go                : used to boot bootloader after nvflash completes\n",
    "\nCommands/Options which require already flashed device:\n"
    "11)--setboot           : used to set a partition as bootable\n",
    "12)--format_partition  : used to format a partition\n",
    "13)--download          : used to download filename into a partition\n",
    "14)--read              : used to read a partition into filename\n",
    "15)--rawdevicewrite    : used to write filename into a sector region of media\n",
    "16)--rawdeviceread     : used to read a sector region of media into a filename\n",
    "17)--getpartitiontable : used to read partition table in text\n",
    "18)--setblhash         : used to download bootloader in secure mode\n",
    "19)--getbct            : used to read back the BCT\n",
    "20)--skip_part         : used to indicate a partition to skip\n",
    "21)--format_all        : used to format/delete all existing partitions\n",
    "22)--obliterate        : used to erase all partitions and bad blocks\n",
    "23)--updatebct         : used to update some section of system bct(bctsection)",
    "\nOther commands/options:\n"
    "24)--resume            : used when device is looping in 3pserver.\n",
    "25)--verifypart        : used to verify contents of a partition in device\n",
    "26)--getbit            : used to read back the bit table to filename in binary form\n",
    "27)--dumpbit           : used to display the bit table read from device in text form\n",
    "28)--odm               : used to do some special diagnostics\n",
    "29)--deviceid          : used to set the device id of target in fpga emulation/simulation\n",
    "30)--devparams         : used to pass metadata(pagesize etc.) in simulation mode\n",
    "31)--quiet             : used to suppress the exccessive console o/p while host-device comm\n",
    "32)--wait              : used to wait for USB cable connection before execution starts\n",
    "33)--instance          : used when multiple devices connected\n",
    "34)--transport         : used to specify the means of communication between host and target\n",
    "35)--setbootdevtype    : used to set boot device type fuse\n",
    "36)--setbootdevconfig  : used to set boot device config fuse\n",
    "37)--diskimgopt        : used to convert .dio or a .store.bin file to the new format (dio2)\n",
    "38)--internaldata      : used to flash the sku-info,serial-id,mac-id,prod-id\n",
    "39)--setentry          : used to specify the entry point and load address of the bootloader\n",
    "40)--settime           : used to update the PMU RTC HW clock\n"
    "============================================================================================\n"
    "\nUse --cmdhelp (--command name)/(command index) for detailed description & usage\n\n",
     };

    NvU32 i;

    for (i = 0; i < 40; i++)
    {
        NvAuPrintf(cmds_list[i]);
    }
}

void
nvflash_cmd_usage(NvFlashCmdHelp *a)
{
    if (NvOsStrcmp(a->cmdname, "--help" ) == 0 ||
        NvOsStrcmp(a->cmdname, "-h") == 0 ||
        NvOsStrcmp(a->cmdname, "1") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf("displays all supported nvflash commands with usage and description\n\n");
    }
    else if (NvOsStrcmp(a->cmdname, "--cmdhelp") == 0 ||
        NvOsStrcmp(a->cmdname, "-ch") == 0 ||
        NvOsStrcmp(a->cmdname, "2") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --cmdhelp <cmd>\n"
            "-----------------------------------------------------------------------\n"
            "used to display usage and description about a particular command cmd\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--resume") == 0 ||
        NvOsStrcmp(a->cmdname, "-r") == 0 ||
        NvOsStrcmp(a->cmdname, "24") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --resume --read <part id> <filename> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "must be first option in command line, used when device is looping\n"
            "in 3pserver which is achieved when a command of nvflash is executed\n"
            "either without sync or go or both, can be used with all nvflash commands\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--create") == 0 ||
             NvOsStrcmp(a->cmdname, "7") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --bct <bct> --setbct --configfile <cfg> --create --odmdata <N> "
            "--bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to do full initialization of the target device using the config file\n"
            "cfg start from creating all partitions, formatting them all, download\n"
            "images to their respective partitions and syncing bct at end\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--setboot") == 0 ||
             NvOsStrcmp(a->cmdname, "11") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --setboot <N/pname> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to set partition N/pname as bootable to already flashed device so that\n"
            "next time on coldboot, device will boot from bootloader at partition N/pname\n"
            "must be used only for bootloader partitions\n"
            "Partition number N can be found from cfg file used for flash earlier or\n"
            "Partition name(pname) can also be used as an argument\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--format_partition") == 0 ||
             NvOsStrcmp(a->cmdname, "--delete_partition") == 0 ||
             NvOsStrcmp(a->cmdname, "12") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --format_partition <N/pname> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to format patition N/pname in already flashed device making it empty\n"
            "Partition number N can be found from cfg file used for flash earlier or\n"
            "Partition name(pname) can also be used as an argument\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--verifypart") == 0 ||
             NvOsStrcmp(a->cmdname, "25") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --bct <bct> --setbct --configfile <cfg> --create --odmdata <N> "
            "--verifypart <N/pname> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to verify contents of partition N/pname in device, hash is calculated\n"
            "and stored while downloading the data in partition and after all images\n"
            "are downloaded, content is read back from partition N/pname calculating hash\n"
            "and matching it with stored hash, use N=-1 for verifying all partitions\n"
            "must be used with --create command\n"
            "Partition number N can be found from cfg file used for flash earlier or\n"
            "Partition name(pname) can also be used as an argument\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--download") == 0 ||
             NvOsStrcmp(a->cmdname, "13") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "1. nvflash --download <N/pname> <filename> --bl <bootloader> --go (in nvprod\n"
            "mode)\n"
            "2. nvflash --blob <blob> --setblhash <bct> --configfile <cfg> --download \n"
            "<N/pname> <filename> --bl <bootloader> --go (in odm secure mode)\n"
            "-----------------------------------------------------------------------\n"
            "used to download filename in partition N/pname into already flashed device\n"
            "command 2 is used only for downloading the bootloader in secure mode,\n"
            "it gets encrypted bct from sbktool containing hash of updated bootloader\n"
            "to be downloaded. This bct is flashed to IRAM and update the system bct\n"
            "with new hash of updated bootloader. In all other cases command 1 is used\n"
            "Partition number N can be found from cfg file used for flash earlier or\n"
            "Partition name(pname) can also be used as an argument\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--read") == 0 ||
             NvOsStrcmp(a->cmdname, "14") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --read <N/pname> <filename> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to read partition N/pname from already flashed device into filename\n"
            "Partition number N can be found from cfg file used for flash earlier or\n"
            "Partition name(pname) can also be used as an argument\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--rawdevicewrite") == 0 ||
             NvOsStrcmp(a->cmdname, "15") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --rawdevicewrite S N <filename> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to write filename into N sectors of media starting from sector S to\n"
            "already flashed device\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--rawdeviceread") == 0 ||
             NvOsStrcmp(a->cmdname, "16") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --rawdeviceread S N <filename> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to read N sectors of media starting from sector S from already\n"
            "flashed device into filename\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--getpartitiontable") == 0 ||
             NvOsStrcmp(a->cmdname, "17") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --getpartitiontable <filename> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to read partition table in text from already flashed device into filename\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--updatebct") == 0 ||
             NvOsStrcmp(a->cmdname, "23") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --bct <bct> --updatebct <bctsection> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to update some section of system bct(bctsection) from bct specified\n"
            "this command is run in 3pserver. As of now, bctsection can be SDRAM which\n"
            "updates SdramParams and NumSdramSets field of bct, DEVPARAM updates\n"
            "DevParams, DevType and NumParamSets fields and BOOTDEVINFO updates\n"
            "BlockSizeLog2, PageSizeLog2 and PartitionSize fields of system bct\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--setblhash") == 0 ||
             NvOsStrcmp(a->cmdname, "18") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --blob <blob> --setblhash <bct> --configfile <cfg> --download N\n"
            "<filename> --bl <bootloader> --go (in odm secure mode)\n"
            "-----------------------------------------------------------------------\n"
            "used to download bootloader in secure mode into already flashed device\n"
            "it updates the hash value of updated bootloader to be downloaded, from\n"
            "encrypted bct got from sbktool, into system bct in miniloader\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--setbct") == 0 ||
             NvOsStrcmp(a->cmdname, "4") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "as in full fledged nvflash command\n"
            "-----------------------------------------------------------------------\n"
            "used to download bct to IRAM, must be used with --sync command to update\n"
            "it in mass storage.\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--getbct") == 0 ||
             NvOsStrcmp(a->cmdname, "19") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --bct <filename> --getbct --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to read back the BCT from already flashed device to user specified\n"
            "filename, read in clear form whether device is secure or non-secure\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--getbit") == 0 ||
             NvOsStrcmp(a->cmdname, "26") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --getbit <filename> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to read back the bit table to filename in binary form in miniloader\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if(NvOsStrcmp(a->cmdname, "--dumpbit") == 0 ||
            NvOsStrcmp(a->cmdname, "27") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --dumpbit --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to display the bit table read from device in text form on user\n"
            "terminal. Also gives info about bct and various bootloaders present in\n"
            "media, normally used for debugging cold boot failures\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--odm") == 0 ||
             NvOsStrcmp(a->cmdname, "28") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --odm CMD data --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to do some special diagnostics like sdram validation, fuelgauge etc\n"
            "each CMD has data associated with it\n"
            "For e.g.\n"
            "1. SD/SE/PWM diagnostic tests:\n"
            "   nvflash --odm runsediag <N> --bl <bootloader> --go\n"
            "   nvflash --odm runsddiag <N1> <N2> --bl <bootloader> --go\n"
            "   nvflash --odm runpwmdiag --bl <bootloader> --go\n"
            "   ---------------------------------------------------------------------\n"
            "   used to run SD/SE/PWM diagnostic tests\n"
            "   where N is the input parameter for runsediag\n"
            "   where N1 and N2 are the input parameters for runsddiag\n"
            "   N1 gives the Sd controller instance and N2 gives the testcase to run\n"
            "   For runsddiag(sd)   : N1 = 0 for cardhu, N1 = 2 for enterprise\n"
            "   For runsddiag(eMMC) : N1 = 3 for cardhu as well as enterprise\n"
            "   For runsddiag       : N2 = 0 for ReadWriteTest\n"
            "   For runsddiag       : N2 = 1 for ReadWriteFullTest\n"
            "   For runsddiag       : N2 = 2 for ReadWriteSpeedTest\n"
            "   For runsddiag(eMMC) : N2 = 3 for ReadWriteBootAreaTest\n"
            "   For runsddiag       : N2 = 4 for EraseTest\n"
            "   For runsediag       : N = 0 (field unused can be used later)\n"
            "2. Sdram verification:\n"
            "   nvflash --bct <bct> --setbct --configile <cfg> --create --odmdata <N> "
            "   --odm verifysdram <N> --bl <bootloader> --go\n"
            "   ---------------------------------------------------------------------\n"
            "   used to verify sdram initialization i.e. verify sdram params in bct is\n"
            "   in accordance with actual SDRAM attached in device or not.\n"
            "   N can be either 0 or 1. '0' implies soft test where known word patterns\n"
            "   are written for each mb and read back. This word pattern validates all\n"
            "   the bits of the word. '1' implies stress test where known word patterns\n"
            "   are written on entire sdram area available and read back. This tests every\n"
            "   bit of the the entire available sdram area\n"
            "-----------------------------------------------------------------------\n\n"
            "3. FuelGaugeFwUpgrade\n"
            "   nvflash --odmdata <N>  --odm fuelgaugefwupgrade <filename1> <filename2> \n"
            "   --bl <bootloader> --go\n"
            "   ---------------------------------------------------------------------\n"
            "   To flash the fuelgauge firmware. file1 is the bqfs file having the firmware\n"
            "   details with corresponding data flash formatting data.file2 is the optional\n"
            "   dffs file and will carry the data flash related information alone.The \n"
            "   firmware upgrade tol reads the files and send the required info to the fuel\n"
            "   gauge device.\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--go") == 0 ||
             NvOsStrcmp(a->cmdname, "9") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "as in full fledged nvflash command\n"
            "-----------------------------------------------------------------------\n"
            "used to boot bootloader after nvflash completes instead of looping in\n"
            "3pserver in resume mode, however sync command is also required to get\n"
            "out of resume mode, can be used with all nvflash commands\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--obliterate") == 0 ||
             NvOsStrcmp(a->cmdname, "22") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --configfile <cfg> --obliterate --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to erase all partitions and bad blocks in already flashed device\n"
            "using partition info from configuration file cfg used in nvflash earlier\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--configfile") == 0 ||
             NvOsStrcmp(a->cmdname, "6") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "as in full fledged nvflash command\n"
            "-----------------------------------------------------------------------\n"
            "used to specify configuration file containing info about various partitions\n"
            "to be made into device, their sizes, name and other attributes\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--bct") == 0 ||
             NvOsStrcmp(a->cmdname, "3") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "as in full fledged nvflash command\n"
            "-----------------------------------------------------------------------\n"
            "used to specify bct file containing device specific parametrs like\n"
            "SDRAM configs, boot device configs etc. This is again modified by device\n"
            "to include info about bootloader, partition table etc. Config file can also\n"
            "be used as input in which case it wil be converted into a BCT file.\n"
            "Now supports cfg file as input which is converted to BCT file internally\n"
            "by the buildbct library.\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--blob" ) == 0 ||
             NvOsStrcmp(a->cmdname, "9") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --blob <blob> --bct <bct> --setbct --configfile <cfg> --create "
            "--odmdata <N> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used for nvflash in odm secure devices, can be used with any nvflash commands\n"
            "blob contains encrypted and signed RCM messages for communication b/w nvflash\n"
            "and bootrom at start, encrypted hash of encrypted bootloader(used with --bl\n"
            "option) for it's validation by miniloader and nvsbktool version info\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--bl") == 0 ||
             NvOsStrcmp(a->cmdname, "8") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "as in full fledged nvflash command\n"
            "-----------------------------------------------------------------------\n"
            "used to specify bootloader which will run 3pserver on device side after\n"
            "it is downloaded by miniloader in SDRAM, normally used with all commands\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--odmdata") == 0 ||
             NvOsStrcmp(a->cmdname, "5") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "as in full fledged nvflash command\n"
            "-----------------------------------------------------------------------\n"
            "specifies a 32 bit integer used to select particular instance of UART\n"
            "determine SDRAM size to be used etc. Also used for some platform specific\n"
            "operations, it is stored in bct by miniloader\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--deviceid") == 0 ||
             NvOsStrcmp(a->cmdname, "29") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --bct <bct> --setbct --configfile <cfg> --create --odmdata <N> "
            "--deviceid <devid> --transport <transport_mode> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to set the device id of target in fpga emulation or simulation mode\n"
            "depending on chip, can be hex or dec, only used with --transport option\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--transport") == 0 ||
             NvOsStrcmp(a->cmdname, "34") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --bct <bct> --setbct --configfile <cfg> --create --odmdata <N> "
            "--deviceid <devid> --transport <transport_mode> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to specify the means of communication between host and target\n"
            "USB, JTAG and Simulation are three modes supported as of now. JTAG is\n"
            "for fpga emulation, Simulation is for nvflash in simulation mode and\n"
            "default is USB, used in normal cases while interaction with device\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--devparams") == 0 ||
             NvOsStrcmp(a->cmdname, "30") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --bct <bct> --setbct --configfile <cfg> --create --odmdata <N> "
            "--deviceid <devid> --transport <transport_mode> --devparams <pagesize> "
            "<blocksize> <totalblocks>  --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to pass logical pagesize, erase block unit size and total number of\n"
            "blocks for nvflash in simulation mode. As of this write, pagesize = 4K\n"
            "blocksize = 2M and totalblocks = media_size/blocksize. These values can be\n"
            "obtained using NvDdkBlockDevGetDeviceInfo api, a wrapper of SdGetDeviceInfo\n"
            "api in block driver, used only for nvflash in simulation mode\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--skip_part") == 0 ||
             NvOsStrcmp(a->cmdname, "20") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --bct <bct> --setbct --configfile <cfg> --create --odmdata <N> "
            "--bl <bootloader> --skip_part <part> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to indicate a partition to skip using following commands:\n"
            " --create: partition skipped with --create is not formatted.\n"
            "<part> is partition name as it appears in config file.\n"
            "This command can only be used associated with --create command.It will\n"
            "fail if config file used differ with the one used for previous device\n"
            "formatting. For this reason, command will also fail if device is not\n"
            "already flashed and formatted.\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--setbootdevtype") == 0 ||
             NvOsStrcmp(a->cmdname, "35") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --setbootdevtype S --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to set boot device type fuse to S which tells boot media to boot from\n"
            "used only in miniloader since cold boot uses bootrom and all settings get\n"
            "reset at that time. Unlike miniloader, BL/kernel uses pinmuxes not fuses\n"
            "S can be emmc, nand_x8, spi etc\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--setbootdevconfig") == 0 ||
             NvOsStrcmp(a->cmdname, "36") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --setbootdevconfig N --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to set boot device config fuse to N which tells slot of boot device\n"
            "to boot from, used only in miniloader since cold boot uses bootrom and\n"
            "all settings get reset at that time. BL/kernel uses pinmuxes not fuses\n"
            "N is 33 for sdmmc4 pop slot and 17 for sdmmc4 planar slot of whistler\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--diskimgopt") == 0 ||
             NvOsStrcmp(a->cmdname, "37") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --bct <bct> --setbct --configfile <cfg> --create --diskimgopt <N> "
            "--odmdata <N> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to convert .dio or a .store.bin file to the new format (dio2) by\n"
            "inserting the required compaction blocks of size N, can be used with\n"
            "any nvflash commands which downloads an image to device\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--quiet") == 0 ||
             NvOsStrcmp(a->cmdname, "-q") == 0 ||
             NvOsStrcmp(a->cmdname, "31") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --quiet --bct <bct> --setbct --configfile <cfg> --create "
            "--odmdata <N> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to suppress the exccessive console output like status info while send\n"
            "or recieve data b/w host and device, can be used with all nvflash commands\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--wait") == 0 ||
             NvOsStrcmp(a->cmdname, "-w" ) == 0 ||
             NvOsStrcmp(a->cmdname, "32") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --wait --bct <bct> --setbct --configfile <cfg> --create --odmdata <N> "
            " --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "used to wait for USB cable connection before start executing any command\n"
            "can be used with any commands of nvflash\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--delete_all" ) == 0 ||
             NvOsStrcmp(a->cmdname, "--format_all" ) == 0 ||
             NvOsStrcmp(a->cmdname, "21") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --delete_all/--format_all --bl <bootloader> --go \n"
            "-----------------------------------------------------------------------\n"
            "used to format/delete all existing partitions on already flashed device\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--instance" ) == 0 ||
             NvOsStrcmp(a->cmdname, "33") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "1. nvflash --bct <bct> --setbct --configfile <cfg> --create --odmdata <N> "
            "   --instance <N> --bl <bootloader> --go\n"
            "2. nvflash --bct <bct> --setbct --configfile <cfg> --create --odmdata <N> "
            "   --instance <path> --bl <bootloader> --go\n"
            "-----------------------------------------------------------------------\n"
            "--instance is used to flash number of devices connected to system through\n"
            "  usb cable from different terminals at one go\n"
            "1. used to flash Nth usb device attached. The instance number N is given\n"
            "   as input and the Nth device in the enumeration list is flashed\n"
            "2. used to flash a specific device found at:/dev/bus/usb/xxx/yyy .This\n"
            "   path string is given as an input argument\n"
            "-----------------------------------------------------------------------\n\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--internaldata" ) == 0 ||
             NvOsStrcmp(a->cmdname, "38") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --bct <bct> --setbct --configfile <cfg> --create --odmdata <N> "
            "   --instance <N> --bl <bootloader> --internaldata <blob> --go\n"
            "------------------------------------------------------------------------\n"
            "--internaldata is used to flash the sku-info,serial-id,mac-id,prod-id.\n"
            "Input is the blob containing the data in offset,size,data format.\n"
            "This blob file is created using the internal tool nvskuflash.\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--setentry" ) == 0 ||
             NvOsStrcmp(a->cmdname, "39") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --setentry N1 N2 --bl <bootloader> --go\n"
            "------------------------------------------------------------------------\n"
            "--setentry is used to specify the entry point and load address of the \n"
            "bootloader via Nvflash commandline. The first argument N1 is the bootloader\n"
            "entrypoint and second argument N2 is the bootloader loadaddress.\n"
            );
    }
    else if (NvOsStrcmp(a->cmdname, "--settime" ) == 0 ||
             NvOsStrcmp(a->cmdname, "40") == 0)
    {
        NvAuPrintf("\n%s command usage ->\r\n",a->cmdname);
        NvAuPrintf(
            "nvflash --settime --bl <bootloader> --go\n"
            "------------------------------------------------------------------------\n"
            "--settime is used to update the PMU RTC HW clock. The clock is updated with \n"
            "epoch time as base.The total time in seconds is received from nvflash host\n"
            "with base time as 1970.\n"
            );
    }
    else
    {
        NvAuPrintf("Unknown command %s \r\n",a->cmdname);
    }
}

