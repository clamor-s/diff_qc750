/*
 * Copyright (c) 2011 Trusted Logic S.A.
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

#ifndef __AES_CTS_INCLUDE_H__
#define __AES_CTS_INCLUDE_H__


CK_RV DecryptAES_CBC_CTS(CK_SESSION_HANDLE hCryptoSession, 
                              CK_OBJECT_HANDLE hObjectKey, 
                              IN OUT uint8_t* IV,
                              IN const uint8_t* pInputData,
                              uint32_t nInputDataLength,
                              OUT uint8_t* pOutputData,
                              OUT uint32_t* pnOutputDataLength);

#endif /* __AES_CTS_INCLUDE_H__ */
