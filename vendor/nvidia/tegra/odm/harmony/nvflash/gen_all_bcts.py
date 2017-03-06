#!/usr/bin/env python

#
# Generates .cfg and .bct files for all combinations of the following
# system, sdram, and boot device types.
#
# To add additional flavors, add new lines to the appropriate lists.
# Ensure that each line ends in a comma.
#
# The intended usage of this code:
#     p4 edit *.cfg *.bct
#     gen_all.py
#     p4 add *.cfg *.bct
#     p4 revert -a
#     <review the files that remain open>
#     p4 submit
#
# Once the code is debugged, all the p4 commands save the last will be
# run from within this script
#

boot_device_types = (
    "2K8Nand_HY27UF084G2B-TP",
    "emmc_THGBM1G6D4EBAI4",
    "emmc_THGBM1G6D4EBAI4_x8",
)

sdram_types = (
    "harmony_12Mhz_H5PS1G83EFR-Y5C_150Mhz_1GB",
    "harmony_12Mhz_H5PS1G83EFR-Y5C_300Mhz_1GB",
    "harmony_a02_12Mhz_H5PS1G83EFR-S5C_333Mhz_1GB",
    "harmony_12Mhz_H5PS1G83EFR-S5C_150Mhz_512MB",
    "harmony_12Mhz_H5PS1G83EFR-S5C_300Mhz_512MB",
    )

import os, sys

# Open all the .cfg and .bct files for editing
retcode = os.system("p4 edit *.cfg *.bct")
if retcode < 0:
    print >>sys.stderr, "p4 edit  returned error ", retcode
    sys.exit(retcode)

wiki_file = open("wiki.txt", "w")

# Generate all the combined .cfg and .bct files
for sdram in sdram_types:
    for boot_device in boot_device_types:
        cfg_name = sdram + "_" + boot_device + ".cfg"
        bct_name = sdram + "_" + boot_device + ".bct"

        print "generating: " + bct_name

        cfg_file = open(cfg_name, "w")

        # Copy the boot device data into the output .cfg.
        boot_device_file = open(boot_device + ".cfg")
        boot_device_data = boot_device_file.read()
        cfg_file.write(boot_device_data)
        boot_device_file.close()

        # Copy the sdram data into the output .cfg.
        sdram_file = open(sdram + ".cfg")
        sdram_data = sdram_file.read()
        cfg_file.write(sdram_data)
        sdram_file.close()

        cfg_file.close()

        # Generate the bct
        retcode = os.system("buildbct " + cfg_name + " " +
                            bct_name + " --ap20")
        if retcode < 0:
            print >>sys.stderr, "buildbct returned error ", retcode
            sys.exit(retcode)

        # Add appropriate data to the wiki output.
        if sdram[1] == '8':
            note_text = "LPDDR2 POP memories."
        elif sdram[1] == '5':
            note_text = "none"
        else:
            note_text = "<unhandled case>"

        wiki_file.write("|-\n")
        wiki_file.write("| <P4>p4sw:2006//sw/mobile/main/customers/nvidia/whistler/nvflash/" + bct_name + " | " + bct_name + " </P4> || - || " + note_text + "\n")

wiki_file.close()

# Add any new ones
retcode = os.system("p4 add *.cfg *.bct")
if retcode < 0:
    print >>sys.stderr, "p4 add returned error ", retcode
    sys.exit(retcode)

# Revert unchanged files
retcode = os.system("p4 revert -a")
if retcode < 0:
    print >>sys.stderr, "p4 revert returned error ", retcode
    sys.exit(retcode)

