/**
 * Copyright (c) 2011 NVIDIA Corporation
 * All Rights Reserved.
 *
 * This software is the confidential and proprietary information of
 * NVIDIA ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with NVIDIA.
 *
 * NVIDIA MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. NVIDIA SHALL
 * NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
 

#ifndef __NVSI_KEYBOX_H__
#define __NVSI_KEYBOX_H__

/**
 * Returns the NVSI keybox.
 *
 * This function should just return the clear (decrypted) keybox in format:
 *
 *  16B device ID
 *  32B AES key
 *  4B NVSI Magic const
 *
 * @param buf (out) - pointer to the buffer to hold the keybox
 * @param length (in/out) - on input, the allocated buffer size.
 *     On output, the number of bytes of the keybox.
 *
 * @retval S_SUCCESS success
 * @retval S_ERROR_SHORT_BUFFER if the buffer is too small to return the keybox
 * @retval Otherwise failed to return keybox
 */
S_RESULT oemcrypto_get_nvsi_keybox(/*OUT*/ uint8_t* buf, /*INOUT*/ uint32_t* length);

#endif // __NVSI_KEYBOX_H__
