
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
 * @file agps_port.c NVidia aGPS RIL Daemon Port Layer
 *
 */

/*************************************************************************************************
 * Project header files. The first in this list should always be the public interface for this
 * file. This ensures that the public interface header file compiles standalone.
 ************************************************************************************************/

#include "agps_port.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

#include <getopt.h> /* getopt */
#include <stdlib.h> /* atoi */
#include <dlfcn.h> /* dlopen, ... */
#include <signal.h>
#include <time.h>  /* timer_create, .. */

/**************************************************************************************************************/
/* Private Macros  */
/**************************************************************************************************************/

#define AGPS_PORT_MAX_OPTS (20)

/**************************************************************************************************************/
/* Private Functions Declarations (if needed) */
/**************************************************************************************************************/

/**************************************************************************************************************/
/* Private Type Declarations */
/**************************************************************************************************************/

/**
* Enumeration of posix option types
*/
enum
{
    NO_ARG=0,                                    /* no argument */
    ONE_ARG=1,                                   /* one mandatory argument */
    OPT_ARG=2,                                   /* one optional argument */
} agps_PosixOptionType;

/**************************************************************************************************************/
/* Private Variable Definitions  */
/**************************************************************************************************************/

/*************************************************************************************************
 * Private function definitions
 ************************************************************************************************/

/*************************************************************************************************
 * Public function definitions (Not doxygen commented, as they should be exported in the header
 * file)
 ************************************************************************************************/

int agps_PortCreateThread(agps_PortThread *thread, void *(*start_routine)(void *), void *arg)
{
    return pthread_create(thread, NULL, start_routine, arg);
}

int agps_PortSemInit(agps_PortSem *sem, int shared, int value)
{
    return sem_init(sem, shared, value);
}

int agps_PortSemPost(agps_PortSem *sem)
{
    return sem_post(sem);
}

int agps_PortSemWait(agps_PortSem *sem)
{
    return sem_wait(sem);
}

void *agps_PortOpenLib(const char *path)
{
    return dlopen(path, RTLD_NOW);
}

void *agps_PortGetLibSymbol(void *lib_handle, const char *symbol)
{
    return dlsym(lib_handle, symbol);
}

void agps_PortUsage(agps_PortOption *options)
{
    agps_PortOption *option;

    option = options;

    ALOGI("\n");
    ALOGI("Usage:\n");
    ALOGI("agpsd <options> \n");

    while (option->var)
    {
        ALOGI("   --%s <value>, -%c <value>      %s\n", option->long_opt, option->short_opt, option->help);
        option++;
    }
}

agps_ReturnCode agps_PortParseArgs(agps_PortOption *options, char *prop_prefix, int argc, char *argv[])
{
    agps_ReturnCode ret = AGPS_OK;
    int res;

    struct option *posix_options;
    char optstring[PROPERTY_VALUE_MAX];
    char propname[PROPERTY_VALUE_MAX];
    char propvalue[PROPERTY_VALUE_MAX];
    agps_PortOption *option;
    int option_index;
    int option_val;
    int *int_option;
    int *flag_option;
    char **string_option;

    /* allocate memory for array of POSIX options */
    posix_options = (struct option *) malloc( sizeof(struct option) * (AGPS_PORT_MAX_OPTS+1) );

    if (NULL == posix_options)
    {
        return -AGPS_ERROR_OUT_OF_MEMORY;
    }

    /* the following do while (0) is used to avoid nested if/return statements */
    do
    {
        option_index = 0;
        optstring[0]='\0';

        /* go through all options */
        option = options;

        /* STEP 1: go through all options, prepare for parsing command-line
                   arguments */
        while (option->var)
        {
            /* break if we have more options than we support */
            if (option_index >= AGPS_PORT_MAX_OPTS)
            {
                ret = -AGPS_ERROR_TOO_MANY_OPTIONS;
                break;
            }

            /* sanity checks */
            if ((option->long_opt == NULL)
                || (option->short_opt == '\0'))
            {
                ret = -AGPS_ERROR_INVALID_OPTION_SPEC;
                break;
            }

            /* zero initialize option var */
            memset(option->var, 0, sizeof(option->var) );

            /* fill in posix option flags */
            posix_options[option_index].name = option->long_opt;
            posix_options[option_index].has_arg = option->type == AGPS_OPTION_FLAG ? NO_ARG : ONE_ARG;
            posix_options[option_index].flag = NULL;
            posix_options[option_index].val = option->short_opt;

            /* extend optstring */
            sprintf(optstring + strlen(optstring),
                    "%c%s",
                    option->short_opt,
                    option->type == AGPS_OPTION_FLAG ? "" : ":");

            /* move on to next option */
            option++;
            option_index++;
        }

        /* break now id there was an error */
        if (ret != AGPS_OK)
        {
            break;
        }

        /* terminate array of posix options */
        memset(&posix_options[++option_index], 0, sizeof(*posix_options) );

        /* STEP 2: parse command line options (iterate through all command line flags */
        while ((option_val = getopt_long(argc, argv, optstring, posix_options, &option_index)) != -1)
        {
            option = options;

            /* find matching opt */
            while (option->short_opt != option_val)
            {
                if (!option->var)
                {
                    ret = -AGPS_ERROR_OPTION_PARSING;
                    break;
                }

                option++;
            }

            /* break now if there was an error */
            if (ret != AGPS_OK)
            {
                break;
            }

            /* set parameter value */
            switch (option->type)
            {
            case AGPS_OPTION_FLAG:
                flag_option = (int*)option->var;
                *flag_option = 1;
                break;
            case AGPS_OPTION_INT:
                int_option = (int*)option->var;
                *int_option = atoi(optarg);
                break;
            case AGPS_OPTION_STRING:
                string_option = (char**)option->var;
                *string_option = malloc( strlen(optarg)+1 );
                if (*string_option == NULL)
                {
                    ret = -AGPS_ERROR_OUT_OF_MEMORY;
                    break;
                }
                strcpy(*string_option, optarg);
                break;
            }
        }

        /* break now if there was an error */
        if (ret != AGPS_OK)
        {
            break;
        }

        /* STEP3: now look for android properties - this will overwrite any parameter
           passed through the command line */

        option = options;

        while (option->var)
        {
            /* we can't pass option flags through properties */

            sprintf(propname, "%s%s", prop_prefix, option->long_opt);

            res = property_get(propname, propvalue, "");
            if (res)
            {
                if (strcmp(propvalue,"none"))
                {
                    ALOGI("%s: Getting property (%s=%s) from environment\n", __FUNCTION__, propname, propvalue);

                    switch (option->type)
                    {
                    case AGPS_OPTION_INT:
                    case AGPS_OPTION_FLAG:
                        int_option = (int*)option->var;
                        *int_option = atoi(propvalue);
                        break;
                    case AGPS_OPTION_STRING:
                        string_option = (char**)option->var;
                        /* free memory if it has already been allocated when parsing command line flags */
                        if (*string_option != NULL)
                        {
                            free(*string_option);
                        }
                        *string_option = malloc( strlen(propvalue)+1 );
                        if (*string_option == NULL)
                        {
                            ret = -AGPS_ERROR_OUT_OF_MEMORY;
                            break;
                        }
                        strcpy(*string_option, propvalue);
                        break;
                    }
                }

                /* break now if there was an error */
                if (ret != AGPS_OK)
                {
                    break;
                }
            }
            /* move on to next option */
            option++;
        }

    } while (0);

    free(posix_options);

    return ret;
}

void agps_PortCleanupArgs(agps_PortOption *options)
{
    agps_PortOption *option;
    char **string_option;

    option = options;

    while (option->var)
    {
        if (option->type == AGPS_OPTION_STRING)
        {
            string_option = (char**)option->var;
            /* free memory */
            if (*string_option != NULL)
            {
                free(*string_option);
            }
        }
        option++;
    }
}

