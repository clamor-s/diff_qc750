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

#ifndef __LIB_BEF_DECODER_H__
#define __LIB_BEF_DECODER_H__

#include "s_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Consistency requirements:
 * ------------------------
 *
 * The implementation must not assume that the content of the encoded buffer remains
 * unchanged between two calls or the library or during a call of the library. This is because
 * the content of the encoded buffer might lie in non-secure memory while the caller of the
 * library executes in the secure world.
 *
 * Additionnally, the implementation must guarantee that it exposes a consistent message
 * structure to the caller: if the caller, directly or indirectly, determines
 * any information about the type of the current element, the length of the current element,
 * or whether there is a current element, then this information must not change until the current
 * element changes (because it is skipped or read for example), even if the encoded buffer
 * is modified between the calls. This constraint does not
 * apply to the length of the wide string view of strings, because it is used only by the
 * normal world (SMAPI). Note that this constraint does not apply to the data itself, but only the the type/lengths
 * So, if the caller invokes one of the copy-array functions twice on the same data, the implementation is not
 * required to return the same result.
 *
 * More precisely:
 * - if one of the functions libBEFDecoderHasData, libBEFReadArrayLength, libBEFDecoderReadStringLength
 *   is called twice on the same offset in the same sequence, it must
 *   return the same result
 * - if one of the functions libBEFDecoderCopyStringAsUTF8, libBEFDecoderCopyUint8Array,
 *   libBEFDecoderCopyUint16Array, libBEFDecoderCopyUint32Array,
 *   libBEFDecoderCopyHandleArray is called twice on the same
 *   offset in the same sequence, then:
 *     * either both the calls must fail with error S_ERROR_BAD_FORMAT or none of them
 *     * both of the calls must return an information about the array length which is consistent with the length
 *        returned by libBEFDecoderReadArrayLength
 */




#define LIB_BEF_DECODER_MAX_SEQUENCE_DEPTH 16

#define LIB_BEF_NULL_ELEMENT ((uint32_t)0xFFFFFFFF)

#define LIB_BEF_TYPE_BOOLEAN         0x00000001
#define LIB_BEF_TYPE_UINT8           0x00000016
#define LIB_BEF_TYPE_UINT16          0x00000026
#define LIB_BEF_TYPE_UINT32          0x00000036
#define LIB_BEF_TYPE_STRING          0x00000071
#define LIB_BEF_TYPE_BOOLEAN_ARRAY   0x0000000E
#define LIB_BEF_TYPE_UINT8_ARRAY     0x0000001E
#define LIB_BEF_TYPE_UINT16_ARRAY    0x0000002E
#define LIB_BEF_TYPE_UINT32_ARRAY    0x0000003E
#define LIB_BEF_TYPE_STRING_ARRAY    0x0000007E
#define LIB_BEF_TYPE_SEQUENCE        0x00000096
#define LIB_BEF_TYPE_UUID            0x000000B6
#define LIB_BEF_TYPE_MEMREF          0x000001B6
#define LIB_BEF_TYPE_NO_DATA         0x000000FF

/*
 * Describes a BEF decoder instance.
 */
typedef struct LIB_BEF_DECODER
{
   /*
    * A pointer to the first byte of the encoded data parsed by this decoder.
    * Datas can be modified.
    */
   uint8_t* pEncodedData;
   /*
    * The total size, in bytes, of the memory block referenced by pEncodedData.
    * @pre nEncodedSize cannot be modified
    */
   uint32_t nEncodedSize;
   /*
    * The error code describing the decoder's error state, or S_SUCCESS if
    * the decoder's error state is not set.
    * S_ERROR_BAD_FORMAT wrong tag found or wrong length, or more generally ill-formed message
    *
    */
   S_RESULT nError;
   /*
    * The current index in the offset stack.
    */
   uint32_t nOffsetStackIndex;
   /*
    * The current offset stack of this decoder. Each element indicates
    * the current offset in a sequence. When a sequence is open, the offset
    * of the parent decoder is set to the next element after the sequence.
    */
   uint32_t nOffsetStack[LIB_BEF_DECODER_MAX_SEQUENCE_DEPTH];

   /*
    * The tag of the current element, or:
    *  - LIB_BEF_TAG_UNKNOWN if not the current tag has not yet been read
    */
   uint8_t nCachedTag;
   /*
    * The subtag of the current element, or:
    *  - LIB_BEF_TAG_UNKNOWN if not the current subtag has not yet been read
    * Only used for composite elements.
    */
   uint8_t nCachedSubTag;

   /*
    * Cache of the first and second words following the tag. Those are used
    * to cache the array or string length to guarantee a consistent view of
    * the message structure even if the content is dynamically tampered.
    * Set to LIB_BEF_NULL_ELEMENT if unknown
    */
   uint32_t nCachedWords[2];
}
LIB_BEF_DECODER;

/*----------------------------------------------------------------------------
 * Decoder
 *----------------------------------------------------------------------------*/

/**
 * All the exported functions of this library take a pointer to a LIB_BEF_DECODER
 * structure as a first parameter. This structure is internally used by the library
 * and its content is modified by the called functions.
 */

/**
 * Initialize a new Binary Encoding Format decoder instance to parse the specified
 * encoded data.
 *
 * The current offset of the newly created decoder is initially set to the
 * first byte of the encoded data referenced by <tt>pEncodedData</tt>.
 *
 * The error state of the newly created decoder is initially cleared.
 *
 * @param pDecoder A pointer to the decoder.
 *
 * @pre pDecoder is not NULL.
 * @pre pDecoder->pEncodedData is correctly set
 * @pre pDecoder->pEncodedSize is correctly set
 *
 */
void libBEFDecoderInit(
      OUT LIB_BEF_DECODER* pDecoder);

/**
 * Indicates if there is some data left to be decoded in the encoded data
 * parsed by the specified Binary Encoding Format decoder instance.
 *
 * If the current offset of the decoder points within a sequence opened with
 * {libBEFDecoderOpenSequence}, this function only indicates if there is
 * some data left to be decoded in the encoded data of the current sequence.
 *
 * If the error state of <tt>pDecoder</tt> is set upon entry, this function
 * returns FALSE.
 *
 * @param pDecoder The pointer of the decoder instance.
 *
 * @return TRUE if there is some data left to be decoded in <tt>pDecoder</tt>,
 * or FALSE otherwise.
 *
 * @pre pDecoder is not NULL.
 *
 */
bool libBEFDecoderHasData(
      IN OUT LIB_BEF_DECODER* pDecoder);


/**
 * Skips the encoded element at the current offset in the encoded data parsed
 * by the specified Binary Encoded Format decoder instance.
 *
 * Upon return, the current offset of the decoder references the first element
 * following the skipped element.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @pre pDecoder is not NULL.
 */
void libBEFDecoderSkip(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * Reads the boolean value at the current offset in the encoded data parsed by
 * the specified Binary Encoded Format decoder instance.
 *
 * Upon return, the current offset of the decoder references the first element
 * following the decoded element.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *                    and if value is not a boolean.
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @return The decoded value. Set to FALSE upon error.
 *
 * @pre pDecoder is not NULL.
 *
 */
bool libBEFDecoderReadBoolean(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * Reads the uint8_t value at the current offset in the encoded data parsed by the
 * specified Binary Encoded Format decoder instance.
 *
 * Upon return, the current offset of the decoder references the first element
 * following the decoded element.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @return the decoded value. Set to zero upon error.
 *
 * @pre pDecoder is not NULL.
 */
uint8_t libBEFDecoderReadUint8(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * Reads the uint16_t value at the current offset in the encoded data parsed by the
 * specified Binary Encoded Format decoder instance.
 *
 * Upon return, the current offset of the decoder references the first element
 * following the decoded element.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @return the decoded value. Set to zero upon error.
 *
 * @pre pDecoder is not NULL.
 */
uint16_t libBEFDecoderReadUint16(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * Reads the uint32_t value at the current offset in the encoded data parsed by the
 * specified Binary Encoded Format decoder instance.
 *
 * Upon return, the current offset of the decoder references the first element
 * following the decoded element.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @return the decoded value. Set to zero upon error.
 *
 * @pre pDecoder is not NULL.
 */
uint32_t libBEFDecoderReadUint32(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * Reads the handle value at the current offset in the encoded data parsed by
 * the specified Binary Encoded Format decoder instance.
 *
 * Upon return, the current offset of the decoder references the first element
 * following the decoded element.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @return the decoded value. Set to zero upon error.
 *
 * @pre pDecoder is not NULL.
 */
S_HANDLE libBEFDecoderReadHandle(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * This function must be called when the current element in the decoder is
 * is a string. In this case, the function returns the number of bytes necessary
 * to represent the string encoded in zero-terminated UTF-8. See RFC3629 for
 * details about the UTF-8 encoding of a Unicode string.
 *
 * The current offset of the decoder upon entry must reference the first byte
 * of value of type string, otherwise the error S_ERROR_BAD_FORMAT is set.
 *
 * If the string is NULL, the function return LIB_BEF_NULL_ELEMENT.
 *
 * Upon return, the current offset of the decoder is left unmodified by this function.
 * Use {libBEFDecoderSkip} to skip the current array value.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @return The decoded string length or LIB_BEF_NULL_ELEMENT upon error.
 *
 * @pre pDecoder is not NULL.
 *
 */
uint32_t libBEFDecoderReadStringLength(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * This function must be called when the current element in the decoder is
 * is a string. In this case, the function returns the number of wchar_t items
 * necessary to represent the string encoded as a zero-terminated array
 * of wchar_t items.
 *
 * The current offset of the decoder upon entry must reference the first byte
 * of an array value, otherwise the error S_ERROR_BAD_FORMAT is set.
 *
 * If the string is NULL, the function return LIB_BEF_NULL_ELEMENT.
 *
 * Upon return, the current offset of the decoder is left unmodified by this function.
 * Use {libBEFDecoderSkip} to skip the current array value.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * Note that the number of items returned by this function may be greater
 * than what is strictly necessary to store the string as a zero-terminated
 * array of wchar_t items.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @return The decoded wchar_t string length or LIB_BEF_NULL_ELEMENT upon error.
 *
 * @pre pDecoder is not NULL.
 *
 */
uint32_t libBEFDecoderReadWStringLength(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * Copies the string at the current offset in the decoder into a buffer provided
 * by the caller. The data actually copied in the buffer is the representation
 * of the string in UTF-8, terminated by a zero byte.
 *
 * The current offset of the decoder upon entry must reference a string value.
 *
 * Upon return, the current offset of the decoder is left unmodified by this function.
 * Use {libBEFDecoderSkip} to skip the current array value.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @param pString A pointer to a buffer filled by the function with the zero-terminated
 *        representation of the string.
 *
 * @return the number of bytes actually copied in <tt>pString</tt>
 *
 * @pre pDecoder is not NULL.
 *
 * @pre pString must point to a buffer of memory that can hold the whole string.
 *
 */
#ifndef EXCLUDE_UTF8
uint32_t libBEFDecoderCopyStringAsUTF8(
      IN OUT LIB_BEF_DECODER* pDecoder,
      OUT char* pString);
#else
uint32_t libBEFDecoderCopyString(
      LIB_BEF_DECODER* pDecoder,
      uint32_t nMaxLength,
      OUT char* pString);
#endif /* INCLUDE_MINIMAL_BEF */

/**
 * Copies the string at the current offset in the decoder into a buffer provided
 * by the caller. The data actually copied in the buffer is the representation
 * of the string as a zero-terminated array of wchar_t items.
 *
 * The current offset of the decoder upon entry must reference a string value.
 *
 * Upon return, the current offset of the decoder is left unmodified by this function.
 * Use {libBEFDecoderSkip} to skip the current array value.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @param pString A pointer to a buffer filled by the function with the zero-terminated wchar_t*
 * representation of the string. Note that if sizeof(wchar_t) is 4, each Unicode characters
 * in the string is represented by one wchar_t element. If sizeof(wchar_t) is 2 however, Unicode
 * characters beyond 0xFFFF are represented by two wchar_t elements.
 *
 * @return the number of wchar_t items actually copied in <tt>pString</tt>
 *
 * @pre pDecoder is not NULL.
 *
 */
#ifdef WCHAR_SUPPORTED
uint32_t libBEFDecoderCopyStringAsWchar(
      IN OUT LIB_BEF_DECODER* pDecoder,
      OUT wchar_t* pString);
#endif

/**
 * Initialize a Binary Encoding Format decoder instance for decoding the
 * sequence at the current offset in the encoded data parsed by the specified
 * decoder instance.
 *
 * The current offset of the new decoder instance references the
 * first element of the decoded sequence.
 *
 * The current offset of the decoder upon entry must reference the first byte
 * of a sequence.
 *
 * Upon return, the current offset of the decoder is set to the element following
 * the sequence.
 *
 * Upon error, this function sets the error state of pDecoder and does nothing
 * on pSequenceDecoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @param pSequenceDecoder A pointer
 * to the new decoder instance.
 *
 * @pre pDecoder is not NULL.
 *
 * @pre pSequenceDecoder is not NULL.
 *
 */
void libBEFDecoderReadSequence(
      IN OUT LIB_BEF_DECODER* pDecoder,
      OUT LIB_BEF_DECODER* pSequenceDecoder);

/**
 * Opens a sequence in the current decoder. The current decoder must point to a
 * sequence.
 *
 * After this function is called, the current decoder points to the first element
 * of the sequence.
 *
 * If the error state of the decoder is set upon entry, this function does nothing.
 *
 * Upon error, this function sets the error state of the current decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *                    and if the decoder does not point to a sequence
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @pre pDecoder is not NULL.
 *
 */
void libBEFDecoderOpenSequence(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * Closes a sequence in the current decoder. At least one sequence must have been
 * opened using the function {LIBBEFDecoderOpenSequence}
 *
 * When this function returns, the current decoder points to the first element
 * following the current sequence and the current error state is reset.
 *
 * If the decoder does not point within a sequence, the error state
 * is set to S_ERROR_BAD_STATE.
 *
 * Contrary to the other functions, this function will succeed even if the current error state
 * is set, provided the decoder points within a sequence.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @pre pDecoder is not NULL.
 */
void libBEFDecoderCloseSequence(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * Reads the number of elements in the current array value at the current
 * offset in the encoded data parsed by the specified Binary Encoded Format
 * decoder instance.
 *
 * The current offset of the decoder upon entry must reference the first byte
 * of an array value.
 *
 * Upon return, the current offset of the decoder is left unmodified by this function.
 * Use {libBEFDecoderSkip} to skip the current array value.
 *
 * If the array is NULL, the function return LIB_BEF_NULL_ELEMENT.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @return The decoded array length or LIB_BEF_NULL_ELEMENT upon error.
 *
 * @pre pDecoder is not NULL.
 *
 */
uint32_t libBEFDecoderReadArrayLength(
      IN OUT LIB_BEF_DECODER* pDecoder);

/**
 * Copies a region of the uint8_t array value at the current offset in the encoded
 * data parsed by the specified Binary Encoded Format decoder instance.
 *
 * The current offset of the decoder upon entry must reference the first byte of
 * an array value.
 *
 * The current offset of the decoder is left unmodified by this function.  Use
 * {libBEFDecoderSkip} to skip the current array value.
 *
 * The function returns the number of elements actually copied to the buffer pArray.
 * If the array is NULL, nothing is copied to pArray and the function returns LIB_BEF_NULL_ELEMENT.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @param nIndex The index of the first element to copy. The first element in
 * the array has index zero.
 *
 * @param nMaxLength The maximum number of elements to copy. This function may
 * copy fewer than <tt>nMaxLength</tt> elements if the end of the array is found
 * before.
 *
 * @param pnArray A pointer to the buffer to fill with up to <tt>nMaxLength</tt>
 * elements.
 *
 * @return the number of elements actually copied in <tt>pnArray</tt>, or LIB_BEF_NULL_ELEMENT
 * upon failure.
 *
 * @pre pDecoder is not NULL.
 * @pre pArray is a valid pointer on an array of at least nMaxLength elements.
 */
uint32_t libBEFDecoderCopyUint8Array(
      IN OUT LIB_BEF_DECODER* pDecoder,
      uint32_t nIndex,
      uint32_t nMaxLength,
      OUT uint8_t* pnArray);


/**
 * Copies a region of the uint16_t array value at the current offset in the encoded
 * data parsed by the specified Binary Encoded Format decoder instance.
 *
 * The current offset of the decoder upon entry must reference the first byte of
 * an array value.
 *
 * The current offset of the decoder is left unmodified by this function.  Use
 * {libBEFDecoderSkip} to skip the current array value.
 *
 * The function returns the number of elements actually copied to the buffer pArray.
 * If the array is NULL, nothing is copied to pArray and the function returns LIB_BEF_NULL_ELEMENT.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @param nIndex The index of the first element to copy. The first element in
 * the array has index zero.
 *
 * @param nMaxLength The maximum number of elements to copy. This function may
 * copy fewer than <tt>nMaxLength</tt> elements if the end of the array is found
 * before.
 *
 * @param pnArray A pointer to the buffer to fill with up to <tt>nMaxLength</tt>
 * elements.
 *
 * @return the number of elements actually copied in <tt>pnArray</tt>, or LIB_BEF_NULL_ELEMENT
 * upon failure.
 *
 * @pre pDecoder is not NULL.
 * @pre pArray is a valid pointer on an array of at least nMaxLength elements.
 */
uint32_t libBEFDecoderCopyUint16Array(
      IN OUT LIB_BEF_DECODER* pDecoder,
      uint32_t nIndex,
      uint32_t nMaxLength,
      OUT uint16_t* pnArray);


/**
 * Copies a region of the uint32_t array value at the current offset in the encoded
 * data parsed by the specified Binary Encoded Format decoder instance.
 *
 * The current offset of the decoder upon entry must reference the first byte of
 * an array value.
 *
 * The current offset of the decoder is left unmodified by this function.  Use
 * {libBEFDecoderSkip} to skip the current array value.
 *
 * The function returns the number of elements actually copied to the buffer pArray.
 * If the array is NULL, nothing is copied to pArray and the function returns LIB_BEF_NULL_ELEMENT.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @param nIndex The index of the first element to copy. The first element in
 * the array has index zero.
 *
 * @param nMaxLength The maximum number of elements to copy. This function may
 * copy fewer than <tt>nMaxLength</tt> elements if the end of the array is found
 * before.
 *
 * @param pnArray A pointer to the buffer to fill with up to <tt>nMaxLength</tt>
 * elements.
 *
 * @return the number of elements actually copied in <tt>pnArray</tt>, or LIB_BEF_NULL_ELEMENT
 * upon failure.
 *
 * @pre pDecoder is not NULL.
 * @pre pArray is a valid pointer on an array of at least nMaxLength elements.
 */
uint32_t libBEFDecoderCopyUint32Array(
      IN OUT LIB_BEF_DECODER* pDecoder,
      uint32_t nIndex,
      uint32_t nMaxLength,
      OUT uint32_t* pnArray);


/**
 * Copies a region of the handle array value at the current offset in the encoded
 * data parsed by the specified Binary Encoded Format decoder instance.
 *
 * The current offset of the decoder upon entry must reference the first byte of
 * an array value.
 *
 * The current offset of the decoder is left unmodified by this function.  Use
 * {libBEFDecoderSkip} to skip the current array value.
 *
 * The function returns the number of elements actually copied to the buffer pArray.
 * If the array is NULL, nothing is copied to pArray and the function returns LIB_BEF_NULL_ELEMENT.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder A pointer to the decoder instance.
 *
 * @param nIndex The index of the first element to copy. The first element in
 * the array has index zero.
 *
 * @param nMaxLength The maximum number of elements to copy. This function may
 * copy fewer than <tt>nMaxLength</tt> elements if the end of the array is found
 * before.
 *
 * @param pnArray A pointer to the buffer to fill with up to <tt>nMaxLength</tt>
 * elements.
 *
 * @return the number of elements actually copied in <tt>pnArray</tt>, or LIB_BEF_NULL_ELEMENT
 * upon failure.
 *
 * @pre pDecoder is not NULL.
 * @pre pArray is a valid pointer on an array of at least nMaxLength elements.
 */
uint32_t libBEFDecoderCopyHandleArray(
      IN OUT LIB_BEF_DECODER* pDecoder,
      uint32_t nIndex,
      uint32_t nMaxLength,
      OUT S_HANDLE* phArray);

/**
 * Decodes the UUID at the current offset in the specified BEF decoder.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * @param pDecoder The handle of the BEF decoder.
 *
 * @param pUUID A pointer to the placeholder receiving the decoded
 * identifier. This placeholder is set to the nil identifier upon error.
 *
 * @pre pDecoder is not NULL.
 *
 **/
void libBEFDecoderReadUUID(
      IN  LIB_BEF_DECODER* pDecoder,
      OUT S_UUID* pUUID);

/**
 * Decodes the memory reference at the current offset in the specified BEF
 * decoder.
 *
 * Upon error, this function sets the error state of the decoder.
 * S_ERROR_BAD_FORMAT in case described above (cf LIB_BEF_DECODER definition)
 *
 * This function does nothing if the error state of the decoder is set upon
 * entry.
 *
 * This function does not perform any verification regarding the meaning
 * of the output parameters.
 *
 * @param pDecoder The pointer of the BEF decoder.
 *
 * @param phBlock A pointer to the placeholder to be set to the handle of the
 * memory block. This placeholder is set to {Uint32_HANDLE_INVALID} if the shared
 * memory block is accessed directly through its pointer.
 *
 * @param pnOffset A pointer to the placeholder to be set to the offset within
 * the memory range.
 *
 * @param pnLength A pointer to the placeholder to be set to the length of the
 * memory range.
 *
 * @param pnFlags A pointer to the placeholder to be set to the access flags to
 * the memory range.
 *
 * @pre pDecoder is not NULL.
 *
 **/
void libBEFDecoderReadMemoryReference(
      IN OUT LIB_BEF_DECODER* pDecoder,
      OUT S_HANDLE* phMemoryBlock,
      OUT uint32_t* pnOffset,
      OUT uint32_t* pnLength,
      OUT uint32_t* pnFlags);

/**
 * Get the type of the current element, and optionally decodes the current
 * element length. This function does not change the current error state.
 * The current offset of the decoder is left unmodified by this function.
 *
 * If the error state of <tt>pDecoder</tt> is set upon entry, this function
 * returns LIB_BEF_TYPE_NO_DATA.
 * Otherwise, if the decoder is currently at the end of the current sequence,
 * then the function returns LIB_BEF_TYPE_NO_DATA
 * Otherwise, the function decodes the current element.
 * - If the current element is ill-formed, the decoder error state is left
 *   unchanged and the function returns LIB_BEF_TYPE_NO_DATA
 * - Otherwise, the function returns the LIB_BEF_TYPE_<XXX> constant that
 *   corresponds to the type of the current element
 *
 * Note that for strings, the function does not check that the UTF-8 content
 * of the string is well-formed. So, a subsequent call to
 * libBEFDecoderReadStringLength may return an error.
 *
 * If pnArrayLength is non-NULL, the function also updates *pnArrayLength with
 * the length of the element, which exists only for some element types:
 *  - the number of elements in an array or 0xFFFFFFFF if the array is NULL
 *  - 0xFFFFFFFF for any other element type or an error
 *
 * @param pDecoder The pointer of the decoder instance.
 *
 * @param pnArrayLength A pointer to the placeholder receiving the array length.
 *
 * @return LIB_BEF_TYPE_XXX or LIB_BEF_TYPE_NO_DATA upon error.
 *
 * @pre pDecoder is not NULL.
 *
 */
uint32_t libBEFDecoderGetCurrentType(
      IN OUT LIB_BEF_DECODER* pDecoder,
      OUT uint32_t* pnArrayLength);

#ifdef __cplusplus
}
#endif

#endif  /* !defined(__LIB_BEF_DECODER_H__) */
