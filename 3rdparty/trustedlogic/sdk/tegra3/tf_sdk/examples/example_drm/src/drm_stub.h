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
 
/* This example depends on the Advanced profile of the Trusted Foundations */

#ifndef __DRM_STUB_H__
#define __DRM_STUB_H__

/**
 * The DRM stub enables an application to decrypt a content block by block.
 *
 * The calling application must call drm_initialize prior to any use of the DRM stub.
 * It must call drm_finalize when the DRM stub is not used any more.
 *
 * The calling application must open sessions on the DRM Agent prior to perform an operation.
 * Two kind of sesisons are supported: management session and decyption session.
 *
 * Management session allows an application to instale the DRM Agent root key. This function must be
 * called once during the DRM Agent life-cycle. An application must install the DRM Agent root key
 * before being able to decrypt content.
 *
 * Decryption session enable an applicaiton to decrypt content by blocks. The session key used to decrypt
 * content is provided by the application when opening the decryption session.
 * The buffer used to perform the decryption is also specified when opening the decryption session.
 * This buffer may be allocated by the caller or by the implementation.
 * Decryption is performed in place, which means the same buffer is used by the caller to provide
 * encrypted data and by the implementation to store the decrypted result.
 * The length of encrypted data block put in the buffer by the caller must be a multiple
 * of DRM_BLOCK_SIZE. The caller is so responsible for padding data if required.
 * An application can only perform one decryption session at a time.
 *
 */


#include "s_type.h"

#define DRM_IMPORT_API S_DLL_EXPORT

/** Error Codes */
#define DRM_SUCCESS                          0x00000000
#define DRM_ERROR_OUT_OF_MEMORY              0x00000001
#define DRM_ERROR_ALREADY_INITIALIZED        0x00000002
#define DRM_ERROR_NOT_INITIALIZED            0x00000003
#define DRM_ERROR_ILLEGAL_ARGUMENT           0x00000004
#define DRM_ERROR_GENERIC                    0x00000005
#define DRM_ERROR_SESSION_NOT_OPEN           0x00000006
#define DRM_ERROR_SESSION_ALREADY_OPEN       0x00000007
#define DRM_ERROR_CRYPTO                     0x00000008
#define DRM_ERROR_KEY_ALREADY_INITIALIZED    0x00000009
#define DRM_ERROR_ILLEGAL_STATE              0x0000000A

/** DRM Session Handle type */
typedef uint32_t DRM_SESSION_HANDLE;

/** DRM ERROR type */
typedef uint32_t DRM_ERROR;

/** Sepcial value for invalid session handles */
#define DRM_SESSION_HANDLE_INVALID 0x00000000

/** The size of the block used by the DRM Agent.
 * Data buffers length must be a multiple of the block size.
 */
#define DRM_BLOCK_SIZE (uint32_t)16

/**
 * Initialize the DRM stub.
 * This function must be called once before any other function.
 *
 * @return DRM_SUCCESS upon success, an error code otherwise.
 */
DRM_ERROR DRM_IMPORT_API drm_initialize( void );

/**
 * Finalize the DRM stub.
 *
 * @return DRM_SUCCESS upon success, an error code otherwise.
 */
DRM_ERROR DRM_IMPORT_API drm_finalize( void );

/**
 * Open a management session.
 *
 * @param pSession  pointer to a session handle. Its value is set
 * to DRM_SESSION_HANDLE_INVALID upon error.
 *
 * @param bUseStiplet  true to use the STIPlet DRM agent.
 *
 * @return DRM_SUCCESS upon success, an error code otherwise.
 */
DRM_ERROR DRM_IMPORT_API drm_open_management_session ( DRM_SESSION_HANDLE* pSession, bool bUseStiplet );

/**
 * Close a management session.
 *
 * @param hSession  the session handle
 *
 * @return DRM_SUCCESS upon success, an error code otherwise.
 */
DRM_ERROR DRM_IMPORT_API drm_close_management_session ( DRM_SESSION_HANDLE hSession );

/**
 * Initialize the drm agent with the device key.
 * This function can be called only once.
 * When this function is called once the device key has already been
 * initialized, this function returns DRM_ERROR_KEY_ALREADY_INITIALIZED.
 *
 * @param hSession the session handle
 *
 * @return DRM_SUCCESS upon success, an error code otherwise.
 */
DRM_ERROR DRM_IMPORT_API drm_management_init( DRM_SESSION_HANDLE hSession );


/**
 * Clean up the drm agent key store. This removes the device key.
 *
 * @param hSession the session handle
 *
 * @return DRM_SUCCESS upon success, an error code otherwise.
 */
DRM_ERROR DRM_IMPORT_API drm_management_clean( DRM_SESSION_HANDLE hSession );

/**
 * Open a decryption session.
 *
 * The caller must provide the session key encrypted with the
 * device public key.
 *
 * The caller may also provide the memory block used to perform the decryption.
 * If the caller doesn't provide the memory block (i.e. pBlock is NULL),
 * the implementation allocates the memory block. Decryption is performed
 * in place, which means the same block is used by the caller to provide
 * encrypted data and by the implementation to store the decrypted result.
 *
 * @param  pEncryptedKey  a buffer containing the encrypted session key.
 *
 * @param nEncryptedKeyLen  the length of the encrypted buffer.
 *
 * @param ppBlock  pointer to the memory block used to perform the decryption. If
 * the memory block is NULL (*ppBlock == NULL), the implemntation is responsible for
 * allocating the block.
 *
 * @param nBlockLen  the size of the block.
 *
 * @param phSession  pointer on a variable set with session handle. Its value is set
 * to DRM_SESSION_HANDLE_INVALID upon error.
 *
 * @return DRM_SUCCESS upon success, an error code otherwise.
 */
DRM_ERROR DRM_IMPORT_API drm_open_decrypt_session (
                                    uint8_t* pEncryptedKey,
                                    uint32_t nEncryptedKeyLen,
                                    uint8_t** ppBlock,
                                    uint32_t nBlockLen,
                                    DRM_SESSION_HANDLE* phSession,
                                    bool bUseStiplet);

/**
 * Close a decryption session.
 *
 * @param hSession  the session handle
 *
 * @return DRM_SUCCESS upon success, an error code otherwise.
 */
DRM_ERROR DRM_IMPORT_API drm_close_decrypt_session ( DRM_SESSION_HANDLE hSession );

/**
 * Decrypts data from the specified offset to the specified length
 * in the block specified when opening the decryption session.
 *
 * Length must be a mulitple of DRM_BLOCK_SIZE.
 *
 * Decrypted data are starts at the same offset than the encrypted one.
 *
 * The decrypted data has the same length than the encrypted data.
 *
 * @param hSession  the session handle
 *
 * @param  nOffset  start offset in the block.
 *
 * @param nLength  the length of the encrypted data in the block. It
 * must be a multiple of DRM_BLOCK_SIZE.
 *
 * @return DRM_SUCCESS upon success, an error code otherwise.
 */
DRM_ERROR DRM_IMPORT_API drm_decrypt(
                      DRM_SESSION_HANDLE hSession,
                      uint32_t nOffset,
                      uint32_t nLength);

#endif /* __DRM_STUB_H__ */
