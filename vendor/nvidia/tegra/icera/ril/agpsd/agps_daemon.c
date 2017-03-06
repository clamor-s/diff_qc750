
/*************************************************************************************************
 * NVidia Corporation
 * Copyright (c) 2011
 * All rights reserved
 *************************************************************************************************
 * $Revision: #1 $
 * $Date: $
 * $Author: $
 ************************************************************************************************/

/**
 * @addtogroup agps
 * @{
 */

/**
 * @file agps_daemon.c NVidia aGPS RIL Daemon
 *
 */

/*************************************************************************************************
 * Project header files. The first in this list should always be the public interface for this
 * file. This ensures that the public interface header file compiles standalone.
 ************************************************************************************************/

#include "agps.h"
#include "agps_port.h"
#include "agps_core.h"
#include "agps_daemon.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

/**************************************************************************************************************/
/* Private Macros  */
/**************************************************************************************************************/

#define AGPS_PROPERTY_PREFIX "agpsd."

#define AGPS_EXIT_SLEEP_TIME_SECONDS 5

/**************************************************************************************************************/
/* Private Functions Declarations (if needed) */
/**************************************************************************************************************/

/**************************************************************************************************************/
/* Private Type Declarations */
/**************************************************************************************************************/

/**
* type definition for the daemon state
*/
typedef struct
{
    agps_PortSem exit_sem;
} agps_DaemonState;

/**************************************************************************************************************/
/* Private Variable Definitions  */
/**************************************************************************************************************/

/**
 * the daemon state
 */
static agps_DaemonState agps_daemon_state;


/*************************************************************************************************
 * Private function definitions
 ************************************************************************************************/


/*************************************************************************************************
 * Public function definitions (Not doxygen commented, as they should be exported in the header
 * file)
 ************************************************************************************************/
int main(int argc, char *argv[])
{
    int ret;
    int res;
    int showhelp = 0;
    agps_PortThread thread;
    agps_CoreConfig agps_core_config;
    agps_ReturnCode agps_ret_code;

    /* Default params values: */
    char *device_path = NULL;
    char *gpal_lib_path = NULL;
    char *test_case = NULL;

    /**
     * Options
     */
    agps_PortOption agps_daemon_options[] =
    {
        { "help",     'h', AGPS_OPTION_FLAG,   &showhelp,      "Display this help"},
        { "dev_path", 'd', AGPS_OPTION_STRING, &device_path,   "aGPS device file path"},
        { "lib_path", 'l', AGPS_OPTION_STRING, &gpal_lib_path, "aGPS GPAL library path"},
        { "test_case",'t', AGPS_OPTION_STRING, &test_case,     "test mode. Valid tests are:\n"
                                                               "           TC_17_2_2_1\n"
                                                               "           UL_DCCH:123456789abcdef (send UL-DCCH vector)\n"
                                                               "           CAL\n"
            },
        /* terminator */
        { 0, 0, 0, NULL, NULL},
    };

    /* parse options */
    agps_ret_code = agps_PortParseArgs(agps_daemon_options, AGPS_PROPERTY_PREFIX, argc, argv);

    if (agps_ret_code != AGPS_OK)
    {
        ALOGE("%s: Error %d when parsing args\n", __FUNCTION__, agps_ret_code );
        showhelp=1;
    }

    /* the following do while(0) is used to avoid nested if/return statements */
    do
    {
        /* show help menu if required */
        if (showhelp)
        {
            agps_PortUsage(agps_daemon_options);
            ret = -1;
            break;
        }

        /* sanity checks */
        if (NULL == device_path)
        {
            ret = -1;
            break;
        }

        /* sanity checks */
        if ((test_case == NULL) && (NULL == gpal_lib_path))
        {
            /* exit silently */
            ret = -1;
            break;
        }

        ALOGI("%s Built %s %s\n", __FILE__, __DATE__, __TIME__ );

        /* fill in CORE layer config items */
        agps_core_config.device_path = device_path;
        agps_core_config.gpal_lib_path = gpal_lib_path;
        agps_core_config.test_case = test_case;

        /* create the semaphore we will be waiting on to exit */
        if (agps_PortSemInit(&agps_daemon_state.exit_sem, 0, 0) != 0)
        {
            ALOGE("%s: Failed to initialize semaphore.\n", __FUNCTION__);
            ret = -1;
            break;
        }

        /* start main thread */
        if (agps_PortCreateThread(&thread, agps_CoreMain, (void*)&agps_core_config) != 0)
        {
            ALOGE("%s: Failed to start main AGRIL thread.\n", __FUNCTION__);
            ret = -1;
            break;
        }

        /* wait for exit signal */
        agps_PortSemWait(&agps_daemon_state.exit_sem);

        ALOGI("%s: Got exit signal, leaving now\n", __FUNCTION__ );

        ret = 0;

    } while (0);

    /* clean up */
    agps_PortCleanupArgs(agps_daemon_options);

    /* sleep for a bit in order to avoid spinning here if the aGPS service is
       started in a loop */
    sleep(AGPS_EXIT_SLEEP_TIME_SECONDS);

    return ret;
}

void agps_DaemonExit(void)
{
    /* send exit signal to main thread */
    agps_PortSemPost(&agps_daemon_state.exit_sem);
}

