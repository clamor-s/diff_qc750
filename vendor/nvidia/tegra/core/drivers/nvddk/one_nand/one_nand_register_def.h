/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * @brief <b>nVIDIA Driver Development Kit:
 *           Register definition of the mux one nand and flex muxone 
 *           nand devices.</b>
 *
 * @b Description:  Defines the complete register set of the mux one nand and
 *                  flex mux one nand devices.
 *                  These definitions are used to access the register of the 
 *                  device using the drf def macros.
 *
 */

#ifndef INCLUDED_ONE_NAND_REGISTER_DEF_H
#define INCLUDED_ONE_NAND_REGISTER_DEF_H

// Boot/data memory area
#define ONE_NAND_FLEX_BOOT_MEMORY_MAIN_0_ADDRESS                    0x0000
#define ONE_NAND_FLEX_DATA_MEMORY_MAIN_0_ADDRESS                    0x0200
#define ONE_NAND_FLEX_BOOT_MEMORY_SPARE_0_ADDRESS                   0x8000
#define ONE_NAND_FLEX_DATA_MEMORY_SPARE_0_ADDRESS                   0x8010


// Manufacturere ID register
#define ONE_NAND_MAN_ID_0_ADDRESS                                   0xF000
#define ONE_NAND_MAN_ID_0_MANUFID_RANGE                             15:0

// Device ID register
#define ONE_NAND_DEV_ID_0_ADDRESS                                   0xF001
#define ONE_NAND_DEV_ID_0_DEVICEID_RANGE                            15:0

#define ONE_NAND_DEV_ID_0_DENSITY_RANGE                    7:4
#define ONE_NAND_DEV_ID_0_DENSITY_Mb128                    0

#define ONE_NAND_DEV_ID_0_SEPERATION_RANGE                 9:8
#define ONE_NAND_DEV_ID_0_SEPERATION_SLC                   0
#define ONE_NAND_DEV_ID_0_SEPERATION_MLC                   1
#define ONE_NAND_DEV_ID_0_SEPERATION_FLEX                  2
#define ONE_NAND_DEV_ID_0_SEPERATION_RES                   3


// Version ID register
#define ONE_NAND_VER_ID_0_ADDRESS                                   0xF002
#define ONE_NAND_VER_ID_0_VERSIONID_RANGE                           15:0

// Data buffer size register 
#define ONE_NAND_DATA_BUF_SIZE_0_ADDRESS                            0xF003
#define ONE_NAND_DATA_BUF_SIZE_0_RANGE                              15:0

// Boot buffer size register 
#define ONE_NAND_BOOT_BUF_SIZE_0_ADDRESS                            0xF004
#define ONE_NAND_BOOT_BUF_SIZE_0_RANGE                              15:0

// Amount of Buffer register
// Data buffer amount - The numbetr of data buffers = 2 (2^N, N =1)
// Boot buffer amount - The numbetr of boot buffers = 1 (2^N, N =0)
#define ONE_NAND_BUFFER_AMOUNT_0_ADDRESS                            0xF005
#define ONE_NAND_BUFFER_AMOUNT_0_DATA_BUFFER_AMOUNT_RANGE           7:0
#define ONE_NAND_BUFFER_AMOUNT_0_BOOT_BUFFER_AMOUNT_RANGE           15:8

// Technology register
#define ONE_NAND_TECHNOLOGY_0_ADDRESS                               0xF006
#define ONE_NAND_TECHNOLOGY_0_TECHNOLOGY_RANGE                      15:0
#define ONE_NAND_TECHNOLOGY_0_TECHNOLOGY_NAND_SLC                   0
#define ONE_NAND_TECHNOLOGY_0_TECHNOLOGY_NAND_MLC                   1

// Start address 1 Register
#define ONE_NAND_START_ADD_1_0_ADDRESS                              0xF100

// Nand Flash block Address
#define ONE_NAND_START_ADD_1_0_FBA_RANGE                            9:0

// Nand flash core select for DDP (Device Flash core Select)
#define ONE_NAND_START_ADD_1_0_DFS_RANGE                            15:15

// Start address 2 Register
#define ONE_NAND_START_ADD_2_0_ADDRESS                              0xF101

// Nand flash device buffer RAM select 
#define ONE_NAND_START_ADD_2_0_DBS_RANGE                            15:15


// Start address 8 Register
#define ONE_NAND_START_ADD_8_0_ADDRESS                              0xF107

// Nand flash sector address
#define ONE_NAND_START_ADD_8_0_FSA_RANGE                            1:0
#define ONE_NAND_START_ADD_8_0_FSA_SECTOR_0                         0
#define ONE_NAND_START_ADD_8_0_FSA_SECTOR_1                         1
#define ONE_NAND_START_ADD_8_0_FSA_SECTOR_2                         2
#define ONE_NAND_START_ADD_8_0_FSA_SECTOR_3                         3

// Nand flash page address
#define ONE_NAND_START_ADD_8_0_FPA_RANGE                            8:2

// Start buffer register Register
#define ONE_NAND_SECTOR_COUNT_0_ADDRESS                             0xF200

// Buffer RAM sector address (BSA) is the sector 0-3 address in 
// the internal BootRAM and DataRAM where data is placed.
#define ONE_NAND_SECTOR_COUNT_0_BSA_RANGE                           11:8
#define ONE_NAND_SECTOR_COUNT_0_BSA3_RANGE                          11:11
#define ONE_NAND_SECTOR_COUNT_0_BSA3_BOOTROM                        0
#define ONE_NAND_SECTOR_COUNT_0_BSA3_DATARAM                        1

#define ONE_NAND_SECTOR_COUNT_0_BSA2_RANGE                          10:10
#define ONE_NAND_SECTOR_COUNT_0_BSA2_DATARAM0                       0
#define ONE_NAND_SECTOR_COUNT_0_BSA2_DATARAM1                       1

// Buffer sector Count
// This gives the number of sectors and start secotr is defined 
// by FSA
// FSA = 0, BSC = 0, then all 8 sector
// FSA = 0, BSC = 1 the only sector 0
#define ONE_NAND_SECTOR_COUNT_0_BSC_COUNT_RANGE                     2:0
#define ONE_NAND_SECTOR_COUNT_0_BSC_COUNT_ALL_SECTOR                0
#define ONE_NAND_SECTOR_COUNT_0_BSC_COUNT_SECTOR_0                  1
#define ONE_NAND_SECTOR_COUNT_0_BSC_COUNT_SECTOR_1                  2
#define ONE_NAND_SECTOR_COUNT_0_BSC_COUNT_SECTOR_2                  3
#define ONE_NAND_SECTOR_COUNT_0_BSC_COUNT_SECTOR_3                  4
#define ONE_NAND_SECTOR_COUNT_0_BSC_COUNT_SECTOR_4                  5
#define ONE_NAND_SECTOR_COUNT_0_BSC_COUNT_SECTOR_5                  6
#define ONE_NAND_SECTOR_COUNT_0_BSC_COUNT_SECTOR_6                  7

// Command resgister
#define ONE_NAND_COMMAND_0_ADDRESS                                  0xF220
#define ONE_NAND_COMMAND_0_COMMAND_RANGE                            15:0
#define ONE_NAND_COMMAND_0_COMMAND_LOAD_PAGE_INTO_BUFFER            0x0000
#define ONE_NAND_COMMAND_0_COMMAND_SUPERLOAD_PAGE_FROM_BUFFER       0x0003
#define ONE_NAND_COMMAND_0_COMMAND_PAGE_RECOVERY_READ               0x0005
#define ONE_NAND_COMMAND_0_COMMAND_PROGRAM_PAGE                     0x0080
#define ONE_NAND_COMMAND_0_COMMAND_FINISH_PROGRAM_OP                0x0080
#define ONE_NAND_COMMAND_0_COMMAND_CACHE_PROGRAM                    0x007F
#define ONE_NAND_COMMAND_0_COMMAND_UNLOCK_BLOCK                     0x0023
#define ONE_NAND_COMMAND_0_COMMAND_LOCK_BLOCK                       0x002A
#define ONE_NAND_COMMAND_0_COMMAND_LOCK_TIGHT_BLOCK                 0x002C
#define ONE_NAND_COMMAND_0_COMMAND_UNLOCK_ALL_BLOCK                 0x0027
#define ONE_NAND_COMMAND_0_COMMAND_BLOCK_ERASE                      0x0094
#define ONE_NAND_COMMAND_0_COMMAND_ERASE_SUSPEND                    0x00B0
#define ONE_NAND_COMMAND_0_COMMAND_ERASE_RESUME                     0x0030
#define ONE_NAND_COMMAND_0_COMMAND_RESET_NAND                       0x00F0
#define ONE_NAND_COMMAND_0_COMMAND_RESET_HOT                        0x00F3
#define ONE_NAND_COMMAND_0_COMMAND_OTP_ACCESS                       0x0065
#define ONE_NAND_COMMAND_0_COMMAND_ACESS_PART_INFO_BLOCK            0x0066


// System configuartion 1 register 
#define ONE_NAND_SYSTEM_CONFIG_1_0_ADDRESS                          0xF221

// Boot buffer write protect status
#define ONE_NAND_SYSTEM_CONFIG_1_0_BWPS_RANGE                       0:0
#define ONE_NAND_SYSTEM_CONFIG_1_0_BWPS_LOCKED                      0

// Write mode
#define ONE_NAND_SYSTEM_CONFIG_1_0_WRITE_MODE_RANGE                 1:1
#define ONE_NAND_SYSTEM_CONFIG_1_0_WRITE_MODE_ASYNCH                0
#define ONE_NAND_SYSTEM_CONFIG_1_0_WRITE_MODE_SYNCH                 1

// Enable HF
#define ONE_NAND_SYSTEM_CONFIG_1_0_HF_RANGE                         2:2
#define ONE_NAND_SYSTEM_CONFIG_1_0_HF_DISABLE                       0
#define ONE_NAND_SYSTEM_CONFIG_1_0_HF_ENABLE                        1
#define ONE_NAND_SYSTEM_CONFIG_1_0_HF_UNDER_66MHZ                   0
#define ONE_NAND_SYSTEM_CONFIG_1_0_HF_OVER_66MHZ                    1

// Ready configuration information
#define ONE_NAND_SYSTEM_CONFIG_1_0_REDY_CONF_RANGE                  4:4
#define ONE_NAND_SYSTEM_CONFIG_1_0_REDY_ACTIVE_WITH_VALID_DATA      0
#define ONE_NAND_SYSTEM_CONFIG_1_0_REDY_ACTIVE_ONE_CLOCK_BEFORE_VALID_DATA   1

// IO buffer enable
#define ONE_NAND_SYSTEM_CONFIG_1_0_IOBE_RANGE                       5:5
#define ONE_NAND_SYSTEM_CONFIG_1_0_IOBE_DISABLE                     0
#define ONE_NAND_SYSTEM_CONFIG_1_0_IOBE_ENABLE                      1

// Inetrrupt polarity
#define ONE_NAND_SYSTEM_CONFIG_1_0_INT_POL_RANGE                    6:6
#define ONE_NAND_SYSTEM_CONFIG_1_0_INT_POL_LOW_ON_READY             0
#define ONE_NAND_SYSTEM_CONFIG_1_0_INT_POL_HIGH_ON_READY            1

// Ready polarity
#define ONE_NAND_SYSTEM_CONFIG_1_0_READY_POL_RANGE                  7:7
#define ONE_NAND_SYSTEM_CONFIG_1_0_READY_POL_LOW_FOR_READY          0
#define ONE_NAND_SYSTEM_CONFIG_1_0_READY_POL_HIGH_FOR_READY         1 

#define ONE_NAND_SYSTEM_CONFIG_1_0_ECC_RANGE                        8:8
#define ONE_NAND_SYSTEM_CONFIG_1_0_ECC_ENABLE                       0 // default
#define ONE_NAND_SYSTEM_CONFIG_1_0_ECC_DISABLE                      1

// Burst length
#define ONE_NAND_SYSTEM_CONFIG_1_0_BL_RANGE                         11:9
#define ONE_NAND_SYSTEM_CONFIG_1_0_BL_CONTINUOUS                    0
#define ONE_NAND_SYSTEM_CONFIG_1_0_BL_WORDS_4                       1
#define ONE_NAND_SYSTEM_CONFIG_1_0_BL_WORDS_8                       2
#define ONE_NAND_SYSTEM_CONFIG_1_0_BL_WORDS_16                      3
#define ONE_NAND_SYSTEM_CONFIG_1_0_BL_WORDS_32                      4


// Burst read write latency
#define ONE_NAND_SYSTEM_CONFIG_1_0_BRWL_RANGE                       14:12

// Read mode.
#define ONE_NAND_SYSTEM_CONFIG_1_0_READMODE_RANGE                   15:15
#define ONE_NAND_SYSTEM_CONFIG_1_0_READMODE_ASYNCH                  0
#define ONE_NAND_SYSTEM_CONFIG_1_0_READMODE_SYNCH                   1


// Controller status register
#define ONE_NAND_CONTROLLER_STATUS_0_ADDRESS                        0xF240

// This bit determines if there is a timeout for load, program and erase 
// operatuion.
#define ONE_NAND_CONTROLLER_STATUS_0_TIMEOUT_RANGE                  0:0
#define ONE_NAND_CONTROLLER_STATUS_0_TIMEOUT_NO_ERROR               0
#define ONE_NAND_CONTROLLER_STATUS_0_TIMEOUT_ERROR                  1

// Current cache program status
#define ONE_NAND_CONTROLLER_STATUS_0_CURRENT_RANGE                  1:1
#define ONE_NAND_CONTROLLER_STATUS_0_CURRENT_PASS                   0
#define ONE_NAND_CONTROLLER_STATUS_0_CURRENT_FAIL                   1

// Previous cache program status
#define ONE_NAND_CONTROLLER_STATUS_0_PREVIOUS_RANGE                 2:2
#define ONE_NAND_CONTROLLER_STATUS_0_PREVIOUS_PASS                  0
#define ONE_NAND_CONTROLLER_STATUS_0_PREVIOUS_FAIL                  1

// Ist block OTP lock status
// This bit shows ehether the 1st block OTP is locked or uinlocked
#define ONE_NAND_CONTROLLER_STATUS_0_OTP_BL_RANGE                   5:5
#define ONE_NAND_CONTROLLER_STATUS_0_OTP_BL_UNLOCK                  0
#define ONE_NAND_CONTROLLER_STATUS_0_OTP_BL_LOCK                    1

// OTP lock status
// This bit shows ehether the OTP block is locked or uinlocked
#define ONE_NAND_CONTROLLER_STATUS_0_OTP_L_RANGE                    6:6
#define ONE_NAND_CONTROLLER_STATUS_0_OTP_L_LOCKED                   0
#define ONE_NAND_CONTROLLER_STATUS_0_OTP_L_UNLOCKED                 1

// PI lock status: This bit shows whether the PI block is locked or unlocked
#define ONE_NAND_CONTROLLER_STATUS_0_PIL_RANGE                      8:8
#define ONE_NAND_CONTROLLER_STATUS_0_PIL_UNLOCKED                   0
#define ONE_NAND_CONTROLLER_STATUS_0_PIL_LOCKED                     1

// This bit shows the overall Error status, including Load Reset, 
// Program Reset, and Erase Reset status.
// In case of 2X Cache Program, Error bit will show the accumulative error 
// status of 2X Cache Program operation, so that if an error occurs during
// 2X Cache Program, this bit will stay as Fail status, until the end of 2X 
// Cache Program.
// In case of 2X Program, Error bit will indicate the 2X Program fail, 
// regardless of Plane1 or Plane2.
#define ONE_NAND_CONTROLLER_STATUS_0_ERROR_RANGE                    10:10
#define ONE_NAND_CONTROLLER_STATUS_0_ERROR_PASS                     0
#define ONE_NAND_CONTROLLER_STATUS_0_ERROR_FAIL                     1

// This bit shows the erase suspend status
#define ONE_NAND_CONTROLLER_STATUS_0_ERASE_RANGE                    11:11
#define ONE_NAND_CONTROLLER_STATUS_0_ERASE_RESUME                   0
#define ONE_NAND_CONTROLLER_STATUS_0_ERASE_SUSPEND                  1


// This bit shows the Program Operation status.
// In 2X Cache Program Operation, Program bit shows the overall status of 2X 
// Cache Program process.
#define ONE_NAND_CONTROLLER_STATUS_0_PROGRAM_RANGE                  12:12
#define ONE_NAND_CONTROLLER_STATUS_0_PROGRAM_READY                  0
#define ONE_NAND_CONTROLLER_STATUS_0_PROGRAM_BUSY_ERROR             1

// This bit shows the Load Operation status.
#define ONE_NAND_CONTROLLER_STATUS_0_LOAD_RANGE                     13:13
#define ONE_NAND_CONTROLLER_STATUS_0_LOAD_READY                     0
#define ONE_NAND_CONTROLLER_STATUS_0_LOAD_BUSY_ERROR                1


// This bit shows whether the host is loading data from the NAND Flash array 
// into the locked BootRAM or whether the host is performing a program/
// erase of a locked block of the NAND Flash array.
#define ONE_NAND_CONTROLLER_STATUS_0_LOCK_RANGE                     14:14
#define ONE_NAND_CONTROLLER_STATUS_0_LOCK_UNLOCKED                  0
#define ONE_NAND_CONTROLLER_STATUS_0_LOCK_LOCKED                    1


// This bit shows the overall internal status of the flex mux onenand device.
#define ONE_NAND_CONTROLLER_STATUS_0_ONGO_RANGE                     15:15
#define ONE_NAND_CONTROLLER_STATUS_0_ONGO_READY                     0
#define ONE_NAND_CONTROLLER_STATUS_0_ONGO_BUSY                      1

// Interrupt status register
#define ONE_NAND_INT_STATUS_0_ADDRESS                               0xF241

// Reset interrupt bit
#define ONE_NAND_INT_STATUS_0_RSTI_RANGE                            4:4
#define ONE_NAND_INT_STATUS_0_RSTI_OFF                              0
#define ONE_NAND_INT_STATUS_0_RSTI_PENDING                          1

// Erase inettrupt bit
#define ONE_NAND_INT_STATUS_0_EI_RANGE                              5:5
#define ONE_NAND_INT_STATUS_0_EI_OFF                                0
#define ONE_NAND_INT_STATUS_0_EI_PENDING                            1

// Write interrupt bit
#define ONE_NAND_INT_STATUS_0_WI_RANGE                              6:6
#define ONE_NAND_INT_STATUS_0_WI_OFF                                0
#define ONE_NAND_INT_STATUS_0_WI_PENDING                            1

// Read interrupt bit
#define ONE_NAND_INT_STATUS_0_RI_RANGE                              7:7
#define ONE_NAND_INT_STATUS_0_RI_OFF                                0
#define ONE_NAND_INT_STATUS_0_RI_PENDING                            1

// Master Interrupt status
#define ONE_NAND_INT_STATUS_0_INT_RANGE                             15:15
#define ONE_NAND_INT_STATUS_0_INT_OFF                               0
#define ONE_NAND_INT_STATUS_0_INT_PENDING                           1


// Start block address register
#define ONE_NAND_START_BLOCK_ADD_0_ADDRESS                          0xF24C
#define ONE_NAND_START_BLOCK_ADD_0_SBA_RANGE                        9:0

// Nand flash write protection register
#define ONE_NAND_WRITE_PROTECT_STATUS_0_ADDRESS                     0xF24E
// Lock tight status
#define ONE_NAND_WRITE_PROTECT_STATUS_0_LTS_RANGE                   0:0
#define ONE_NAND_WRITE_PROTECT_STATUS_0_LTS_UNLOCK                  0
#define ONE_NAND_WRITE_PROTECT_STATUS_0_LTS_LOCK_TIGHT              1

#define ONE_NAND_WRITE_PROTECT_STATUS_0_LS_RANGE                    1:1
#define ONE_NAND_WRITE_PROTECT_STATUS_0_LS_LOCKED                   1
#define ONE_NAND_WRITE_PROTECT_STATUS_0_LS_UNLOCKED                 0

#define ONE_NAND_WRITE_PROTECT_STATUS_0_US_RANGE                    2:2
#define ONE_NAND_WRITE_PROTECT_STATUS_0_US_UNLOCKED                 1
#define ONE_NAND_WRITE_PROTECT_STATUS_0_US_LOCKED                   0

#define ONE_NAND_WRITE_PROTECT_STATUS_0_STATUS_RANGE                2:0
#define ONE_NAND_WRITE_PROTECT_STATUS_0_STATUS_PROGRAM_ALLOWED      4


// ECC status register  for the mux one nand device
#define ONE_NAND_ECC_0_ADDRESS                                      0xFF00
#define ONE_NAND_ECC_0_SECTOR0_ERROR_RANGE                          1:0
#define ONE_NAND_ECC_0_SECTOR0_ERROR_MAX_ERROR                      2
#define ONE_NAND_ECC_0_SECTOR1_ERROR_RANGE                          3:2
#define ONE_NAND_ECC_0_SECTOR1_ERROR_MAX_ERROR                      2
#define ONE_NAND_ECC_0_SECTOR2_ERROR_RANGE                          5:4
#define ONE_NAND_ECC_0_SECTOR2_ERROR_MAX_ERROR                      2
#define ONE_NAND_ECC_0_SECTOR3_ERROR_RANGE                          7:6
#define ONE_NAND_ECC_0_SECTOR3_ERROR_MAX_ERROR                      2
#define ONE_NAND_ECC_0_SECTOR4_ERROR_RANGE                          9:8
#define ONE_NAND_ECC_0_SECTOR4_ERROR_MAX_ERROR                      2
#define ONE_NAND_ECC_0_SECTOR5_ERROR_RANGE                          11:10
#define ONE_NAND_ECC_0_SECTOR5_ERROR_MAX_ERROR                      2
#define ONE_NAND_ECC_0_SECTOR6_ERROR_RANGE                          13:12
#define ONE_NAND_ECC_0_SECTOR6_ERROR_MAX_ERROR                      2
#define ONE_NAND_ECC_0_SECTOR7_ERROR_RANGE                          15:14
#define ONE_NAND_ECC_0_SECTOR7_ERROR_MAX_ERROR                      2

// ECC status regsiters for the flex mux one nand devices
#define ONE_NAND_ECC_0_FLEX_SECTOR0_ERROR_MAX_ERROR                 0x10
#define ONE_NAND_ECC_0_FLEX_SECTOR1_ERROR_RANGE                     12:8
#define ONE_NAND_ECC_0_FLEX_SECTOR1_ERROR_MAX_ERROR                 0x10

// ECC status register 1
#define ONE_NAND_ECC_1_0_ADDRESS                                    0xFF00
#define ONE_NAND_ECC_1_0_FLEX_SECTOR0_ERROR_RANGE                   4:0
#define ONE_NAND_ECC_1_0_FLEX_SECTOR0_ERROR_MAX_ERROR               0x10
#define ONE_NAND_ECC_1_0_FLEX_SECTOR1_ERROR_RANGE                   12:8
#define ONE_NAND_ECC_1_0_FLEX_SECTOR1_ERROR_MAX_ERROR               0x10

// ECC status register 2
#define ONE_NAND_ECC_2_0_ADDRESS                                    0xFF01
#define ONE_NAND_ECC_2_0_FLEX_SECTOR3_ERROR_RANGE                   4:0
#define ONE_NAND_ECC_2_0_FLEX_SECTOR3_ERROR_MAX_ERROR               0x10
#define ONE_NAND_ECC_2_0_FLEX_SECTOR4_ERROR_RANGE                   12:8
#define ONE_NAND_ECC_2_0_FLEX_SECTOR4_ERROR_MAX_ERROR               0x10

// ECC status register 3
#define ONE_NAND_ECC_3_0_ADDRESS                                    0xFF02
#define ONE_NAND_ECC_3_0_FLEX_SECTOR5_ERROR_RANGE                   4:0
#define ONE_NAND_ECC_3_0_FLEX_SECTOR5_ERROR_MAX_ERROR               0x10
#define ONE_NAND_ECC_3_0_FLEX_SECTOR6_ERROR_RANGE                   12:8
#define ONE_NAND_ECC_3_0_FLEX_SECTOR6_ERROR_MAX_ERROR               0x10

// ECC status register 4
#define ONE_NAND_ECC_4_0_ADDRESS                                    0xFF03
#define ONE_NAND_ECC_4_0_FLEX_SECTOR7_ERROR_RANGE                   4:0
#define ONE_NAND_ECC_4_0_FLEX_SECTOR7_ERROR_MAX_ERROR               0x10
#define ONE_NAND_ECC_4_0_FLEX_SECTOR8_ERROR_RANGE                   12:8
#define ONE_NAND_ECC_4_0_FLEX_SECTOR8_ERROR_MAX_ERROR               0x10

#endif  // INCLUDED_ONE_NAND_REGISTER_DEF_H

