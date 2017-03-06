/* Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved. */
/*
** Copyright 2010, Icera Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** You may not use this file except in compliance with the License.
** You may obtain a copy of the License at:
**
** http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** imitations under the License.
*/
/*
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/**
* @file ril_request_object.c implements the RIL Request data which is transferred between
* different thread contexts
*
* This file implements functions for copying RIL requests. Note that several RIL Requests
* only have pointers to data buffers, but don't contain the data itself. This makes it necessary
* to distinguish between RIL Requests and implement a special handling for several of them.
*/
/*************************************************************************************************
* Project header files. The first in this list should always be the public interface for this
* file. This ensures that the public interface header file compiles standalone.
************************************************************************************************/
#include "ril_request_object.h" /* this */

#include <icera-ril.h>
#include <telephony/ril.h> /* RIL_Token, RIL Requests, ... */

#define LOG_NDEBUG 0        /**< enable (==0)/disable(==1) log output for this file */
#define LOG_TAG "RIL_REQO"  /**< log tag for this file */
#include <utils/Log.h>      /* ALOGD, LOGE, LOGI */

/*************************************************************************************************
* Standard header files
************************************************************************************************/
#include <malloc.h>
#include <memory.h>

/*************************************************************************************************
* Private Macros
************************************************************************************************/
/* redefine function names as defined in ril_commands.h */
#define dispatchCallForward         ril_request_copy_call_forward
#define dispatchDial                ril_request_copy_dial
#define dispatchSIM_IO              ril_request_copy_sim_io
#define dispatchSmsWrite            ril_request_copy_sms_write
#define dispatchString              ril_request_copy_string
#define dispatchStrings             ril_request_copy_strings
#define dispatchRaw                 ril_request_copy_raw
#define dispatchInts                ril_request_copy_raw
#define dispatchVoid                ril_request_copy_void
#define dispatchGsmBrSmsCnf         ril_request_copy_gsm_br_sms_cnf
#define dispatchCdmaSms             ril_request_copy_dummy
#define dispatchCdmaSmsAck          ril_request_copy_dummy
#define dispatchCdmaBrSmsCnf        ril_request_copy_dummy
#define dispatchRilCdmaSmsWriteArgs ril_request_copy_dummy
#if (RIL_VERSION >= 6)
#define dispatchDataCall            ril_request_copy_strings
#define responseSetupDataCall       ril_request_response_dummy
#define dispatchCdmaSubscriptionSource  ril_request_copy_dummy
#define dispatchVoiceRadioTech      ril_request_copy_dummy
#endif

#define responseCallForwards        ril_request_response_dummy
#define responseCallList            ril_request_response_dummy
#define responseCellList            ril_request_response_dummy
#define responseContexts            ril_request_response_dummy
#define responseInts                ril_request_response_dummy
#define responseRaw                 ril_request_response_dummy
#define responseSIM_IO              ril_request_response_dummy
#define responseSMS                 ril_request_response_dummy
#define responseString              ril_request_response_dummy
#define responseStrings             ril_request_response_dummy
#define responseVoid                ril_request_response_dummy
#define responseSimStatus           ril_request_response_dummy
#define responseRilSignalStrength   ril_request_response_dummy
#define responseDataCallList        ril_request_response_dummy
#define responseGsmBrSmsCnf         ril_request_response_dummy
#define responseCdmaBrSmsCnf        ril_request_response_dummy

/*************************************************************************************************
* Private type definitions
************************************************************************************************/

/** common prototype for any RIL request copy function */
typedef void* (*ril_request_copy_func_t) ( void *data, size_t datalen );
/** common prototype for any RIL request free function */
typedef void  (*ril_request_free_func_t) ( void *data, size_t datalen );

/**
 * This type maps RIL request Ids to RIl request types. These types allow an earier handling
 * when copied RIl Requests containing pointers to data buffers instead of the data itself.
 */
typedef struct _ril_request_type_map_t
{
    int request;                          /**< RIL request ID */
    ril_request_copy_func_t dispatch;     /**< classification of RIL request */
    void  (*dummy_func) ( void );         /**< dummy function,
                                             just for compilation/linkage when using ril_commands.h*/
}
ril_request_type_map_t;

/*************************************************************************************************
* Private function declarations
************************************************************************************************/
static void *ril_request_copy_dummy ( void *data, size_t datalen );
static void *ril_request_copy_call_forward( void *data, size_t datalen );
static void *ril_request_copy_dial( void *data, size_t datalen );
static void *ril_request_copy_sim_io( void *data, size_t datalen );
static void *ril_request_copy_sms_write( void *data, size_t datalen );
static void *ril_request_copy_string( void *data, size_t datalen );
static void *ril_request_copy_strings( void *data, size_t datalen );
static void *ril_request_copy_raw( void *data, size_t datalen );
static void *ril_request_copy_void( void *data, size_t datalen );
static void *ril_request_copy_gsm_br_sms_cnf( void *data, size_t datalen );

static void ril_request_response_dummy( void );

/*************************************************************************************************
* Private variable definitions
************************************************************************************************/
/**
 * A table which allows the grouping of RIL Requests to make handling of RIL Request
 * classes more easy
 */
static ril_request_type_map_t  s_request_type_map [] =
{
/** include the mapping table and re-use it for our purposes */
#include "../libril/ril_commands.h"
};

/**
 * A table which allows the grouping of RIL Internal Requests to make handling of
 * RIL Internal Request classes more easy
 */
static ril_request_type_map_t  s_request_internal_type_map [] =
{
/** include the mapping table and re-use it for our purposes */
#include "ril_internal_commands.h"
};

/*************************************************************************************************
* Private function definitions
************************************************************************************************/
static void ril_request_response_dummy( void )
{
    return;
}

static void *ril_request_copy_dummy( void *data, size_t datalen )
{
    return 0;
}

static void *ril_request_copy_call_forward( void *data, size_t datalen )
{
    RIL_CallForwardInfo *ret = ril_request_copy_raw(data, datalen);

    if (ret->number)
    {
        ret->number = strdup(ret->number);
    }

    return ret;
}

static void ril_request_free_call_forward( void *data, size_t datalen )
{
    RIL_CallForwardInfo *cff = data;

    if (cff->number)
    {
        free(cff->number);
    }

    free(cff);
}

static void *ril_request_copy_dial( void *data, size_t datalen )
{
    RIL_Dial *ret = ril_request_copy_raw(data, datalen);

    if (ret->address)
    {
        ret->address = strdup(ret->address);
    }

    if ( ret->uusInfo )
    {
      /* TODO: copy UUSD information */
    }

    return ret;
}

static void ril_request_free_dial( void *data, size_t datalen )
{
    RIL_Dial *d = data;

    if (d->address)
    {
        free(d->address);
    }

    free(d);
}

static void *ril_request_copy_sim_io( void *data, size_t datalen )
{
    RIL_SIM_IO *ret = ril_request_copy_raw(data, datalen);

    if (ret->path)
    {
        ret->path = strdup(ret->path);
    }

    if (ret->data)
    {
        ret->data = strdup(ret->data);
    }

    if (ret->pin2)
    {
        ret->pin2 = strdup(ret->pin2);
    }

#if (RIL_VERSION >= 6)
    if (ret->aidPtr)
    {
        ret->aidPtr = strdup(ret->aidPtr);
    }
#endif

    return ret;
}

static void ril_request_free_sim_io( void *data, size_t datalen )
{
    RIL_SIM_IO *sio = data;

    if (sio->path)
    {
        free(sio->path);
    }

    if (sio->data)
    {
        free(sio->data);
    }

    if (sio->pin2)
    {
        free(sio->pin2);
    }

    free(sio);
}

static void *ril_request_copy_sms_write( void *data, size_t datalen )
{
    RIL_SMS_WriteArgs *ret = ril_request_copy_raw(data, datalen);

    if (ret->pdu)
    {
        ret->pdu = strdup(ret->pdu);
    }

    if (ret->smsc)
    {
        ret->smsc = strdup(ret->smsc);
    }

    return ret;
}

static void ril_request_free_sms_write( void *data, size_t datalen )
{
    RIL_SMS_WriteArgs *args = data;

    if (args->pdu)
    {
        free(args->pdu);
    }

    if (args->smsc)
    {
        free(args->smsc);
    }

    free(args);
}

static void *ril_request_copy_strings( void *data, size_t datalen )
{
    char **a = (char **)data;
    char **ret;
    int strCount = datalen / sizeof(char *);
    int i;

    ret = malloc(strCount * sizeof(char *));
    memset(ret, 0, sizeof(char *) * strCount);

    for (i = 0; i < strCount; i++)
    {
        if (a[i])
        {
            ret[i] = strdup(a[i]);
        }
    }

    return (void *) ret;
}

static void ril_request_free_strings( void *data, size_t datalen )
{
    int count = datalen / sizeof(char *);
    int i;

    for (i = 0; i < count; i++)
    {
        if (((char **) data)[i])
        {
            free(((char **) data)[i]);
        }
    }

    free(data);
}

static void *ril_request_copy_gsm_br_sms_cnf( void *data, size_t datalen )
{
    RIL_GSM_BroadcastSmsConfigInfo **a =
            (RIL_GSM_BroadcastSmsConfigInfo **) data;
    int count;
    void **ret;
    int i;

    count = datalen / sizeof(RIL_GSM_BroadcastSmsConfigInfo *);

    ret = malloc(count * sizeof(RIL_GSM_BroadcastSmsConfigInfo *));
    memset(ret, 0, sizeof(*ret));

    for (i = 0; i < count; i++)
    {
        if (a[i])
        {
            ret[i] = ril_request_copy_raw(a[i], sizeof(RIL_GSM_BroadcastSmsConfigInfo));
        }
    }

    return ret;
}

static void ril_request_free_gsm_br_sms_cnf( void *data, size_t datalen )
{
    int count = datalen / sizeof(RIL_GSM_BroadcastSmsConfigInfo);
    int i;

    for (i = 0; i < count; i++)
    {
        if (((RIL_GSM_BroadcastSmsConfigInfo **) data)[i])
        {
            free(((RIL_GSM_BroadcastSmsConfigInfo **) data)[i]);
        }
    }

    free(data);
}

static void *ril_request_copy_string( void *data, size_t datalen )
{
    if (data)
    {
        return strdup((char *) data);
    }
    return NULL;
}

static void *ril_request_copy_raw( void *data, size_t datalen )
{
    void *ret = malloc(datalen);
    if ( ret )
    {
        memcpy(ret, data, datalen);
    }
    return ret;
}

static void ril_request_free_raw( void *data, size_t datalen )
{
   free(data);
}

static void *ril_request_copy_void( void *data, size_t datalen )
{
  return NULL;
}

#define RIL_REQUEST_MAX    (int)(sizeof(s_request_type_map)/sizeof(ril_request_type_map_t))
#define IRIL_REQUEST_MAX    (int)(sizeof(s_request_internal_type_map)/sizeof(ril_request_type_map_t))

static ril_request_copy_func_t ril_request_get_copy_func( int request )
{
    if (request < RIL_REQUEST_MAX) {
        return s_request_type_map[request].dispatch;
    }
    if ((request >= IRIL_REQUEST_ID_BASE) && (request < IRIL_REQUEST_ID_BASE+IRIL_REQUEST_MAX)){
        return s_request_internal_type_map[request - IRIL_REQUEST_ID_BASE].dispatch;
    }
    return NULL;
}

static ril_request_free_func_t ril_request_get_free_func( int request )
{
    ril_request_copy_func_t copy_func = ril_request_get_copy_func(request);
    ril_request_free_func_t free_func = NULL;

    if (copy_func == dispatchInts  ||
        copy_func == dispatchRaw   ||
        copy_func == dispatchString)
    {
        free_func = ril_request_free_raw;
    }
    else if (copy_func == dispatchStrings)
    {
        free_func = ril_request_free_strings;
    }
    else if (copy_func == dispatchSIM_IO)
    {
        free_func = ril_request_free_sim_io;
    }
    else if (copy_func == dispatchDial)
    {
        free_func = ril_request_free_dial;
    }
    else if (copy_func == dispatchCallForward)
    {
        free_func = ril_request_free_call_forward;
    }
    else if (copy_func == dispatchSmsWrite)
    {
        free_func = ril_request_free_sms_write;
    }
    else if (copy_func == dispatchGsmBrSmsCnf)
    {
        free_func = ril_request_free_gsm_br_sms_cnf;
    }
    else
    {
        /* free_func = NULL, not needed as pre-initialized with NULL */
        /* if (copy_func == dispatchVoid) is covered here also */
    }

    return free_func;
}

static void* ril_request_copy_request_data( int request, void* data, size_t datalen )
{
    ril_request_copy_func_t copy_func = ril_request_get_copy_func( request );

    if ( NULL != data && NULL != copy_func )
    {
        return copy_func( data, datalen );
    }
    return NULL;
}

static void ril_request_free_request_data( int request, void* data, size_t datalen )
{
    ril_request_free_func_t free_func = ril_request_get_free_func( request );

    if ( NULL != data && NULL != free_func )
    {
        free_func( data, datalen );
    }
}

/*************************************************************************************************
* Public function definitions
************************************************************************************************/
struct _ril_request_object_t* ril_request_object_create( int request, void *data, size_t datalen, void* ril_token )
{
    ril_request_object_t* req_obj = (ril_request_object_t*) malloc (sizeof(ril_request_object_t));

    if ( req_obj )
    {
        req_obj->request = request;
        req_obj->data    = ril_request_copy_request_data( request, data, datalen );
        req_obj->datalen = datalen;
        req_obj->token   = ril_token;
    }

    return req_obj;
}

void ril_request_object_destroy( struct _ril_request_object_t* req_obj )
{
    ril_request_free_request_data( req_obj->request, req_obj->data, req_obj->datalen );

    free( req_obj );
}
