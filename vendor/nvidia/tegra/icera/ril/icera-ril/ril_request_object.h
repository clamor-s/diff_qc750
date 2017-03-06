/* Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved. */
/*
** Copyright 2010, Icera Inc.
*/

/**
* @{
*/

/**
* @file ril_request_object.h The interface of the RIL-Request object which are exchanged via
*  the RIL request queue to realize the non-blocking onrequest functionality
*
*/

#ifndef __RIL_REQUEST_OBJECT_H_
#define __RIL_REQUEST_OBJECT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*************************************************************************************************
* Standard header files
************************************************************************************************/
#include <stddef.h>  /* size_t */

/*************************************************************************************************
* Public Types
************************************************************************************************/

/**
 * The request object whoch stores all parameters which are passed to a onRequest()
 * function call. Note that the token field has type void* instead of RIL_Token to
 * avoid the include of ril.h here. However, RIL_token is a void* too.
 */
typedef struct _ril_request_object_t
{
    int    request;   /**< RIL request ID as defined in ril.h */
    void*  data;      /**< RIL request data structure belonging to the RIL request */
    size_t datalen;   /**< size of RIL request data structure */
    void*  token;     /**< RIL_token parameter */
}
ril_request_object_t;

/*************************************************************************************************
* Public function declarations
************************************************************************************************/
/**
 * Create and initialize a RIL Request Object
 *
 * This function allocates memory for the ril_request_object_t and copies the
 * content of the calling parameter to the new data buffer, so that the input
 * parameters can be freed directly after the function returned.
 * Note: Several RIL request data structures contain only points to some
 * external buffers, like dial strings etc. A specific handling for all
 * requests is covered by this function.
 *
 * @param  request [IN] RIL request ID
 * @param  data    [IN] RIL request data buffer, might be NULL if not needed
 * @param  datalen [IN] size of data buffer
 * @param  token   [IN] pointer to RIL_token buffer
 *
 * @return returns a new ril_request_object if successfull, NULL otherwise
 *
 * @sa ril_request_object_destroy,  ril_request_object_t
 */
struct _ril_request_object_t* ril_request_object_create( int request, void *data, size_t datalen, void* token );

/**
 * Destroy a RIL Request Object
 *
 * This function frees all resources of the given ril_request_object
 * Note: If the RIL request data structure contains pointers which have been
 * allocated by the ril_request_object_create function, these are freed by
 * this destroy function also.
 *
 * @param  ril_req_obj [IN] RIL Request Object to be freed
 *
 * @return void
 *
 * @sa ril_request_object_create,  ril_request_object_t
 */
void ril_request_object_destroy( struct _ril_request_object_t* ril_req_obj );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RIL_REQUEST_OBJECT_H_ */

/** @} END OF FILE */
