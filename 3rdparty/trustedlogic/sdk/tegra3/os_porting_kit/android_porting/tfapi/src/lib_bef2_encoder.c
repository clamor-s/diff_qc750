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
#ifdef ANDROID
#include <stddef.h>
#endif

#ifndef SMC_OMAP3430_SYMBIAN_KERNEL
#include <string.h>
#include <assert.h>
#endif

#include "lib_bef2_internal.h"
#include "lib_bef2_encoder.h"

#include "s_error.h"


/*---------------------------------------------------------------------------
 * Basic encoder append functions
 *---------------------------------------------------------------------------*/

/*
 * Writes the specified element at the end of the encoded data of the specified
 * encoder monitor.
 *
 * These functions do not check that encoder capacity, call libBEFEncoderEnsureCapacity
 * before
 *
 * This function does nothing if the error state of the encoder monitor is set
 * upon entry.
 */
static void static_libBEFEncoderAppendUint8(
            LIB_BEF_ENCODER* pEncoder,
            uint8_t nValue)
{
   assert(pEncoder != NULL);

   if (pEncoder->nError != S_SUCCESS)
   {
      return;
   }

   pEncoder->pBuffer[pEncoder->nSize++] = nValue;
}
#ifndef INCLUDE_MINIMAL_BEF
static void static_libBEFEncoderAppendUint16(
            LIB_BEF_ENCODER* pEncoder,
            uint16_t nValue)
{
   assert(pEncoder != NULL);

   if (pEncoder->nError != S_SUCCESS)
   {
      return;
   }

   pEncoder->pBuffer[pEncoder->nSize++] = (uint8_t)(nValue & 0xFF);
   pEncoder->pBuffer[pEncoder->nSize++] = (uint8_t)(nValue >> 8);
}
#endif /* INCLUDE_MINIMAL_BEF */

static void static_libBEFEncoderAppendUint32(
            LIB_BEF_ENCODER* pEncoder,
            uint32_t nValue)
{
   assert(pEncoder != NULL);

   if (pEncoder->nError != S_SUCCESS)
   {
      return;
   }

   pEncoder->pBuffer[pEncoder->nSize++] = (uint8_t)(nValue & 0xFF);
   pEncoder->pBuffer[pEncoder->nSize++] = (uint8_t)((nValue >> 8) & 0xFF);
   pEncoder->pBuffer[pEncoder->nSize++] = (uint8_t)((nValue >> 16) & 0xFF);
   pEncoder->pBuffer[pEncoder->nSize++] = (uint8_t)(nValue >> 24);
}

static void static_libBEFEncoderAppendRaw(
                                      LIB_BEF_ENCODER* pEncoder,
                                      const void* pData,
                                      uint32_t nCount)
{
   assert(pEncoder != NULL);

   if (pEncoder->nError != S_SUCCESS)
   {
      return;
   }

   if (nCount == 0)
   {
      /* nothing to write */
      return;
   }

   memcpy(&pEncoder->pBuffer[pEncoder->nSize],
               pData,
               nCount);

   pEncoder->nSize += nCount;
}


/*---------------------------------------------------------------------------
 * Utility Functions
 *---------------------------------------------------------------------------*/
/* Ensure that the encoder can accomodate nOverHeadSize+nDataSize
   additional bytes. Otherwise, set its error state to S_ERROR_SHORT_BUFFER
   and set pEncoder->nRequiredCapacity to the required capacity. Set
   error state to S_ERROR_OUT_OF_MEMORY if the required capacity cannot
   be expressed on 32 bits */
bool libBEFEncoderEnsureCapacity(
                  LIB_BEF_ENCODER* pEncoder,
                  uint32_t nOverHeadSize,
                  uint32_t nDataSize)
{
   uint32_t nRequiredSize;
   assert(pEncoder != NULL);
   assert(pEncoder->nSize <= pEncoder->nCapacity);

   nRequiredSize = pEncoder->nSize + nOverHeadSize;
   if (nRequiredSize < nOverHeadSize)
   {
      /* The addition overflowed */
      pEncoder->nRequiredCapacity = 0xFFFFFFFF;
      pEncoder->nError = S_ERROR_OUT_OF_MEMORY;
      return false;
   }
   nRequiredSize += nDataSize;
   if (nRequiredSize < nDataSize)
   {
      /* The addition overflowed */
      pEncoder->nRequiredCapacity = 0xFFFFFFFF;
      pEncoder->nError = S_ERROR_OUT_OF_MEMORY;
      return false;
   }
   if (nRequiredSize > pEncoder->nCapacity)
   {
      /* Encoder capacity exceeded */
      pEncoder->nError = S_ERROR_SHORT_BUFFER;
      pEncoder->nRequiredCapacity = nRequiredSize;
      return false;
   }
   return true;
}

#ifndef EXCLUDE_UTF8
#ifdef WCHAR_SUPPORTED
/*
 * Note on wchar_t interpretation:
 *
 * wchar_t can be either 2-byte (e.g., Windows) or 4-byte (e.g., Linux) long
 *
 * For 2-byte wchar_t, we assume a UTF-16 encoding, as specified
 * in RFC 2781. Unicode characters greater or equal to 0x10000 should be
 * encoded in two wchar_t values (surrogate pair). The code below is a
 * bit more permissive. When it encounters a high surrogate(0xD800-0xDBFF) it
 * does not check that the following item is a low surrogate (0xDC00-0xDFFF)
 * but just extracts the significant bits out of it. On the other hand,
 * anything that looks like a low surrogate (0xDC00-0xDFFF) is simply ignored,
 * whether is follows a high surrogate or not.
 *
 * For 4-byte wchar_t, we assume a one-to-one mapping between Unicode character
 * and the wchar_t value. However, values between 0xD800 and 0xDFFF, which do
 * not represent valid Unicode characters, are interpreted as if they a UTF-16
 * encodings.
 */
static uint32_t static_libBEFEncoderGetWcharStringLengthAsUTF8(
         const wchar_t *pWcharString)
{
   uint32_t nResult = 0;

   while (true)
   {
      uint32_t nWchar = *pWcharString++;
      if (nWchar == 0)
      {
         break;
      }
      if (nWchar < 0x80)
      {
         /* One-byte encoding */
         nResult++;
      }
      else if (nWchar < 0x0800)
      {
         /* Two-byte encoding */
         nResult += 2;
      }
      else
      {
         if (nWchar >= 0xD800 && nWchar < 0xDC00)
         {
            /* High surrogate. The Unicode character value necessarily >= 0x10000
               and will use a four-byte UTF-8 encoding */
            nResult += 4;
         }
         else if (nWchar >= 0xDC00 && nWchar < 0xE00)
         {
            /* Low surrogate. Ignore it */
         }
         else if (nWchar < 0x10000)
         {
            /* Three-byte encoding */
            nResult += 3;
         }
         else
         {
            /* Four-byte encoding */
            nResult += 4;
         }
      }
   }

   return nResult;
}

static void static_libBEFEncoderWcharToUtf8(
      uint8_t* pDestUtf8,
      const wchar_t* pSrcWchar)
{
   while (true)
   {
      uint32_t
      nWchar = (uint32_t)*pSrcWchar++;
      if (nWchar == 0)
      {
         return;
      }
      if (nWchar < 0x80)
      {
         /* One-byte encoding */
         *pDestUtf8++ = (uint8_t)nWchar;
      }
      else if (nWchar < 0x0800)
      {
         /* Two-byte encoding */
         *pDestUtf8++ = (uint8_t)(((nWchar >> 6)) | 0xC0);
         *pDestUtf8++ = (uint8_t)((nWchar & 0x3F) | 0x80);
      }
      else
      {
         if (nWchar >= 0xD800 && nWchar < 0xDC00)
         {
            /* high surrogate. Adjust character value */
            nWchar = 0x10000 + (((nWchar & 0x3FF) << 10) | ((*(pSrcWchar + 1)) & 0x3FF));
            pSrcWchar++;
         }
         else if (nWchar >= 0xDC00 && nWchar < 0xE000)
         {
            /* low surrogate: ignore */
            continue;
         }
         if (nWchar < 0x10000)
         {
            /* Three-byte encoding */
            *pDestUtf8++ = (uint8_t)((nWchar >> 12) | 0xE0);
            *pDestUtf8++ = (uint8_t)(((nWchar >> 6) & 0x3F) | 0x80);
            *pDestUtf8++ = (uint8_t)((nWchar & 0x3F) | 0x80);
         }
         else
         {
            /* Four-byte encoding */
            *pDestUtf8++ = (uint8_t)((nWchar >> 18) | 0xF0);
            *pDestUtf8++ = (uint8_t)(((nWchar >> 12) & 0x3F) | 0x80);
            *pDestUtf8++ = (uint8_t)(((nWchar >> 6) & 0x3F) | 0x80);
            *pDestUtf8++ = (uint8_t)((nWchar & 0x3F) | 0x80);
         }
      }
   }
}
#endif
#endif /* EXCLUDE_UTF8 */


/*---------------------------------------------------------------------------
 * API Implementation
 *---------------------------------------------------------------------------*/

void libBEFEncoderInit(
      LIB_BEF_ENCODER* pEncoder)
{
   assert(pEncoder != NULL);

   pEncoder->nSize = 0;
   pEncoder->nError = S_SUCCESS;
   /*
    * lib_bef2_decoder uses index 0 as a global sequence
    * simulate the same behavior by simply skipping the first index
    */
   pEncoder->nOffsetStackIndex = 1;
   memset(pEncoder->nOffsetStack, 0x00, sizeof(pEncoder->nOffsetStack));
}

static void static_libBEFEncoderWriteFixedLengthType(
                        LIB_BEF_ENCODER* pEncoder,
                        uint8_t nType,
                        uint32_t nValue)
{
   assert(pEncoder != NULL);

   switch(nType)
   {
#ifndef INCLUDE_MINIMAL_BEF
   case LIB_BEF_BASE_TYPE_BOOLEAN:
      if (libBEFEncoderEnsureCapacity(pEncoder,1,0))
      {
         /* For booleans, the tag encodes the value itself */
         static_libBEFEncoderAppendUint8(pEncoder, nValue != 0 ? LIB_BEF_TAG_TRUE : LIB_BEF_TAG_FALSE);
      }
      break;
#endif /* INCLUDE_MINIMAL_BEF */
   case LIB_BEF_BASE_TYPE_UINT8:
      if (libBEFEncoderEnsureCapacity(pEncoder,1,1))
      {
         static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_BASE_TYPE_UINT8 | LIB_BEF_FLAG_VALUE_EXPLICIT);
         static_libBEFEncoderAppendUint8(pEncoder, (uint8_t)nValue);
      }
      break;
#ifndef INCLUDE_MINIMAL_BEF
   case LIB_BEF_BASE_TYPE_UINT16:
      if (libBEFEncoderEnsureCapacity(pEncoder,1,2))
      {
         static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_BASE_TYPE_UINT16 | LIB_BEF_FLAG_VALUE_EXPLICIT);
         static_libBEFEncoderAppendUint16(pEncoder, (uint16_t)nValue);
      }
      break;
#endif /* INCLUDE_MINIMAL_BEF */
   case LIB_BEF_BASE_TYPE_UINT32:
#ifndef INCLUDE_MINIMAL_BEF
   case LIB_BEF_BASE_TYPE_HANDLE:
#endif /* INCLUDE_MINIMAL_BEF */
      if (libBEFEncoderEnsureCapacity(pEncoder,1,4))
      {
         static_libBEFEncoderAppendUint8(pEncoder, nType | LIB_BEF_FLAG_VALUE_EXPLICIT);
         static_libBEFEncoderAppendUint32(pEncoder, (uint32_t)nValue);
      }
      break;
   default: break;
   }
}


static void static_libBEFEncoderWriteFixedLengthTypeArray(
                       LIB_BEF_ENCODER* pEncoder,
                       uint8_t nType,
                       uint32_t nArrayLength,
                       const void* pnArray)
{
   uint8_t nTag;
   uint32_t i;

   assert(pEncoder != NULL);

   if (pEncoder->nError != S_SUCCESS)
   {
      return;
   }

   if(pnArray == NULL)
   {
      nTag = (uint8_t)(nType|LIB_BEF_FLAG_NULL_ARRAY);
      if (libBEFEncoderEnsureCapacity(pEncoder, 1, 0))
      {
         static_libBEFEncoderAppendUint8(pEncoder,nTag);
      }
      return;
   }
   else
   {
      nTag = (uint8_t)(nType|LIB_BEF_FLAG_NON_NULL_ARRAY);

      switch(nType)
      {
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_BOOLEAN:
#endif /* INCLUDE_MINIMAL_BEF */
      case LIB_BEF_BASE_TYPE_UINT8:
         if (libBEFEncoderEnsureCapacity(pEncoder, 5, nArrayLength))
         {
            /* Write the tag */
            static_libBEFEncoderAppendUint8(pEncoder,nTag);
            /* write the array length as uint32_t */
            static_libBEFEncoderAppendUint32(pEncoder,nArrayLength);
#ifndef INCLUDE_MINIMAL_BEF
            if (nType == LIB_BEF_BASE_TYPE_BOOLEAN)
            {
               /* We must normalize each boolean to 0x00 or 0x01 */
               uint32_t i;
               for (i = 0; i < nArrayLength; i++)
               {
                  static_libBEFEncoderAppendUint8(pEncoder, ((bool*)pnArray)[i] ? LIB_BEF_TAG_TRUE : LIB_BEF_TAG_FALSE);
               }
            }
            else
#endif /* INCLUDE_MINIMAL_BEF */
            {
               /* Simply copy the uint8_t array */
               static_libBEFEncoderAppendRaw(pEncoder, pnArray, nArrayLength);
            }
         }
         break;
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_UINT16:
         if (libBEFEncoderEnsureCapacity(pEncoder, 5, nArrayLength<<1))
         {
            /* Write the tag */
            static_libBEFEncoderAppendUint8(pEncoder,nTag);
            /* write the array length as uint32_t */
            static_libBEFEncoderAppendUint32(pEncoder,nArrayLength);
            for (i = 0; i < nArrayLength; i++)
            {
               static_libBEFEncoderAppendUint16(pEncoder,((uint16_t*)pnArray)[i]);
            }
         }
         break;
#endif /* INCLUDE_MINIMAL_BEF */
      case LIB_BEF_BASE_TYPE_UINT32:
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_HANDLE:
#endif /* INCLUDE_MINIMAL_BEF */
         if (libBEFEncoderEnsureCapacity(pEncoder, 5, nArrayLength<<2))
         {
            /* Write the tag */
            static_libBEFEncoderAppendUint8(pEncoder,nTag);
            /* write the array length as uint32_t */
            static_libBEFEncoderAppendUint32(pEncoder,nArrayLength);
            for (i = 0; i < nArrayLength; i++)
            {
               static_libBEFEncoderAppendUint32(pEncoder,((uint32_t*)pnArray)[i]);
            }
         }
         break;
      default:
         break;
      }
   }
}

#ifndef INCLUDE_MINIMAL_BEF
void libBEFEncoderWriteBoolean(
                               LIB_BEF_ENCODER* pEncoder,
                               bool bValue)
{
   static_libBEFEncoderWriteFixedLengthType(
      pEncoder,
      LIB_BEF_BASE_TYPE_BOOLEAN,
      bValue);
}
#endif /* INCLUDE_MINIMAL_BEF */


void libBEFEncoderWriteUint8(
                             LIB_BEF_ENCODER* pEncoder,
                             uint8_t nValue)
{
   static_libBEFEncoderWriteFixedLengthType(
      pEncoder,
      LIB_BEF_BASE_TYPE_UINT8,
      nValue);
}


void libBEFEncoderWriteUint8Array(
                                  LIB_BEF_ENCODER* pEncoder,
                                  uint32_t nArrayLength,
                                  const uint8_t* pnArray)
{
   static_libBEFEncoderWriteFixedLengthTypeArray(
      pEncoder,
      LIB_BEF_BASE_TYPE_UINT8,
      nArrayLength,
      pnArray);
}


#ifndef INCLUDE_MINIMAL_BEF
void libBEFEncoderWriteUint16(
                              LIB_BEF_ENCODER* pEncoder,
                              uint16_t nValue)
{
   static_libBEFEncoderWriteFixedLengthType(
      pEncoder,
      LIB_BEF_BASE_TYPE_UINT16,
      nValue);
}


void libBEFEncoderWriteUint16Array(
                                   LIB_BEF_ENCODER* pEncoder,
                                   uint32_t nArrayLength,
                                   const uint16_t* pnArray)
{
   static_libBEFEncoderWriteFixedLengthTypeArray(
      pEncoder,
      LIB_BEF_BASE_TYPE_UINT16,
      nArrayLength,
      pnArray);
}
#endif /* INCLUDE_MINIMAL_BEF */


void libBEFEncoderWriteUint32(
                              LIB_BEF_ENCODER* pEncoder,
                              uint32_t nValue)
{
   static_libBEFEncoderWriteFixedLengthType(
      pEncoder,
      LIB_BEF_BASE_TYPE_UINT32,
      nValue);
}


#ifndef INCLUDE_MINIMAL_BEF
void libBEFEncoderWriteUint32Array(
                                   LIB_BEF_ENCODER* pEncoder,
                                   uint32_t nArrayLength,
                                   const uint32_t* pnArray)
{
   static_libBEFEncoderWriteFixedLengthTypeArray(
      pEncoder,
      LIB_BEF_BASE_TYPE_UINT32,
      nArrayLength,
      pnArray);
}

void libBEFEncoderWriteHandle(
                              LIB_BEF_ENCODER* pEncoder,
                              S_HANDLE nValue)
{
   static_libBEFEncoderWriteFixedLengthType(
      pEncoder,
      LIB_BEF_BASE_TYPE_HANDLE,
      nValue);
}


void libBEFEncoderWriteHandleArray(
                                   LIB_BEF_ENCODER* pEncoder,
                                   uint32_t nArrayLength,
                                   const S_HANDLE* pnArray)
{
   static_libBEFEncoderWriteFixedLengthTypeArray(
      pEncoder,
      LIB_BEF_BASE_TYPE_HANDLE,
      nArrayLength,
      pnArray);
}
#endif /* INCLUDE_MINIMAL_BEF */



void libBEFEncoderWriteString(
                              LIB_BEF_ENCODER* pEncoder,
                              IN const char* pString)
{
   if (pString == NULL)
   {
      if (libBEFEncoderEnsureCapacity(pEncoder, 1, 0))
      {
         static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_BASE_TYPE_STRING | LIB_BEF_FLAG_NULL_VALUE);
      }
   }
   else
   {
      uint32_t nStringLen;
      for (nStringLen = 0; pString[nStringLen] != 0; nStringLen++) ;
      
      if (libBEFEncoderEnsureCapacity(pEncoder, 5, nStringLen))
      {
         static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_BASE_TYPE_STRING | LIB_BEF_FLAG_VALUE_EXPLICIT);
         /* Write the size in bytes of the string as a uint32_t */
         static_libBEFEncoderAppendUint32(pEncoder, nStringLen);

         /* write the string content */
         memcpy(pEncoder->pBuffer+pEncoder->nSize,
            pString,
            nStringLen);
         pEncoder->nSize += nStringLen;
      }
   }
}

#ifndef EXCLUDE_UTF8
#ifdef WCHAR_SUPPORTED
void libBEFEncoderWriteWString(
                              LIB_BEF_ENCODER* pEncoder,
                              IN const wchar_t* pString)
{
   if (pString == NULL)
   {
      if (libBEFEncoderEnsureCapacity(pEncoder, 1, 0))
      {
         static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_BASE_TYPE_STRING | LIB_BEF_FLAG_NULL_VALUE);
      }
   }
   else
   {
      uint32_t nStringLen = static_libBEFEncoderGetWcharStringLengthAsUTF8(pString);

      if (libBEFEncoderEnsureCapacity(pEncoder, 5, nStringLen))
      {
         static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_BASE_TYPE_STRING | LIB_BEF_FLAG_VALUE_EXPLICIT);
         /* Write the size in bytes of the string as a uint32_t */
         static_libBEFEncoderAppendUint32(pEncoder, nStringLen);

         /* write the string content */
         static_libBEFEncoderWcharToUtf8(
            &pEncoder->pBuffer[pEncoder->nSize],
            pString);
         pEncoder->nSize += nStringLen;
      }
   }
}
#endif
#endif /* EXCLUDE_UTF8 */


void libBEFEncoderOpenSequence(LIB_BEF_ENCODER* pEncoder)
{
   assert(pEncoder != NULL);

   if (pEncoder->nError != S_SUCCESS)
   {
      return;
   }

   /* If we reach the max depth (1 to n), set an error */
   if (pEncoder->nOffsetStackIndex == LIB_BEF_ENCODER_MAX_SEQUENCE_DEPTH)
   {
      pEncoder->nError = S_ERROR_OUT_OF_MEMORY;
      return;
   }

   if (libBEFEncoderEnsureCapacity(pEncoder, 5, 0))
   {
      pEncoder->nOffsetStack[pEncoder->nOffsetStackIndex] = pEncoder->nSize;
      pEncoder->nOffsetStackIndex++;
      /* nSize+5 to skip the tag and length*/
      pEncoder->nSize += 5;
   }
}


void libBEFEncoderCloseSequence(LIB_BEF_ENCODER* pEncoder)
{
   uint32_t nCurrentPosition;
   uint32_t nCurrentSequenceOffset;
   uint32_t nCurrentSequenceSize;

   assert(pEncoder != NULL);

   if (pEncoder->nError != S_SUCCESS)
   {
      return;
   }

   /* Encoder nOffsetStackIndex starts at index 1, first opened sequence is at index 2 */
   if (pEncoder->nOffsetStackIndex == 1)
   {
      pEncoder->nError = S_ERROR_BAD_STATE;
      return;
   }

   nCurrentPosition = pEncoder->nSize;

   /* retrieve the parent sequence info stored when the sequence was opened */
   nCurrentSequenceOffset=pEncoder->nOffsetStack[pEncoder->nOffsetStackIndex-1];

   /* compute sequence size */
   nCurrentSequenceSize = pEncoder->nSize - nCurrentSequenceOffset - 5;

   /* temporarily "rewind" the encoder to the start of the sequence */
   pEncoder->nSize = nCurrentSequenceOffset;

   /* Write tag */
   static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_TAG_SEQUENCE);
   static_libBEFEncoderAppendUint32(pEncoder, nCurrentSequenceSize);

   /* Restore current position */
   pEncoder->nSize = nCurrentPosition;

   /* Pop current sequence from the stack */
   pEncoder->nOffsetStackIndex--;
}


void libBEFEncoderWriteUUID(
      IN  LIB_BEF_ENCODER* pEncoder,
      IN  const S_UUID* pIdentifier)
{
   assert(pIdentifier != NULL);

   if (libBEFEncoderEnsureCapacity(pEncoder, 2, 16))
   {
      static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_TAG_COMPOSITE);
      static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_SUBTAG_UUID);
      static_libBEFEncoderAppendRaw(pEncoder, (uint8_t*)pIdentifier, 16); /* Endianness assume little-endian here */
   }
}

void libBEFEncoderWriteMemoryReference(
      IN OUT LIB_BEF_ENCODER* pEncoder,
      IN  S_HANDLE hMemoryBlock,
      IN  uint32_t nOffset,
      IN  uint32_t nLength,
      IN  uint32_t nFlags)
{
   if (libBEFEncoderEnsureCapacity(pEncoder,2, 16))
   {
      static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_TAG_COMPOSITE);
      static_libBEFEncoderAppendUint8(pEncoder, LIB_BEF_SUBTAG_MEM_REF);
      static_libBEFEncoderAppendUint32(pEncoder, (uint32_t)hMemoryBlock);
      static_libBEFEncoderAppendUint32(pEncoder, nOffset);
      static_libBEFEncoderAppendUint32(pEncoder, nLength);
      static_libBEFEncoderAppendUint32(pEncoder, nFlags);
   }
}
