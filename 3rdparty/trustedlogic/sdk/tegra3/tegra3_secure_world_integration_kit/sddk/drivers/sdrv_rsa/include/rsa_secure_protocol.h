/**
 * Copyright (c) 2010-2011 Trusted Logic S.A.
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

#ifndef __RSA_SECURE_PROTOCOL_H__
#define __RSA_SECURE_PROTOCOL_H__


/** RSA Secure Driver UUID
 *  {E079E630-6708-11E1-B86C-0800200C9A66}
 */
#define SERVICE_RSA_UUID   { 0xE079E630, 0x6708, 0x11E1, {0xB8, 0x6C, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66 } }

/* ----- Service APIs ------ */
/**
 * OpenClientSession API:
 * No input parameter is required
 *
 * This turns on hardware resources and initialize internal structures.
 *
  * This function always return S_SUCCESS
 */


/**
 * CloseClientSession API:
 * No input parameter is required
 *
 * Clean internal resources:
 * - if a key has been set, the key is erased,
 */


/**
 * InvokeCommand API:
 *
 * Command RSA_SET_AES_KEY:
 *
 * @param buff (type S_PARAM_TYPE_MEMREF_INPUT), the key that is programmed in the OTF engine.
 *
 * @return S_ERROR_BAD_PARAMETERS, S_ERROR_OUT_OF_MEMORY, S_SUCCESS
 *
 * This command installs the RSA encrypted key in the OTF engine.
 *
 */
/* Commands ID */
#define RSA_SET_AES_KEY        1


#endif /* __RSA_SECURE_PROTOCOL_H__ */
