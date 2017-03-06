/*************************************************************************************************
 *
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 ************************************************************************************************/

/**
 * @defgroup filesystem Flash File System
 *
 */

/**
 * @file drv_fs_cfg.h Some filesystem configurations
 *
 */

#ifndef DRV_FS_CFG_H
#define DRV_FS_CFG_H

/*************************************************************************************************
 * Project header files
 ************************************************************************************************/
#include "icera_global.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

/*************************************************************************************************
 * Macros
 ************************************************************************************************/

/**
 * Root directories
 *
 * List of misc root dir names used as mount point for a given
 * device.
 */
#define DRV_FS_DIR_NAME_PARTITION1 "/"        /** Used to mount DRV_FS_DEVICE_NAME_PARTITION1 */
#define DRV_FS_DIR_NAME_ZERO_CD    "/zero-cd" /** Used to mount DRV_FS_DEVICE_NAME_ZERO_CD */
#define DRV_FS_DIR_NAME_BACKUP     "/backup"  /** Used to mount DRV_FS_DEVICE_NAME_BACKUP */
#define DRV_FS_DIR_NAME_TIIO       "/tiio"

/**
 * Directories
 *
 * Some directories used to store different files...
 * (created through mkdir)
 */
#define DRV_FS_APP_DIR_NAME        "/app"
#define DRV_FS_DATA_DIR_NAME       "/data"
#define DRV_FS_DATA_FACT_DIR_NAME      "/data/factory" /* MUST be considered as a R/O folder at runtime... */

#define DRV_FS_DATA_MODEM_NV_DIR_NAME  "/data/modem"

#define DRV_FS_DATA_CONFIG_DIR_NAME    "/data/config" /* MUST be considered as a R/O folder at runtime... */
#define DRV_FS_DATA_MODEM_DIR_NAME DRV_FS_DATA_DIR_NAME"/modem"
#define DRV_FS_DEBUG_DATA_DIR_NAME DRV_FS_DATA_DIR_NAME"/debug"
#define DRV_FS_DATA_SSL_DIR_NAME DRV_FS_DATA_DIR_NAME"/ssl"
#define DRV_FS_DATA_GPS_DIR_NAME DRV_FS_DATA_DIR_NAME"/gps"

#define DRV_FS_WEBUI_DIR_NAME DRV_FS_DIR_NAME_ZERO_CD"/webui"

/**
 * File names
 *
 */
#define TEMP_AUDIO_FILE      DRV_FS_DATA_DIR_NAME"/audio_tmp.bin"          /** Temporary file created during audio parameters file update */
#define AUDIO_ECAL_FILE      DRV_FS_DATA_DIR_NAME"/audioEcal.bin"          /** File containing audio electrical calibration */
#define AUDIO_TCAL_FILE      DRV_FS_DATA_DIR_NAME"/audioTcal.bin"          /** File containing audio transducer calibration */
#define HIF_CONFIG_FILE      DRV_FS_DATA_DIR_NAME"/hifConfig.bin"          /** File containing host interface configuration */
#define EXTAUTH_FILE         DRV_FS_DATA_DIR_NAME"/extAuth.bin"            /** File containing external authentication attributes */
#define NVRAM_RW_FILE              DRV_FS_DATA_DIR_NAME"/nvram-rw.bin"
#define CAL_PATCH_HISTORY          DRV_FS_DATA_DIR_NAME"/calibration_change.log"
#define FT_CUSTOM_FILE             DRV_FS_DATA_DIR_NAME"/ft_custom.bin"
#define FT_PREV_PROC_FILE          DRV_FS_DATA_DIR_NAME"/ft_prev_proc.bin"
#define FT_DATA_FILE               DRV_FS_DATA_DIR_NAME"/ft_data.bin"
#define MDMBACKUP_APPLICATION_FILE DRV_FS_APP_DIR_NAME"/modem.backup.wrapped"    /** MDM backup application binary */
#define TEMP_UPDATE_FILE           DRV_FS_APP_DIR_NAME"/dummy_archive.tmp"       /** Temporary file created during archive update */

#define ENGINEERING_MODE_FILE      DRV_FS_DATA_MODEM_DIR_NAME"/engineering_mode.bin"
#define BOOT_CONFIG_FILE           DRV_FS_DATA_FACT_DIR_NAME"/bootConfig.bin"

/*************************************************************************************************
 * Public Types
 ************************************************************************************************/

/*************************************************************************************************
 * Public variable declarations
 ************************************************************************************************/

/*************************************************************************************************
 * Public function declarations
 ************************************************************************************************/

#endif /* #ifndef DRV_FS_CFG_H*/

/** @} END OF FILE */

