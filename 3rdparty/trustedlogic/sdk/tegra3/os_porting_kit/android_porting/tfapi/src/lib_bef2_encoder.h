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

#ifndef __LIB_BEF_ENCODER_H__
#define __LIB_BEF_ENCODER_H__

#include "s_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LIB_BEF_ENCODER_MAX_SEQUENCE_DEPTH 16

/*
* Describes a BEF encoder instance.
*/
typedef struct LIB_BEF_ENCODER
{
   /*
    * A pointer to the byte array containing the encoded data.
    */
   uint8_t* pBuffer;
   /*
    * The current size, in bytes, of the encoded data.
    */
   uint32_t nSize;
   /*
    * The error code describing the encoder's error state, or S_SUCCESS if
    * the encoder's error state is not set.
    * S_ERROR_OUT_OF_MEMORY sequence stack too small
    * S_ERROR_SHORT_BUFFER buffer needs to be reallocated
    * S_ERROR_BAD_FORMAT otherwise
    */
   S_RESULT nError;
   /*
    * The current index in the offset stack.
    */
   uint32_t nOffsetStackIndex;
   /*
    * The current offset stack of this encoder. Each element indicates
    * the current offset in a sequence. When a sequence is open, the offset
    * of the parent decoder is set to the next element after the sequence.
    */
   uint32_t nOffsetStack[LIB_BEF_ENCODER_MAX_SEQUENCE_DEPTH];
   /*
    * The capacity, in bytes, of the byte array referenced by pBuffer.
    */
   uint32_t nCapacity;
   /*
    *
    * When nError is set to S_ERROR_SHORT_BUFFER, the size needed to encode properly.
    * Otherwise when SHORT_BUFFER is not set, nRequiredCapacity = nCapacity.
   */
   uint32_t nRequiredCapacity;
} LIB_BEF_ENCODER;

/*----------------------------------------------------------------------------
 * Encoder
 *----------------------------------------------------------------------------*/

/**
 * All the exported functions of this library take a pointer to a LIB_BEF_ENCODER
 * structure as a first parameter. This structure is internally used by the library
 * and its content is modified by the called functions.
 * Only pBuffer, nSize and nError are public to the caller and the other elements
 * of this structure must not be used outside the library.
 */

/**
 * Resets the specified Binary Encoding Format encoder instance.
 *
 * This function should be called once before using the encoder
 * instance.
 *
 * It can also be used for erasing the content of the encoder
 * and its error state.
 *
 * @param pEncoder The pointer to the encoder.
 *
 * @pre pEncoder is not NULL.
 * @pre pEncoder->pBuffer is correctly set
 * @pre pEncoder->nCapacity is correctly set
 *
 */
void libBEFEncoderInit(
      OUT LIB_BEF_ENCODER* pEncoder);

/**
 * Encodes an UUID in the BEF encoder.
 *
 * Upon error, this function sets the error state of the encoder.
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder The pointer of the BEF encoder.
 *
 * @param pIdentifier A pointer to the identifier to encode.
 *
 * @pre pEncoder is not NULL.
 **/
void libBEFEncoderWriteUUID(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      IN const S_UUID* pIdentifier);

/**
 * Appends the encoded representation of the specified boolean value to the
 * encoded data of the specified Binary Encoding Format encoder instance.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param bValue The value to encode.
 *
 * @pre pEncoder is not NULL.
 *
 */
void libBEFEncoderWriteBoolean(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      bool bValue);


/**
 * Appends the encoded representation of the specified uint8_t value to the encoded
 * data of the specified Binary Encoding Format encoder instance.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param nValue The value to encode.
 *
 * @pre pEncoder is not NULL.
 */
void libBEFEncoderWriteUint8(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      uint8_t nValue);


/**
 * Appends the encoded representation of the specified uint8_t array to the encoded
 * data the specified Binary Encoding Format encoder instance.
 *
 * If the <tt>pnArray</tt> parameter is set to NULL, a null uint8_t array value is
 * encoded.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param nArrayLength The number of elements in the array.  This parameter is
 * ignored if <tt>pnArray</tt> is NULL.
 *
 * @param pnArray A pointer to the array to encode, or NULL to encode a null
 * uint8_t array value.
 *
 * @pre pEncoder is not NULL.
 *
 */
void libBEFEncoderWriteUint8Array(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      uint32_t nArrayLength,
      IN const uint8_t* pnArray);


/**
 * Appends the encoded representation of the specified uint16_t value to the encoded
 * data of the specified Binary Encoding Format encoder instance.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param nValue The value to encode.
 *
 * @pre pEncoder is not NULL.
 */
void libBEFEncoderWriteUint16(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      uint16_t nValue);


/**
 * Appends the encoded representation of the specified uint16_t array to the encoded
 * data of the specified Binary Encoding Format encoder instance.
 *
 * If the <tt>pnArray</tt> parameter is set to NULL, a null uint16_t array value is
 * encoded.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param nArrayLength The number of elements in the array.  This parameter is
 * ignored if <tt>pnArray</tt> is NULL.
 *
 * @param pnArray A pointer to the array to encode, or NULL to encode a null
 * uint16_t array value.
 *
 * @pre pEncoder is not NULL.
 */
void libBEFEncoderWriteUint16Array(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      uint32_t nArrayLength,
      IN const uint16_t* pnArray);


/**
 * Appends the encoded representation of the specified uint32_t value to the encoded
 * data of the specified Binary Encoding Format encoder instance.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param nValue The value to encode.
 *
 * @pre pEncoder is not NULL.
 */
void libBEFEncoderWriteUint32(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      uint32_t nValue);

/**
 * Appends the encoded representation of the specified uint32_t array to the encoded
 * data of the specified Binary Encoding Format encoder instance.
 *
 * If the <tt>pnArray</tt> parameter is set to NULL, a null uint32_t array value is
 * encoded.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param nArrayLength The number of elements in the array.  This parameter is
 * ignored if <tt>pnArray</tt> is NULL.
 *
 * @param pnArray A pointer to the array to encode, or NULL to encode a null
 * uint32_t array value.
 *
 * @pre pEncoder is not NULL.
 */
void libBEFEncoderWriteUint32Array(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      uint32_t nArrayLength,
      IN const uint32_t* pnArray);

/**
 * Appends the encoded representation of the specified handle value to the encoded
 * data of the specified Binary Encoding Format encoder instance.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param nValue The value to encode.
 *
 * @pre pEncoder is not NULL.
 */
void libBEFEncoderWriteHandle(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      S_HANDLE nValue);


/**
 * Appends the encoded representation of the specified handle array to the encoded
 * data of the specified Binary Encoding Format encoder instance.
 *
 * If the <tt>pnArray</tt> parameter is set to NULL, a null uint32_t array value is
 * encoded.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param nArrayLength The number of elements in the array.  This parameter is
 * ignored if <tt>pnArray</tt> is NULL.
 *
 * @param pnArray A pointer to the array to encode, or NULL to encode a null
 * uint32_t array value.
 *
 * @pre pEncoder is not NULL.
 */
void libBEFEncoderWriteHandleArray(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      uint32_t nArrayLength,
      IN const S_HANDLE* pnArray);

/**
 * Appends the encoded data representation of a UTF-8 string
 * to the encoded data of the specified Binary Encoding Format encoder instance.
 *
 * If the <tt>pString</tt> parameter is set to NULL, a null string value is
 * encoded.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param pString A pointer to the UTF-8 string to encode, or NULL to encode a
 * null string value.
 *
 * @pre pEncoder is not NULL.
 */
void libBEFEncoderWriteString(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      IN const char* pString);

/**
 * Appends the encoded data representation of a wchar_t string
 * to the encoded data of the specified Binary Encoding Format
 * encoder instance.
 *
 * If the <tt>pString</tt> parameter is set to NULL, a null string value is
 * encoded.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param pString A pointer to the UTF-8 string to encode, or NULL to encode a
 * null string value.
 *
 * @pre pEncoder is not NULL.
 */
#ifdef WCHAR_SUPPORTED
void libBEFEncoderWriteWString(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      IN const wchar_t* pString);
#endif

/**
 * Appends the encoded representation of the specified memory reference
 * to the encoded data of the specified Binary Encoding Format
 * encoder instance.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if encoder buffer is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * This function does not perform any verification regarding the parameter validity.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @param hBlock A handle to the shared memory block, or {S_HANDLE_NULL}
 * if the shared memory block is accessed directly through its pointer.
 *
 * @param pMemoryBlock A pointer to the shared memory block for direct access,
 * NULL if the shared memory block is accessed through its handle.
 *
 * @param nOffset the offset, in bytes, of the start of the range to be referenced within the
 * shared memory block.
 *
 * @param nLength the length, in bytes, of the referenced range.
 *
 * @param nFlags the condition access flags to the range of data within the shared memory block.
 * The value of these flags is not interpreted by this function.
 *
 * @pre pEncoder is not NULL.
 *
 */
void libBEFEncoderWriteMemoryReference(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      S_HANDLE hMemoryBlock,
      uint32_t nOffset,
      uint32_t nLength,
      uint32_t nFlags);

/**
 * Opens a new sequence at the end of the encoded data of the specified Binary
 * Encoding Format encoder instance.
 *
 * Use {libBEFEncoderCloseSequence} to close the newly opened sequence.
 *
 * Opening a new sequence is allowed even if one or more parent sequences are
 * currently opened.  In this case {libBEFEncoderCloseSequence} must be called
 * for each opened sequence.
 *
 * Upon error, this function sets the error state of the encoder.
 * error state equal to S_ERROR_SHORT_BUFFER if sequence stack is too small
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @pre pEncoder is not NULL.
 */
void libBEFEncoderOpenSequence(
      IN OUT LIB_BEF_ENCODER* pEncoder);


/**
 * Closes the last opened sequence for the specified Binary Encoding Format
 * encoder instance.
 *
 * At least one sequence must be currently opened for the encoder.  Otherwise
 * this function sets its error state to {S_ERROR_BAD_STATE}.
 *
 * This function does not increase the current offset, so it cannot fail
 * due with error S_ERROR_SHORT_BUFFER.
 *
 * This function does nothing if the error state of the encoder is set upon
 * entry.
 *
 * @param pEncoder A pointer to the encoder instance.
 *
 * @pre pEncoder is not NULL.
 */
void libBEFEncoderCloseSequence(
      IN OUT LIB_BEF_ENCODER* pEncoder);

#ifdef __cplusplus
}
#endif

#endif  /* !defined(__LIB_BEF_ENCODER_H__) */
