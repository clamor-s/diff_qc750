/**
 * Copyright (c) 2006-2009 Trusted Logic S.A.
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
 
#ifndef __DRM_PROTOCOL_H__
#define __DRM_PROTOCOL_H__

/* This example depends on the Advanced profile of the Trusted Foundations */

/**
 * The DRM Agent service is a mono-instance, multi-session, mono-command service.
 * The protocol associated with this service is outlined in the rest of this header file.
 */

/** Service Identifier */
/* {13f616f9-8572-4a6f-a1f1-03aa9b05f9ee} */
#define SERVICE_DRM_UUID   { 0x13F616F9, 0x8572, 0x4A6F, { 0xA1, 0xF1, 0x03, 0xAA, 0x9B, 0x05, 0xF9, 0xEE } }

/* {7773744F-3CED-4a97-AFDC-A39758C45D2D} */
#define STIPLET_DRM_UUID   { 0x7773744F, 0x3CED, 0x4a97, { 0xAF, 0xDC, 0xA3, 0x97, 0x58, 0xC4, 0x5D, 0x2D } }

/**
 * OpenSession.
 *
 * The OpenSession requires an integer of type Uint32 which specifies the session type.
 * The DRM_AGENT_MANAGEMENT_SESSION and DRM_AGENT_DECRYPT_SESSION constants indicates
 * the session type. For a decryption session, a encrypted key is also passed.
 *
 * Parameters:
 * 0: VALUE_INPUT: a=session type
 * 1: NONE for a management session; MEMREF_INPUT for a decryption session
 */
/** Constant for indicating a management session */
#define DRM_AGENT_MANAGEMENT_SESSION         0x00000001
/** Constant for indicating a decryption session */
#define DRM_AGENT_DECRYPT_SESSION            0x00000002

/* Command Parameters */

/**
 * Decrypt Command. Decrypts a block of data.
 *
 * Parameters:
 * 0: MEMREF_INPUT: the encrypted buffer
 * 1: MEMREF_OUTPUT: the decrypted buffer
 *
 * Actual output size is returned in parameter 1. 
 *
 * The error DRM_AGENT_ERROR_BUFFER_TOO_SMALL is returned when the 
 * the output buffer is too small.
 */
#define DRM_AGENT_CMD_DECRYPT                0x00000001

/**
 * Init Command. Install the root key.
 *
 * No parameters
 */
#define DRM_AGENT_CMD_INIT                   0x00000002


/**
 * Clean Command. Clean up the key store.
 *
 * No parameters
 */
#define DRM_AGENT_CMD_CLEAN                   0x00000003

/** Service-specific error codes */
#define DRM_AGENT_ERROR_CRYPTO               0x00000001
#define DRM_AGENT_ERROR_ALREADY_INITIALIZED  0x00000002
/* Note that the value of DRM_AGENT_ERROR_BUFFER_TOO_SMALL must match CKR_BUFFER_TOO_SMALL
   because that is the error code returned by a Cryptoki Update Shortcut */
#define DRM_AGENT_ERROR_BUFFER_TOO_SMALL     0x00000150 

#endif /* __DRM_PROTOCOL_H__ */
