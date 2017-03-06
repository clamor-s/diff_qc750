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

#ifndef __OTF_SECURE_PROTOCOL_H__
#define __OTF_SECURE_PROTOCOL_H__


/** OTF Secure Driver UUID
 *  {AE0E814B-E980-423B-8BAE-D2FD63024E59}
 */
#define SERVICE_OTF_UUID   { 0xAE0E814B, 0xE980, 0x423B, {0x8B, 0xAE, 0xD2, 0xFD, 0x63, 0x02, 0x4E, 0x59 } }

/* ----- Service APIs ------ */
/**
 * OpenClientSession API:
 * No input parameter is required
 *
 * This turns on hardware resources and initialize internal structures.
 *
 * If the HDCP driver is available (UUID: {529B125E-4E58-11E0-823B-062BDFD72085}) a session between the
 * OTF Secure Driver and the HDCP Secure Driver is also opened.
 *
 * This function always return S_SUCCESS
 */


/**
 * CloseClientSession API:
 * No input parameter is required
 *
 * Clean internal resources:
 * - if a key has been set, the key is erased,
 * - if Secure HDCP driver is available, the session between the OTF driver and the HDCP driver is closed.
 */


/**
 * InvokeCommand API:
 *
 * Command OTF_SET_AES_KEY:
 *
 * @param buff (type S_PARAM_TYPE_MEMREF_INPUT), the key that is programmed in the OTF engine.
 *
 * @return S_ERROR_BAD_PARAMETERS, S_ERROR_OUT_OF_MEMORY, S_SUCCESS
 *
 * This command installs the key in the OTF engine. If the Secure HDCP Driver is available, this commands also
 * starts the HDCP link monitoring.
 *
 */
/* Commands ID */
#define OTF_SET_AES_KEY        1


#endif /* __OTF_SECURE_PROTOCOL_H__ */
