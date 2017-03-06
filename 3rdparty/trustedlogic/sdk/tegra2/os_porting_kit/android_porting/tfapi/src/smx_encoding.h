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
#ifndef __SMX_ENCODING_H__
#define __SMX_ENCODING_H__


#include "smx_defs.h"

/**
 * Initializes a new encoder attached to the specified device.
 *
 * @param pOperation
 *
 * @param nInitialCapacity The initial capacity, in bytes, of the encoder. If
 * set to zero, the default initial capacity will be used.
 *
 * @return {SM_SUCCESS} upon successful completion, or an appropriate error code
 * upon failure.
 */
SM_ERROR SMXEncoderInit(
      SM_OPERATION* pOperation,
      uint32_t nInitialCapacity);


/**
 * Uninitializes the specified encoder.
 *
 * Upon both successful completion and failure, the encoder has been destroyed
 * and cannot be used anymore.
 *
 * This function does nothing and returns {SM_SUCCESS} if <tt>pEncoder</tt> is
 * set to NULL.
 *
 * @param pEncoder A pointer to the encoder (NULL allowed).
 *
 * @return {SM_SUCCESS} upon successful completion, or an appropriate error code
 * upon failure.
 */
SM_ERROR SMXEncoderUninit(
      SMX_ENCODER* pEncoder);


/**
 * Sets the error state of the specified encoder.
 *
 * This function has no effect if the encoder's error state has already been
 * set.
 *
 * @param pEncoder A pointer to the encoder.
 *
 * @param nError The error code describing the encoder's error state.
 */
void SMXEncoderSetError(
      SMX_ENCODER* pEncoder,
      SM_ERROR nError);

/**
 * Retrieves the current error state of the specified encoder.
 *
 * @param pEncoder A pointer to the encoder.
 *
 * @return {SM_SUCCESS} if the encoder's error state is not set, or the error
 * code describing the encoder's error state otherwise.
 */
SM_ERROR SMXEncoderGetError(
      SMX_ENCODER* pEncoder);

/**
 * Creates a new decoder for the specified encoded data, attached to the
 * specified device.
 *
 * The encoded data must not be modified until the decoder has been destroyed.
 *
 * @param pDevice A pointer to the device the decoder will be attached to.
 *
 * @param pEncodedData A pointer to the first byte of the encoded data.
 *
 * @param nEncodedDataSize The size, in bytes, of the encoded data.
 *
 * @param ppDecoder A pointer to the placeholder to be set to the address of the
 * new decoder. This placeholder is set to NULL upon failure.
 *
 * @param phDecoder A pointer to the placeholder to be set to the handle of the
 * new decoder. This placeholder is set to {SM_HANDLE_INVALID} upon failure.
 *
 * @param pnDecoderReference A pointer to be set to the reference of the new
 * decoder. This placeholder is set to zero upon failure.
 *
 * @return {SM_SUCCESS} upon successful completion, or an appropriate error code
 * upon failure.
 */
SM_ERROR SMXDecoderCreate(
      SM_OPERATION* pOperation,
      const void *pEncodedData,
      uint32_t nEncodedDataSize,
      SMX_DECODER** ppDecoder);

void SMXDecoderAttach(
      SMX_DECODER* pParentDecoder,
      SMX_DECODER* pChildDecoder);

/**
 * Destroys the specified decoder.
 *
 * Upon both successful completion and failure, the decoder has been destroyed
 * and cannot be used anymore.
 *
 * This function does nothing and returns {SM_SUCCESS} if <tt>pDecoder</tt> is
 * set to NULL.
 *
 * @param pDecoder The address of the decoder (NULL allowed).
 *
 * @param bMustReleaseDecoder {true} if the decoder must be freed.
 *
 * @return {SM_SUCCESS} upon successful completion, or an appropriate error code
 * upon failure.
 */
SM_ERROR SMXDecoderDestroy(
      SMX_DECODER* pDecoder,
      bool bMustReleaseDecoder);

/**
 * Retrieves the current error state of the specified decoder.
 *
 * @param pDecoder A pointer to the decoder.
 *
 * @return {SM_SUCCESS} if the decoder's error state is not set, or the error
 * code describing the decoder's error state otherwise.
 */
SM_ERROR SMXDecoderGetError(
      SMX_DECODER* pDecoder);

/**
 * Sets the error state of the specified decoder.
 *
 * This function has no effect if the decoder's error state has already been
 * set.
 *
 * @param pDecoder A pointer to the decoder.
 *
 * @param nError The error code describing the decoder's error state.
 */
void SMXDecoderSetError(
      SMX_DECODER* pDecoder,
      SM_ERROR nError);

/**
 * Reads the property array at the current offset in the encoded data parsed by
 * the specified decoder.
 *
 * Upon return, the current offset of the decoder references the first item
 * following the decoded item.
 *
 * If the decoder error state is set upon entry, the function directly returns
 * NULL and does nothing more.
 *
 * This function allocates the returned property array. Once the array is no
 * longer needed, the client must free it using the function {SChannelFreeMem}.
 *
 * @param pDecoder A pointer to the decoder.
 *
 * @param pnPropertyCount A pointer to the placeholder to be set to the number
 * of entries in the returned property array. This placeholder is set to zero
 * upon failure.
 *
 * @return A pointer to the first entry of the decoded property array, or NULL
 * if the array is empty and upon failure.
 */
SM_PROPERTY * SMXDecoderReadProperties(
      SMX_DECODER* pDecoder,
      uint32_t *pnPropertyCount);


SMX_DECODER* SMXDecoderReadSequence(
      SMX_DECODER* pDecoder);

#endif  /* !defined(__SMX_ENCODING_H__) */
