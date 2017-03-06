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

#ifndef __LIB_CHAR_H__
#define __LIB_CHAR_H__


#ifdef __cplusplus
extern "C" {
#endif
#if 0
}  /* balance curly quotes */
#endif


#include <stddef.h>

#include "s_type.h"

/**
 * @item    Topic
 * @name    UTF-8 and UTF-16 Encoding
 *
 * The UTF-8 and UTF-16 encoding are defined in the ISO/IEC standard 10646-1
 *
 * In ISO 10646, each character is assigned a number, which Unicode calls the
 * Unicode scalar value. This number is the same as the UCS-4 value of the
 * character, and this topic will refer to it as the "character value" for
 * brevity.
 *
 * TF4T uses a limited range of characters with a character value inferior to
 * 0x10000.
 *
 * In the UTF-16 encoding, characters are represented using either one or two
 * unsigned 16-bit integers, depending on the character value. Characters with
 * values less than 0x10000 are represented as a single 16-bit integer with a
 * value equal to that of the character number.
 *
 * UTF-8 strings are encoded so that character sequences that contain only
 * ASCII characters can be represented using only one byte per character,
 * but characters with a value up to 0xFFFF can be represented.
 *
 * The bytes of the UTF-8 encoding are stored in big-endian (high byte first)
 * order.
 *
 * All characters in the range 'u0000' to 'u007F' are represented by a single
 * byte:
 *
 * <code>   0 bits 0-7   </code>
 *
 * The seven bits of data in the byte give the value of the character
 * represented.
 *
 * The characters in the range 'u0080' to 'u07FF' are represented by a pair
 * of bytes x and y:
 *
 * <code>   x: 1 1 0 bits 6-10    y: 1 0 bits 0-5  </code>
 *
 * The bytes represent the character with the value :
 *
 * <code>   ((x & 0x1f) << 6) + (y & 0x3f) </code>
 *
 * Characters in the range 'u0800' to 'uFFFF' are represented by three bytes
 * x, y, and z:
 *
 * <code>   x: 1 1 1 0 bits 12-15   y: 1 0 bits 6-11   z: 1 0 bits 0-5  </code>
 *
 * The bytes represent the character with the value :
 *
 * <code>   ((x & 0xf) << 12) + ((y & 0x3f) << 6) + (z & 0x3f)  </code>
 */


/**
 * Declares a constant Utf8 string in the source code. The LIB_CHAR_UTF8_POINTER
 * macro is used to retreive the constant Utf8 string pointer.
 *
 * The LIB_CHAR_UTF8_POINTER macro can only be used in the file where the string
 * is declared by LIB_CHAR_UTF8_DECLARE.
 *
 * <b>Example:</b>
 * <code>
 *   LIB_CHAR_UTF8_DECLARE( ANY_CONSTANT_STRING, "A constant Ascii string");
 *
 *   ...
 *
 *   PUTF8STRING pConstantUtf8 = LIB_CHAR_UTF8_POINTER( ANY_CONSTANT_STRING );
 * </code>
 *
 * See LIB_CHAR_UTF8_POINTER to access to the Utf8 string value.
 *
 * @syntax  LIB_CHAR_UTF8_DECLARE(string_tag, ascii_string)
 *
 * @param   string_tag     the name of the string.
 *
 * @param   ascii_string   a constant Ascii string.
 */
#define LIB_CHAR_UTF8_DECLARE(string_tag, ascii_string) \
static const struct string_tag##_s \
{ const uint16_t count; const uint8_t buffer[sizeof(ascii_string)]; \
} g_x##string_tag = { sizeof(ascii_string) - 1, ascii_string}


/**
 * Retreives a constant Utf8 string declared with LIB_CHAR_UTF8_DECLARE.
 *
 * The LIB_CHAR_UTF8_POINTER macro can only be used in the c file where the
 * string is declared by LIB_CHAR_UTF8_DECLARE.
 *
 * See the LIB_CHAR_UTF8_DECLARE macro to declare the Utf8 string value.
 *
 * @syntax   PUTF8STRING LIB_CHAR_UTF8_POINTER(string_tag)
 *
 * @param    string_tag    a constant Ascii string declared with
 *                         LIB_CHAR_UTF8_DECLARE.
 *
 * @return   A pointer on the constant Utf8 string.
 */
#define LIB_CHAR_UTF8_POINTER(string_tag) \
   ((PUTF8STRING)(&g_x##string_tag))



/**
 * Converts the specified uint32_t value to its UTF-8 representation in the
 * specified radix.
 *
 * @param nValue The value to convert.
 *
 * @param nRadix The radix to be used for the conversion.
 *
 * @param pBuffer A pointer to the buffer receiving the resulting UTF-8 string.
 * If this parameter is set to NULL, the resulting string is not copied.
 *
 * @param nBufferCapacity The maximum number of bytes that can be stored in the
 * buffer referenced by <tt>pBuffer</tt>.  The value of this parameter is
 * ignored if <tt>pBuffer</tt> is set to NULL.
 *
 * @param pnBufferSize A pointer to the placeholder to be set to the size, in
 * bytes, of the resulting UTF-8 string.  If <tt>pBuffer</tt> is not NULL, the
 * value of this placeholder is always less than or equal to
 * <tt>nBufferCapacity</tt>.
 *
 * @in      nFill  The fill character. If this value is non-zero, the value is
 *          right-aligne in the buffer and the remaining left characters are
 *          set with the value of <tt>nFill</tt>. If this value is zero, the
 *          value is left-aligned and the buffer is truncated.
 *
 * @pre <tt>nRadix</tt> must be between 2 and 36, both inclusive.
 */
void libCharItoA(
      uint32_t nValue,
      uint32_t nRadix,
      OUT uint8_t* pBuffer,
      uint32_t nBufferCapacity,
      OUT uint32_t* pnBufferSize,
      char nFill);

#if 0
{  /* balance curly quotes */
#endif
#ifdef __cplusplus
}  /* closes extern "C" */
#endif


#endif  /* !defined(__LIB_CHAR_H__) */
