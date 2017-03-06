/*
 * Copyright (c) 2001-2010 Trusted Logic S.A.
 * All Rights Reserved.
 *
 * This software is the confidential and proprietary information of
 * Trusted Logic S.A. ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with Trusted Logic S.A.
 *
 * TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
 * NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
#ifndef __S_ERROR_H__
#define __S_ERROR_H__

#define S_SUCCESS                      ((S_RESULT)0x00000000)
#define SM_SUCCESS                     S_SUCCESS
#define TEEC_SUCCESS                   S_SUCCESS
#define SST_SUCCESS                    S_SUCCESS

/**
 * Generic error code : Generic error
 **/
#define S_ERROR_GENERIC                ((S_RESULT)0xFFFF0000)
#define SM_ERROR_GENERIC               S_ERROR_GENERIC
#define TEEC_ERROR_GENERIC             S_ERROR_GENERIC
#define SST_ERROR_GENERIC              S_ERROR_GENERIC

/**
 * Generic error code : The underlying security system denies the access to the
 * object
 **/
#define S_ERROR_ACCESS_DENIED          ((S_RESULT)0xFFFF0001)
#define SM_ERROR_ACCESS_DENIED         S_ERROR_ACCESS_DENIED
#define TEEC_ERROR_ACCESS_DENIED       S_ERROR_ACCESS_DENIED
#define SST_ERROR_ACCESS_DENIED        S_ERROR_ACCESS_DENIED

/**
 * Generic error code : The pending operation is cancelled.
 **/
#define S_ERROR_CANCEL                 ((S_RESULT)0xFFFF0002)
#define SM_ERROR_CANCEL                S_ERROR_CANCEL
#define TEEC_ERROR_CANCEL              S_ERROR_CANCEL

/**
 * Generic error code : The underlying system detects a conflict
 **/
#define S_ERROR_ACCESS_CONFLICT        ((S_RESULT)0xFFFF0003)
#define SM_ERROR_EXCLUSIVE_ACCESS      S_ERROR_ACCESS_CONFLICT
#define TEEC_ERROR_ACCESS_CONFLICT     S_ERROR_ACCESS_CONFLICT
#define SST_ERROR_ACCESS_CONFLICT      S_ERROR_ACCESS_CONFLICT

/**
 * Generic error code : Too much data for the operation or some data remain
 * unprocessed by the operation.
 **/
#define S_ERROR_EXCESS_DATA            ((S_RESULT)0xFFFF0004)
#define SM_ERROR_EXCESS_DATA           S_ERROR_EXCESS_DATA
#define TEEC_ERROR_EXCESS_DATA         S_ERROR_EXCESS_DATA


/**
 * Generic error code : Error of data format
 **/
#define S_ERROR_BAD_FORMAT             ((S_RESULT)0xFFFF0005)
#define SM_ERROR_FORMAT                S_ERROR_BAD_FORMAT
#define TEEC_ERROR_BAD_FORMAT          S_ERROR_BAD_FORMAT

/**
 * Generic error code : The specified parameters are invalid
 **/
#define S_ERROR_BAD_PARAMETERS         ((S_RESULT)0xFFFF0006)
#define SM_ERROR_ILLEGAL_ARGUMENT      S_ERROR_BAD_PARAMETERS
#define TEEC_ERROR_BAD_PARAMETERS      S_ERROR_BAD_PARAMETERS
#define SST_ERROR_BAD_PARAMETERS       S_ERROR_BAD_PARAMETERS


/**
 * Generic error code : Illegal state for the operation.
 **/
#define S_ERROR_BAD_STATE              ((S_RESULT)0xFFFF0007)
#define SM_ERROR_ILLEGAL_STATE         S_ERROR_BAD_STATE
#define TEEC_ERROR_BAD_STATE           S_ERROR_BAD_STATE

/**
 * Generic error code : The item is not found
 **/
#define S_ERROR_ITEM_NOT_FOUND         ((S_RESULT)0xFFFF0008)
#define SM_ERROR_ITEM_NOT_FOUND        S_ERROR_ITEM_NOT_FOUND
#define TEEC_ERROR_ITEM_NOT_FOUND      S_ERROR_ITEM_NOT_FOUND
#define SST_ERROR_ITEM_NOT_FOUND       S_ERROR_ITEM_NOT_FOUND

/**
 * Generic error code : The specified operation is not implemented
 **/
#define S_ERROR_NOT_IMPLEMENTED        ((S_RESULT)0xFFFF0009)
#define SM_ERROR_NOT_IMPLEMENTED       S_ERROR_NOT_IMPLEMENTED
#define TEEC_ERROR_NOT_IMPLEMENTED     S_ERROR_NOT_IMPLEMENTED

/**
 * Generic error code : The specified operation is not supported
 **/
#define S_ERROR_NOT_SUPPORTED          ((S_RESULT)0xFFFF000A)
#define SM_ERROR_NOT_SUPPORTED         S_ERROR_NOT_SUPPORTED
#define TEEC_ERROR_NOT_SUPPORTED       S_ERROR_NOT_SUPPORTED

/**
 * Generic error code : Insufficient data is available for the operation.
 **/
#define S_ERROR_NO_DATA                ((S_RESULT)0xFFFF000B)
#define SM_ERROR_NO_DATA               S_ERROR_NO_DATA
#define TEEC_ERROR_NO_DATA             S_ERROR_NO_DATA

/**
 * Generic error code : Not enough memory to perform the operation
 **/
#define S_ERROR_OUT_OF_MEMORY          ((S_RESULT)0xFFFF000C)
#define SM_ERROR_OUT_OF_MEMORY         S_ERROR_OUT_OF_MEMORY
#define TEEC_ERROR_OUT_OF_MEMORY       S_ERROR_OUT_OF_MEMORY
#define SST_ERROR_OUT_OF_MEMORY        S_ERROR_OUT_OF_MEMORY

/**
 * Generic error code : The service is currently unable to handle the request;
 * try later
 **/
#define S_ERROR_BUSY                   ((S_RESULT)0xFFFF000D)
#define SM_ERROR_BUSY                  S_ERROR_BUSY
#define TEEC_ERROR_BUSY                S_ERROR_BUSY

/**
 * Generic error code : security violation
 **/
#define S_ERROR_SECURITY               ((S_RESULT)0xFFFF000F)
#define SM_ERROR_SECURITY              S_ERROR_SECURITY
#define TEEC_ERROR_SECURITY            S_ERROR_SECURITY

/**
 * Generic error code : the buffer is too short
 **/
#define S_ERROR_SHORT_BUFFER           ((S_RESULT)0xFFFF0010)
#define SM_ERROR_SHORT_BUFFER          S_ERROR_SHORT_BUFFER
#define TEEC_ERROR_SHORT_BUFFER        S_ERROR_SHORT_BUFFER


/**
 * Generic error code : SControl Asynchronous Operations are not supported.
 */
#define S_ERROR_ASYNC_OPERATIONS_NOT_SUPPORTED ((S_RESULT)0xFFFF0011)
#define SM_ERROR_ASYNC_OPERATIONS_NOT_SUPPORTED  S_ERROR_ASYNC_OPERATIONS_NOT_SUPPORTED

/**
 * Generic error code : the number of handles currently created
 * for a specific resource has reached the maximum amount.
 **/
#define S_ERROR_NO_MORE_HANDLES           ((S_RESULT)0xFFFF0013)

/**
 * Generic error code : the monotonic counter is corrupted
 **/
#define S_ERROR_CORRUPTED           ((S_RESULT)0xFFFF0014)

/**
 * Generic error code : the operation is not terminated
 **/
#define S_PENDING                      ((S_RESULT)0xFFFF2000)

/**
 * Generic error code : A timeout occurred
 **/
#define S_ERROR_TIMEOUT                ((S_RESULT)0xFFFF3001)

/**
 * Error code: Error of the underlying OS.
 **/
#define S_ERROR_UNDERLYING_OS          ((S_RESULT)0xFFFF3002)
#define TEEC_ERROR_OS                  S_ERROR_UNDERLYING_OS


/**
 * Error code: The operation is cancelled by a signal.
 **/
#define S_ERROR_CANCELLED_BY_SIGNAL    ((S_RESULT)0xFFFF3003)

/**
 * Generic error code : Overflow
 **/
#define S_ERROR_OVERFLOW               ((S_RESULT)0xFFFF300F)
#define SST_ERROR_OVERFLOW             S_ERROR_OVERFLOW

/**
 * Generic error code : The item already exists
 **/
#define S_ERROR_ITEM_EXISTS            ((S_RESULT)0xFFFF3012)

/**
 * Generic error code : The application reported an error.  The code of the
 * applicative error is encoded in the message data.
 */
#define S_ERROR_SERVICE                ((S_RESULT)0xFFFF1000)
#define SM_ERROR_SERVICE               S_ERROR_SERVICE

#define S_PENDING                      ((S_RESULT)0xFFFF2000)
#define SM_PENDING                     S_PENDING

/**
 * Generic error code : Critical error causing the platform to shutdown.
 */
#define S_ERROR_CRITICAL                ((S_RESULT)0xFFFF3010)

/**
 * Generic error code : the underlying peripheral is unreachable.
 */
#define S_ERROR_UNREACHABLE             ((S_RESULT)0xFFFF3013)

/*------------------------------------------------------------------------------
   Communication Error Codes
------------------------------------------------------------------------------*/
/**
 * Generic communication error
 **/
#define S_ERROR_COMMUNICATION          ((S_RESULT)0xFFFF000E)
#define SM_ERROR_COMMUNICATION         S_ERROR_COMMUNICATION
#define TEEC_ERROR_COMMUNICATION       S_ERROR_COMMUNICATION

/**
 * Error of communication : Error of protocol
 **/
#define S_ERROR_CONNECTION_PROTOCOL    ((S_RESULT)0xFFFF3020)

/**
 * Error of communication : The connection is broken.
 **/
#define S_ERROR_CONNECTION_BROKEN      ((S_RESULT)0xFFFF3021)

/**
 * Error of communication : Error during the connection setup.
 **/
#define S_ERROR_CONNECTION_SETUP       ((S_RESULT)0xFFFF3022)

/**
 * Error of communication : The connection is refused by the distant target.
 **/
#define S_ERROR_CONNECTION_REFUSED     ((S_RESULT)0xFFFF3023)

/**
 * Error of communication: The target of the connection is dead
 **/
#define S_ERROR_TARGET_DEAD            ((S_RESULT)0xFFFF3024)
#define SM_ERROR_TARGET_DEAD           S_ERROR_TARGET_DEAD
#define TEEC_ERROR_TARGET_DEAD         S_ERROR_TARGET_DEAD


/*------------------------------------------------------------------------------
   Storage Error Codes
------------------------------------------------------------------------------*/

/** File system error code: not enough space to complete the operation. */
#define S_ERROR_STORAGE_NO_SPACE       ((S_RESULT)0xFFFF3041)
#define SST_ERROR_NO_SPACE          0xFFFF3041

/**
 * File system error code: The file system is corrupted.
 */
#define S_ERROR_STORAGE_CORRUPTED      ((S_RESULT)0xFFFF3045)
#define SST_ERROR_CORRUPTED            S_ERROR_STORAGE_CORRUPTED

/**
 * File system error code: The file system is unreachable.
 */
#define S_ERROR_STORAGE_UNREACHABLE    ((S_RESULT)0xFFFF3046)

/*------------------------------------------------------------------------------
   Authentication / X509 error codes
------------------------------------------------------------------------------*/
#define S_ERROR_AUTHENTICATION_FAILED  ((S_RESULT)0xFFFF3060)
#define S_ERROR_WRONG_SIGNATURE        ((S_RESULT)0xFFFF3061)
#define S_ERROR_BAD_CERTIFICATE        ((S_RESULT)0xFFFF3062)
#define S_ERROR_WRONG_ISSUER           ((S_RESULT)0xFFFF3063)
#define S_ERROR_CERTIFICATE_EXPIRED    ((S_RESULT)0xFFFF3064)

/*------------------------------------------------------------------------------
   Crypto error codes
------------------------------------------------------------------------------*/
#define S_ERROR_BAD_KEY                ((S_RESULT)0xFFFF3070)

/*------------------------------------------------------------------------------
   Indicates the physical memory is in TCM
------------------------------------------------------------------------------*/
#define S_ERROR_ARM_MEMORY_IS_TCM      ((S_RESULT)0xFFFF3100)

/*------------------------------------------------------------------------------
   VM-specific Error Codes
------------------------------------------------------------------------------*/
#define S_ERROR_UNCAUGHT_EXCEPTION     ((S_RESULT)0xFFFF3080)
#define S_ERROR_TRUSTED_INTERPRETER    ((S_RESULT)0xFFFF3081)


/*------------------------------------------------------------------------------
   Range [0xFFFF3200:0xFFFF35FF] is reserved for internal use
------------------------------------------------------------------------------*/

#endif /* __S_ERROR_H__ */


