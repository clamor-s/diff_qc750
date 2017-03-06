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
    'whistler_12Mhz_H5PS1G63EFR_150Mhz_512MB',
    'whistler_12Mhz_H5PS1G63EFR_200Mhz_512MB',
    'whistler_12Mhz_H5PS1G63EFR_333Mhz_512MB',
    'whistler_12Mhz_H8TBR00Q0MLR-0DM_100Mhz_256MB',
    'whistler_12Mhz_H8TBR00Q0MLR-0DM_150Mhz_256MB',
    'whistler_12Mhz_H8TBR00Q0MLR-0DM_200Mhz_256MB',
    'whistler_12Mhz_H8TBR00Q0MLR-0DM_250Mhz_256MB',
    'whistler_12Mhz_H8TBR00Q0MLR-0DM_300Mhz_256MB',
    'whistler_26Mhz_H5PS1G63EFR_300Mhz_512MB',
    'whistler_26Mhz_H8TBR00Q0MLR-0DM_300Mhz_256MB',
    'whistler_a02_12Mhz_H5PS1G63EFR_333Mhz_512MB',
    'whistler_a02p_pop_12Mhz_H5LD1G23MFR-S6_300Mhz_256MB',
]

device_configs = [
    'emmc_THGBM1G6D4EBAI4',
    'emmc_THGBM1G6D4EBAI4_x8',
    '2K8Nand_K5E2G16ACM-D060',
]

custom_configs = [
    # Configurations with special notes.
    [
        'whistler_a02p_pop_12MHz_EDB4032B1PB6DF_333Mhz_512MB',
        'emmc_THGBM1G6D4EBAI4',
        'whistler_a02p_pop_12MHz_EDB4032B1PB6DF_333Mhz_512MB_emmc_THGBM1G6D4EBAI4',
        'Standard BCT for SW.'
    ],
    [
        'whistler_a02p_pop_12MHz_EDB4032B1PB6DF_333Mhz_512MB',
        'emmc_THGBM1G6D4EBAI4_x8',
        'whistler_a02p_pop_12MHz_EDB4032B1PB6DF_333Mhz_512MB_emmc_THGBM1G6D4EBAI4_x8',
        'Standard BCT for SW.'
    ],
    [
        'whistler_a02p_pop_12MHz_EDB4032B1PB6DF_333Mhz_512MB',
        '2K8Nand_K5E2G16ACM-D060',
        'whistler_a02p_pop_12MHz_EDB4032B1PB6DF_333Mhz_512MB_2K8Nand_K5E2G16ACM-D060',
        'Standard BCT for SW.'
    ],
    [
        'whistler_a02_12Mhz_H5PS1G63EFR_333Mhz_512MB',
        'emmc_THGBM1G6D4EBAI4',
        'whistler_a02_12Mhz_H5PS1G63EFR_333Mhz_512MB_emmc_THGBM1G6D4EBAI4',
        'Tested across PVT. Standard BCT for SW.'
    ],
    [
        'whistler_a02_12Mhz_H5PS1G63EFR_333Mhz_512MB',
        'emmc_THGBM1G6D4EBAI4_x8',
        'whistler_a02_12Mhz_H5PS1G63EFR_333Mhz_512MB_emmc_THGBM1G6D4EBAI4_x8',
        'Tested across PVT. Standard BCT for SW.'
    ],
    [
        'whistler_a02_12Mhz_H5PS1G63EFR_333Mhz_512MB',
        '2K8Nand_K5E2G16ACM-D060',
        'whistler_a02_12Mhz_H5PS1G63EFR_333Mhz_512MB_2K8Nand_K5E2G16ACM-D060',
        'Tested across PVT. Standard BCT for SW.'
    ],
    [
        'whistler_a02p_pop_12Mhz_H5LD1G23MFR-S6_300Mhz_256MB',
        'emmc_THGBM1G6D4EBAI4',
        'whistler_a02p_pop_12Mhz_H5LD1G23MFR-S6_300Mhz_256MB_emmc_THGBM1G6D4EBAI4',
        'Tested across PVT. Standard BCT for SW. Hynix LPDDR2.'
    ],
    [
        'whistler_a02p_pop_12Mhz_H5LD1G23MFR-S6_300Mhz_256MB',
        'emmc_THGBM1G6D4EBAI4_x8',
        'whistler_a02p_pop_12Mhz_H5LD1G23MFR-S6_300Mhz_256MB_emmc_THGBM1G6D4EBAI4_x8',
        'Tested across PVT. Standard BCT for SW. Hynix LPDDR2.'
    ],
    [
        'whistler_a02p_pop_12Mhz_H5LD1G23MFR-S6_300Mhz_256MB',
        '2K8Nand_K5E2G16ACM-D060',
        'whistler_a02p_pop_12Mhz_H5LD1G23MFR-S6_300Mhz_256MB_2K8Nand_K5E2G16ACM-D060',
        'Tested across PVT. Standard BCT for SW. Hynix LPDDR2.'
    ],

    # Special configuration for 4K page NAND
    [
        'whistler_12Mhz_H8TBR00Q0MLR-0DM_300Mhz_256MB',
        '4K8Nand_K9LBG08U0M-PCB0',
        'whistler_12Mhz_H8TBR00Q0MLR-0DM_300Mhz_256MB_4K8Nand_K9LBG08U0M-PCB0',
        '4K page NAND'
    ]
]

#
# Function that generates all the desired BCT combinations.
# 
# Note that it uses generate_bct_combinations() to produce the 
# cross product of a set of SDRAM configs and device configs.
# It also extends this list with some custom configurations.
#
def generate_bct_list():
    """Generate the BCT list for Whistler."""
    result = []

    # Note that placing the custom configs before the common
    # cross product variations ensures they appear at the front of the
    # tables on the wiki.
    result.extend(custom_configs)
    result.extend(generate_bct_combinations(sdram_configs, device_configs))

    return result

#
# Invoke the BCT processing code which will process command line arguments,
# validate input, generate all the requested output, manipulates Perforce,
# and provides instructions for completing the BCT generation process.
process_bcts(generate_bct_list())
