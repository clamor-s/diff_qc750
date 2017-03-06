#!/usr/bin/env python

# Set the Python search path to find the BCT processing code
import sys
sys.path.append('../../../tools')
from process_bcts import *

#
# Produces the data needed by the bct generation tools.
#
# The data required by the bct generation tools consists of a list of tuples,
# each consisting of:
#     sdram_file: This is the filename, without the trailing .cfg, for a set of
#           SDRAM parameters.
#
#     device_file: This is the filename, without the trailing .cfg, for a set
#           of device parameters.
#
#     bct_file: This is the filename, without the trailing .cfg or .bct, of
#           the final BCT filename.  The .cfg version is the input to
#           buildbct, and the .bct version is the output.
#
#     notes: Any notes for the BCT.  These will end up in the wiki output.
#
# This file contains at least one function which can be called by other tools:
#     generate_bct_list() - Returns the list of BCT tuples.
#
# Futher, simply running this file by itself will print the list of tuples.
#

sdram_configs = [
    'harmony_a02_12Mhz_H5PS1G83EFR-S5C_333Mhz_512MB',
    'harmony_a02_12Mhz_H5PS1G83EFR-S5C_333Mhz_1GB',
    'harmony_12Mhz_H5PS1G83EFR-S5C_150Mhz_512MB',
    'harmony_12Mhz_H5PS1G83EFR-S5C_300Mhz_512MB',
    'harmony_12Mhz_H5PS1G83EFR-Y5C_150Mhz_1GB',
    'harmony_12Mhz_H5PS1G83EFR-Y5C_300Mhz_1GB',
]

device_configs = [
    'emmc_THGBM1G6D4EBAI4',
    'emmc_THGBM1G6D4EBAI4_x8',
    '2K8Nand_HY27UF084G2B-TP',
]

custom_configs = [
    # New BCTs with ODT and CTT off
    [
        'harmony_a02_12Mhz_H5PS1G83EFR-S6C_333Mhz_1GB',
        'emmc_THGBM1G6D4EBAI4',
        'harmony_a02_12Mhz_H5PS1G83EFR-S6C_333Mhz_1GB_emmc_THGBM1G6D4EBAI4',
        'No ODT/CTT for lower power. Should work with both S5C and S6C SDRAMs.',
    ],
    [
        'harmony_a02_12Mhz_H5PS1G83EFR-S6C_333Mhz_1GB',
        'emmc_THGBM1G6D4EBAI4_x8',
        'harmony_a02_12Mhz_H5PS1G83EFR-S6C_333Mhz_1GB_emmc_THGBM1G6D4EBAI4_x8',
        'No ODT/CTT for lower power. Should work with both S5C and S6C SDRAMs.',
    ],
    [
        'harmony_a02_12Mhz_H5PS1G83EFR-S6C_333Mhz_1GB',
        '2K8Nand_HY27UF084G2B-TP',
        'harmony_a02_12Mhz_H5PS1G83EFR-S6C_333Mhz_1GB_2K8Nand_HY27UF084G2B-TP',
        'No ODT/CTT for lower power. Should work with both S5C and S6C SDRAMs.',
    ],
]

#
# Function that generates all the desired BCT combinations.
# 
# Note that it uses generate_bct_combinations() to produce the 
# cross product of a set of SDRAM configs and device configs.
# It also extends this list with some custom configurations.
#
def generate_bct_list():
    """Generate the BCT list for Harmony."""
    result = []
    result.extend(custom_configs)
    result.extend(generate_bct_combinations(sdram_configs, device_configs))
    return result

#
# Invoke the BCT processing code which will process command line arguments,
# validate input, generate all the requested output, manipulates Perforce,
# and provides instructions for completing the BCT generation process.
process_bcts(generate_bct_list())
