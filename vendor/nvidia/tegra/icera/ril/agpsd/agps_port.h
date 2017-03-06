/*************************************************************************************************
 * NVidia Corporation
 * Copyright (c) 2011
 * All rights reserved
 *************************************************************************************************
  * $Revision: #1 $
 * $Date: 2010/08/04 $
 * $Author: gheinrich $
 ************************************************************************************************/

/**
 * @addtogroup apgs
 * @{
 */

/**
 * @file agps_port.h NVidia aGPS RIL OS Port Layer
 *
 */

#ifndef AGPS_PORT_H
#define AGPS_PORT_H

/*************************************************************************************************
 * Project header files
 ************************************************************************************************/

#include "agps.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

#include <semaphore.h>

/*************************************************************************************************
 * Macros
 ************************************************************************************************/

/*************************************************************************************************
 * Public Types
 ************************************************************************************************/

/** type definition for a thread */
typedef pthread_t agps_PortThread;

/** type definition for a semaphore */
typedef sem_t agps_PortSem;

/**
* Enumeration of option types
*/
typedef enum
{
    AGPS_OPTION_FLAG,                            /* no parameter */
    AGPS_OPTION_INT,                             /* integer parameter */
    AGPS_OPTION_STRING,                          /* string parameter */
} agps_PortOptionType;

/**
* type definition for an option
*/
typedef struct
{
    /** long option, e.g. --help */
    const char *long_opt;
    /** short option, e.g. -h */
    char short_opt;
    /** option type (flag, integer, string) */
    agps_PortOptionType type;
    /** pointer to pointer to variable. The nature of the variable
     *  depends on the option type.
     *  For flags, an (int*) must be provided
     *  For integers, an (int*) must be provided
     *  For stringsm a (char**) must be provided
     */
    void *var;
    /** help message */
    const char *help;
} agps_PortOption;

/*************************************************************************************************
 * Public variable declarations
 ************************************************************************************************/

/*************************************************************************************************
 * Public function declarations
 ************************************************************************************************/

/**
 * Create a thread..
 *
 * @param thread         thread handler
 * @param start_routine  thread routine
 * @param arg            pointer to args passed to thread
 *                       routine.
 *
 * @return int 0 on success, value < 0 on failure.
 */
int agps_PortCreateThread(agps_PortThread *thread, void *(*start_routine)(void *), void *arg);

/**
 * Init semaphore
 *
 * @param sem     pointer to existing agps_PortSem
 * @param shared  is sem shared bewteen processes. (0 is enough
 *                to share sem between threads of a same
 *                process.)
 * @param value   init sem token value
 *
 * @return int    0 on success, -1 on failure and errno is set
 *                appropriately.
 */
int agps_PortSemInit(agps_PortSem *sem, int shared, int value);

/**
 * Post sem token.
 *
 * Might "unlock" thread or process waiting for sem token.
 *
 * @param sem  pointer to initialised agps_PortSem
 *
 * @return int 0 on success, -1 on failure and errno is set
 *             appropriately.
 */
int agps_PortSemPost(agps_PortSem *sem);

/**
 * Wait for sem token.
 *
 * "Locking" call if num of token == 0
 *
 * @param sem  pointer to initialised agps_PortSem
 *
 * @return int 0 on success, -1 on failure and errno is set
 *             appropriately.
 */
int agps_PortSemWait(agps_PortSem *sem);

/**
 * Open shared library
 *
 * @param libpath Path to library
 * @return handle to library, or NULL if library could not be
 *         opened
 */
void *agps_PortOpenLib(const char *path);

/**
* Get shared library symbol
* @param lib_handle Handle returned by agps_PortOpenLib()
* @param symbol Name of symbol
* @return symbol
*/
void *agps_PortGetLibSymbol(void *lib_handle, const char *symbol);

/**
* Print usage
* @param options Array of options
*/
void agps_PortUsage(agps_PortOption *options);

/**
* Parse arguments. Arguments from the command line are
* overwritten by Android properties
*
* Note that only AGPS_OPTION_FLAG type of options cannot be
* overwritten by Android properties
*
* All options must have a long format AND a short format
*
* @param options     Pointer to array of options
* @param prop_prefix Prefix to be used when looking for Android
*                    properties
* @param argc        Argument count (from command line)
* @param argv        Array of arguments (from command line)
* @return error code
*/
agps_ReturnCode agps_PortParseArgs(agps_PortOption *options, char *prop_prefix, int argc, char *argv[]);

/**
* Free memory previously allocated by agps_DaemonParseArgs()
* @param options Pointer to array of options
*/
void agps_PortCleanupArgs(agps_PortOption *options);

#endif /* AGPS_PORT_H */

