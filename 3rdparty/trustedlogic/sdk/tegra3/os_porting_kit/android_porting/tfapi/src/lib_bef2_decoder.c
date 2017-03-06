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

#include <string.h>
#include <assert.h>

#include "lib_bef2_internal.h"
#include "lib_bef2_decoder.h"

#define LIB_BEF_TAG_UNKNOWN    0xFF
#define LIB_BEF_TAG_INVALID    0xFE
#define LIB_BEF_SUBTAG_INVALID 0xFD

#include "s_error.h"

/*---------------------------------------------------------------------------
 * Utility Functions
 *---------------------------------------------------------------------------*/

/* CURRENT_OFFSET is the current index in the encoded data buffer */
#define CURRENT_OFFSET(pDecoder) ((pDecoder)->nOffsetStack[(pDecoder)->nOffsetStackIndex])

/* PARENT_OFFSET is the index of the parent decoder. It indicates the end offset of the
 * current decoder.
 */
#define PARENT_OFFSET(pDecoder) \
   ((pDecoder)->nOffsetStackIndex == 0 ? \
       (pDecoder)->nEncodedSize : \
       (pDecoder)->nOffsetStack[(pDecoder)->nOffsetStackIndex-1])


/*---------------------------------------------------------------------------
 * Decoding of basic integer types
 *---------------------------------------------------------------------------*/

/* Read an uint8_t after checking the buffer size.
 *
 * Return true on success and false if the decoder error state
 * was set on entry to the function or if there is not enough
 * space in the buffer for a uint8_t. In this case, the error
 * state is set to S_ERROR_BAD_FORMAT
 *
 */
static bool static_libBEFDecoderReadUint8(LIB_BEF_DECODER* pDecoder,
                                          OUT uint8_t* pnValue)
{
   assert(pDecoder != NULL);
   *pnValue = 0;

   if (pDecoder->nError != S_SUCCESS)
   {
      return false;
   }
   if (PARENT_OFFSET(pDecoder) - CURRENT_OFFSET(pDecoder) < 1)
   {
      /* Not enough room to decode a uint8_t */
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return false;
   }
   *pnValue = pDecoder->pEncodedData[CURRENT_OFFSET(pDecoder)];
   CURRENT_OFFSET(pDecoder)++;
   return true;
}

#ifndef INCLUDE_MINIMAL_BEF
/* Same as libBEFDecoderReadUint16 for uint16_t */
static bool static_libBEFDecoderReadUint16(LIB_BEF_DECODER* pDecoder,
                                         OUT uint16_t* pnValue)
{
   assert(pDecoder != NULL);
   *pnValue = 0;

   if (pDecoder->nError != S_SUCCESS)
   {
      return false;
   }
   if (PARENT_OFFSET(pDecoder) - CURRENT_OFFSET(pDecoder) < 2)
   {
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return false;
   }
   *pnValue =  (uint16_t)(    pDecoder->pEncodedData[CURRENT_OFFSET(pDecoder)]
                           | (pDecoder->pEncodedData[CURRENT_OFFSET(pDecoder)+1]<< 8));
   CURRENT_OFFSET(pDecoder) += 2;
   return true;
}
#endif /* INCLUDE_MINIMAL_BEF */

/* Same as libBEFDecoderReadUint32 for uint32_t */
static bool static_libBEFDecoderReadUint32(LIB_BEF_DECODER* pDecoder,
                                         OUT uint32_t* pnValue)
{
   assert(pDecoder != NULL);
   *pnValue = 0;

   if (pDecoder->nError != S_SUCCESS)
   {
      return false;
   }
   if (PARENT_OFFSET(pDecoder) - CURRENT_OFFSET(pDecoder) < 4)
   {
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return false;
   }
   *pnValue =   pDecoder->pEncodedData[CURRENT_OFFSET(pDecoder)]
            |  (pDecoder->pEncodedData[CURRENT_OFFSET(pDecoder)+1] << 8)
            |  (pDecoder->pEncodedData[CURRENT_OFFSET(pDecoder)+2] << 16)
            |  (pDecoder->pEncodedData[CURRENT_OFFSET(pDecoder)+3] << 24);
   CURRENT_OFFSET(pDecoder) += 4;
   return true;
}

/* Read a raw buffer of nValueLength bytes. Set them to zero on error */
static bool static_libBEFDecoderReadRaw(
    LIB_BEF_DECODER* pDecoder,
    IN uint32_t nValueLength,
    OUT uint8_t* pnValueBuffer)
{
   assert(pDecoder != NULL);

   if (pDecoder->nError != S_SUCCESS)
   {
      goto error;
   }
   if (PARENT_OFFSET(pDecoder) - CURRENT_OFFSET(pDecoder) < nValueLength)
   {
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      goto error;
   }
   memcpy(pnValueBuffer, &pDecoder->pEncodedData[CURRENT_OFFSET(pDecoder)], nValueLength);
   CURRENT_OFFSET(pDecoder) += nValueLength;
   return true;

error:
   memset(pnValueBuffer, 0x00, nValueLength);
   return false;
}

/*---------------------------------------------------------------------------
 * Decoder cache management
 *---------------------------------------------------------------------------*/

/*
 * Read the tag at the current position in the decoder buffer or from the
 * cached value in the decoder structure. Check that the tag is one of the
 * valid tags. Increment the current offset by one.
 *
 * Return false on error.
 */
static bool static_libBEFDecoderReadTag(LIB_BEF_DECODER* pDecoder, uint8_t* pnTag)
{
   assert(pDecoder != NULL);
   *pnTag = LIB_BEF_TAG_UNKNOWN;

   if (pDecoder->nError != S_SUCCESS)
   {
      return false;
   }

   if (pDecoder->nCachedTag != LIB_BEF_TAG_UNKNOWN)
   {
      /* We have already read a tag for this element. Reuse it from the cache,
         so that we guarantee a consistent view of the message structure.
         Don't forget to advance the current offset like if we had really read a uint8_t
       */
      *pnTag = pDecoder->nCachedTag;
      CURRENT_OFFSET(pDecoder)++;
      if (pDecoder->nCachedTag == LIB_BEF_TAG_INVALID)
      {
         /* This can happen if libBEFDecoderGetCurrentType was previously called
            and detected that the current element is ill-formed. Now set the
            error state.
         */
         pDecoder->nError = S_ERROR_BAD_FORMAT;
         return false;
      }
      else
      {
         return true;
      }
   }
   else
   {
      uint8_t nTag;
      /* Do read the tag */
      if (static_libBEFDecoderReadUint8(pDecoder, &nTag) == false)
      {
         /* We're at the end of the current sequence */
         return false;
      }
      /* Check that the tag is valid */
      switch (LIB_BEF_GET_BASE_TYPE(nTag))
      {
      case LIB_BEF_BASE_TYPE_COMPOSITE:
      case LIB_BEF_BASE_TYPE_SEQUENCE:
         /* Flag must be LIB_BEF_FLAG_VALUE_EXPLICIT */
         if (LIB_BEF_GET_FLAG(nTag) != LIB_BEF_FLAG_VALUE_EXPLICIT)
         {
            goto bad_format;
         }
         break;
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_BOOLEAN:
         /* Valid tag is either LIB_BEF_TAG_TRUE or LIB_BEF_TAG_FALSE
            or flag == LIB_BEF_FLAG_NULL_ARRAY or  LIB_BEF_FLAG_NON_NULL_ARRAY */
         if (nTag == LIB_BEF_TAG_TRUE || nTag == LIB_BEF_TAG_FALSE)
         {
            break;
         }
         if (LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_NULL_ARRAY ||
             LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_NON_NULL_ARRAY)
         {
            break;
         }
         goto bad_format;
#endif /* INCLUDE_MINIMAL_BEF */
      case LIB_BEF_BASE_TYPE_STRING:
         /* Allowed flags are: LIB_BEF_FLAG_NULL_VALUE, LIB_BEF_FLAG_NULL_ARRAY, LIB_BEF_FLAG_NON_NULL_ARRAY */
         if (LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_NULL_VALUE)
         {
            break;
         }
         /* FALLTHROUGH */
      case LIB_BEF_BASE_TYPE_UINT8:
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_UINT16:
#endif /* INCLUDE_MINIMAL_BEF */
      case LIB_BEF_BASE_TYPE_UINT32:
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_HANDLE:
#endif /* INCLUDE_MINIMAL_BEF */
         /* Allowed flags are: LIB_BEF_FLAG_VALUE_EXPLICIT, LIB_BEF_FLAG_NULL_ARRAY, LIB_BEF_FLAG_NON_NULL_ARRAY */
         if (LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_VALUE_EXPLICIT ||
             LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_NULL_ARRAY ||
             LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_NON_NULL_ARRAY)
         {
            break;
         }
         goto bad_format;
      default:
         /* Invalid base type */
         goto bad_format;
      }
      *pnTag = nTag;
      pDecoder->nCachedTag = nTag;
      return true;
   }

bad_format:
   pDecoder->nError = S_ERROR_BAD_FORMAT;
   return false;
}

/*
 * Read the subtag at the current position in the decoder buffer or from the
 * cached value in the decoder structure. Check that the subtag is one of the
 * valid subtags. Increment the current offset by one.
 *
 * Return false on error.
 */
static bool static_libBEFDecoderReadSubTag(LIB_BEF_DECODER* pDecoder, uint8_t* pnSubTag)
{
   assert(pDecoder != NULL);
   *pnSubTag = LIB_BEF_TAG_UNKNOWN;

   if (pDecoder->nError != S_SUCCESS)
   {
      return false;
   }

   if (pDecoder->nCachedSubTag != LIB_BEF_TAG_UNKNOWN)
   {
      /* We have already read a subtag for this element. Reuse it from the cache,
         so that we guarantee a consistent view of the message structure.
         Don't forget to advance the current offset like if we had really read a uint8_t
       */
      *pnSubTag = pDecoder->nCachedSubTag;
      CURRENT_OFFSET(pDecoder)++;
      return true;
   }
   else
   {
      uint8_t nSubTag;
      /* Do read the subtag */
      if (static_libBEFDecoderReadUint8(pDecoder, &nSubTag) == false)
      {
         /* We're at the end of the current sequence */
         return false;
      }
      /* Check that the subtag is valid */
      switch (nSubTag)
      {
      case LIB_BEF_SUBTAG_MEM_REF:
      case LIB_BEF_SUBTAG_UUID:
         break;
      default:
         /* Invalid subtag */
         goto bad_format;
      }
      *pnSubTag = nSubTag;
      pDecoder->nCachedSubTag = nSubTag;
      return true;
   }

bad_format:
   pDecoder->nError = S_ERROR_BAD_FORMAT;
   return false;
}

/* Read the first or second cached words */
static bool static_libBEFDecoderReadCachedWord(
   LIB_BEF_DECODER* pDecoder,
   uint32_t* pnWord,
   uint32_t nIndex)
{
   *pnWord = LIB_BEF_NULL_ELEMENT;

   if (pDecoder->nError != S_SUCCESS)
   {
      return false;
   }

   if (pDecoder->nCachedWords[nIndex] != LIB_BEF_NULL_ELEMENT)
   {
      /* We have already read a value for this element. Reuse it from the cache,
         so that we guarantee a consistent view of the message structure.
         Don't forget to advance the current offset like if we had really read a uint32_t
       */
      *pnWord = pDecoder->nCachedWords[nIndex];
      CURRENT_OFFSET(pDecoder) += 4;
      return true;
   }
   else
   {
      uint32_t nWord;
      if (static_libBEFDecoderReadUint32(pDecoder, &nWord) == false)
      {
         /* We're at the end the current sequence */
         return false;
      }
      else if (nWord == LIB_BEF_NULL_ELEMENT)
      {
         /* The message contains 0xFFFFFFFF in place of the word.
            This is necessarily a bad format because the corresponding
            array/string encessarily overflows the whole message.
            And it's important not to mix up with the LIB_BEF_NULL_ELEMENT
            used to denote the absence of a cached value...
            */
         pDecoder->nError = S_ERROR_BAD_FORMAT;
         return false;
      }
      else
      {
         pDecoder->nCachedWords[nIndex] = nWord;
         *pnWord = nWord;
         return true;
      }
   }
}

static void static_libBEFDecoderInvalidateCache(LIB_BEF_DECODER* pDecoder)
{
   pDecoder->nCachedTag      = LIB_BEF_TAG_UNKNOWN;
   pDecoder->nCachedSubTag   = LIB_BEF_TAG_UNKNOWN;
   pDecoder->nCachedWords[0] = LIB_BEF_NULL_ELEMENT;
   pDecoder->nCachedWords[1] = LIB_BEF_NULL_ELEMENT;
}

/*-----------------------------------------------------------------
 * API Implementation
 *-----------------------------------------------------------------*/
void libBEFDecoderInit(
      OUT LIB_BEF_DECODER* pDecoder)
{
   assert(pDecoder != NULL);

   memset(pDecoder->nOffsetStack, 0x00, sizeof(pDecoder->nOffsetStack));
   pDecoder->nOffsetStackIndex = 0;
   pDecoder->nError = S_SUCCESS;
   static_libBEFDecoderInvalidateCache(pDecoder);
}

bool libBEFDecoderHasData(
      LIB_BEF_DECODER* pDecoder)
{
   assert(pDecoder != NULL);

   if (pDecoder->nError != S_SUCCESS)
   {
      return false;
   }
   return CURRENT_OFFSET(pDecoder) < PARENT_OFFSET(pDecoder);
}

uint32_t libBEFDecoderGetCurrentType(
      IN OUT LIB_BEF_DECODER* pDecoder,
      OUT uint32_t* pnArrayLength)
{
   uint32_t nOriginOffset;
   uint8_t nTag;
   uint8_t nSubTag;
   uint32_t nType;
   uint32_t nArrayLength;

   assert(pDecoder != NULL);

   nType = LIB_BEF_TYPE_NO_DATA;
   nArrayLength = LIB_BEF_NULL_ELEMENT;

   nOriginOffset = CURRENT_OFFSET(pDecoder);

   if(!libBEFDecoderHasData(pDecoder))
   {
      if(pnArrayLength != NULL)
      {
         *pnArrayLength = nArrayLength;
      }
      return nType;
   }

   if(static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      goto end;
   }

   switch(nTag)
   {
   case LIB_BEF_TAG_COMPOSITE:
      if (static_libBEFDecoderReadSubTag(pDecoder, &nSubTag) == false)
      {
         goto end;
      }
      nType = (nSubTag << 8)|nTag;
      break;

   case LIB_BEF_TAG_TRUE:
   case LIB_BEF_TAG_FALSE:
      nType = LIB_BEF_TYPE_BOOLEAN;
      break;

   case (LIB_BEF_BASE_TYPE_UINT8   |LIB_BEF_FLAG_VALUE_EXPLICIT):
#ifndef INCLUDE_MINIMAL_BEF
   case (LIB_BEF_BASE_TYPE_UINT16  |LIB_BEF_FLAG_VALUE_EXPLICIT):
#endif /* INCLUDE_MINIMAL_BEF */
   case (LIB_BEF_BASE_TYPE_UINT32  |LIB_BEF_FLAG_VALUE_EXPLICIT):
   case (LIB_BEF_BASE_TYPE_SEQUENCE|LIB_BEF_FLAG_VALUE_EXPLICIT):
      nType = nTag;
      break;

   case (LIB_BEF_BASE_TYPE_STRING|LIB_BEF_FLAG_VALUE_EXPLICIT):
   case (LIB_BEF_BASE_TYPE_STRING|LIB_BEF_FLAG_NULL_VALUE):
      nType = LIB_BEF_TYPE_STRING;
      break;

   default:
      /* Otherwise, the element is an array */
      if (LIB_BEF_GET_FLAG(nTag) != LIB_BEF_FLAG_NULL_ARRAY)
      {
         /* Non-NULL array. Check its length */
         static_libBEFDecoderReadCachedWord(pDecoder, &nArrayLength, 0);
      }
      if(pDecoder->nError == S_SUCCESS)
      {
         nType = LIB_BEF_GET_BASE_TYPE(nTag)|0x0000000E;
      }
      break;
   }

end:
   if(pDecoder->nError != S_SUCCESS)
   {
      /* Something bad happened, as this function must not affect the decoder
         error state, reset it but save the information using special values in
         the cache. */
      pDecoder->nError        = S_SUCCESS;
      pDecoder->nCachedTag    = LIB_BEF_TAG_INVALID;
      pDecoder->nCachedSubTag = LIB_BEF_SUBTAG_INVALID;
   }

   /* rewind the decoder to the initial position */
   CURRENT_OFFSET(pDecoder) = nOriginOffset;

   if(pnArrayLength != NULL)
   {
      *pnArrayLength = nArrayLength;
   }
   return nType;
}

void libBEFDecoderSkip(
      LIB_BEF_DECODER* pDecoder)
{
   uint32_t nSkipBytes;
   uint8_t  nTag;

   assert(pDecoder != NULL);

   if(static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return;
   }

   if (LIB_BEF_GET_BASE_TYPE(nTag) == LIB_BEF_BASE_TYPE_COMPOSITE)
   {
      /* All composite types (UUID and MemRef) have 17 bytes following the tag
         (one byte of sub-tag and 16 bytes of payload) */
      nSkipBytes = 17;
      goto doSkip;
   }

   if (nTag == (LIB_BEF_BASE_TYPE_STRING | LIB_BEF_FLAG_VALUE_EXPLICIT))
   {
      /* non-NULL String */
      if (static_libBEFDecoderReadCachedWord(pDecoder, &nSkipBytes, 0) == false)
      {
         return;
      }
      goto doSkip;
   }

   if (nTag == LIB_BEF_TAG_SEQUENCE)
   {
      /* Sequence element */
      if (static_libBEFDecoderReadCachedWord(pDecoder, &nSkipBytes, 0) == false)
      {
        return;
      }
      goto doSkip;
   }

   if (LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_NULL_ARRAY ||
       LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_NULL_VALUE)
   {
      /* A NULL value. Nothing more to skip than the tag */
      nSkipBytes = 0;
      goto doSkip;
   }

   if (LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_NON_NULL_ARRAY)
   {
      /* non-NULL array */
      uint32_t nArrayCount;

      if (static_libBEFDecoderReadCachedWord(pDecoder, &nArrayCount, 0) == false)
      {
         return;
      }
      switch (LIB_BEF_GET_BASE_TYPE(nTag))
      {
      case LIB_BEF_BASE_TYPE_STRING:
         /* Read the total length in bytes of the string array */
         if (static_libBEFDecoderReadCachedWord(pDecoder, &nSkipBytes, 1) == false)
         {
            return;
         }
         break;
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_BOOLEAN:
#endif /* INCLUDE_MINIMAL_BEF */
      case LIB_BEF_BASE_TYPE_UINT8:
         nSkipBytes = nArrayCount;
         break;
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_UINT16:
         /* We're going to multiply nArrayCount by 2. Protect against overflow */
         if ((nArrayCount & 0x80000000) != 0)
         {
            pDecoder->nError = S_ERROR_BAD_FORMAT;
            return;
         }
         nSkipBytes = nArrayCount << 1;
         break;
#endif /* INCLUDE_MINIMAL_BEF */
      case LIB_BEF_BASE_TYPE_UINT32:
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_HANDLE:
#endif /* INCLUDE_MINIMAL_BEF */
         /* We're going to multiply nArrayCount by 4. Protect against overflow */
         if ((nArrayCount & 0xC0000000) != 0)
         {
            pDecoder->nError = S_ERROR_BAD_FORMAT;
            return;
         }
         nSkipBytes = nArrayCount << 2;
         break;
      }
   }
   else
   {
      /* Base Type */
      switch (LIB_BEF_GET_BASE_TYPE(nTag))
      {
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_BOOLEAN:
         nSkipBytes = 0; /* The value is encoded in the tag */
         break;
#endif /* INCLUDE_MINIMAL_BEF */
      case LIB_BEF_BASE_TYPE_UINT8:
         nSkipBytes = 1;
         break;
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_UINT16:
         nSkipBytes = 2;
         break;
#endif /* INCLUDE_MINIMAL_BEF */
      case LIB_BEF_BASE_TYPE_UINT32:
#ifndef INCLUDE_MINIMAL_BEF
      case LIB_BEF_BASE_TYPE_HANDLE:
#endif /* INCLUDE_MINIMAL_BEF */
         nSkipBytes = 4;
         break;
      }
   }

doSkip:

   if (PARENT_OFFSET(pDecoder) - CURRENT_OFFSET(pDecoder) < nSkipBytes)
   {
      /* The current element is too large to fit in the current sequence */
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return;
   }

   CURRENT_OFFSET(pDecoder) += nSkipBytes;
   static_libBEFDecoderInvalidateCache(pDecoder);
}

uint32_t libBEFDecoderReadArrayLength(
      LIB_BEF_DECODER* pDecoder)
{
   uint8_t nTag;
   uint32_t nOriginOffset;
   uint32_t nArrayCount;
   uint32_t nArrayByteLength;

   nOriginOffset = CURRENT_OFFSET(pDecoder);

   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return LIB_BEF_NULL_ELEMENT;
   }
   if (LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_NULL_ARRAY)
   {
      /* NULL array */
      nArrayCount = LIB_BEF_NULL_ELEMENT;
      goto end;
   }
   else if (LIB_BEF_GET_FLAG(nTag) != LIB_BEF_FLAG_NON_NULL_ARRAY)
   {
      /* this is not an array */
      goto bad_format;
   }

   /* Non-NULL array. Check its length */
   if (static_libBEFDecoderReadCachedWord(pDecoder, &nArrayCount, 0) == false)
   {
      return LIB_BEF_NULL_ELEMENT;
   }

   /* Check that the array doesn't overflow the current sequence */
   switch (LIB_BEF_GET_BASE_TYPE(nTag))
   {
#ifndef INCLUDE_MINIMAL_BEF
   case LIB_BEF_BASE_TYPE_BOOLEAN:
#endif /* INCLUDE_MINIMAL_BEF */
   case LIB_BEF_BASE_TYPE_UINT8:
      /* one byte per element */
      nArrayByteLength = nArrayCount;
      break;
#ifndef INCLUDE_MINIMAL_BEF
   case LIB_BEF_BASE_TYPE_UINT16:
      /* Two bytes per element. Mind for overflow! */
      if ((nArrayCount & 0x80000000) != 0)
      {
         goto bad_format;
      }
      nArrayByteLength = nArrayCount << 1;
      break;
#endif /* INCLUDE_MINIMAL_BEF */
   case LIB_BEF_BASE_TYPE_UINT32:
#ifndef INCLUDE_MINIMAL_BEF
   case LIB_BEF_BASE_TYPE_HANDLE:
#endif /* INCLUDE_MINIMAL_BEF */
      /* Four bytes per element. Mind for overflow! */
      if ((nArrayCount & 0xC0000000) != 0)
      {
         goto bad_format;
      }
      nArrayByteLength = nArrayCount << 2;
      break;
   case LIB_BEF_BASE_TYPE_STRING:
      /* String array, the total length in bytes is in the following word */
      if (static_libBEFDecoderReadCachedWord(pDecoder, &nArrayByteLength, 1) == false)
      {
         return LIB_BEF_NULL_ELEMENT;
      }
      /* Sanity check: they may be at most nArrayByteLength strings in the array
         (this is if all strings are NULL) */
      if (nArrayCount > nArrayByteLength)
      {
         goto bad_format;
      }
      break;
   }

   /* Check that the array bytes fit in the current sequence */
   if (PARENT_OFFSET(pDecoder) - CURRENT_OFFSET(pDecoder) < nArrayByteLength)
   {
      goto bad_format;
   }

end:
   /* rewind the decoder to the initial position */
   CURRENT_OFFSET(pDecoder) = nOriginOffset;

   return nArrayCount;

bad_format:
   /* rewind the decoder to the initial position */
   CURRENT_OFFSET(pDecoder) = nOriginOffset;

   pDecoder->nError = S_ERROR_BAD_FORMAT;
   return LIB_BEF_NULL_ELEMENT;
}

/*-----------------------------------------------------------------
 * Simple reads
 *-----------------------------------------------------------------*/
#ifndef INCLUDE_MINIMAL_BEF
bool libBEFDecoderReadBoolean(
      LIB_BEF_DECODER* pDecoder)
{
   uint8_t nTag;

   /* Read the tag */
   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return false;
   }

   if (nTag != LIB_BEF_TAG_TRUE && nTag != LIB_BEF_TAG_FALSE)
   {
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return false;
   }
   static_libBEFDecoderInvalidateCache(pDecoder);
   return (bool)nTag;
}
#endif /* INCLUDE_MINIMAL_BEF */

uint8_t libBEFDecoderReadUint8(
      LIB_BEF_DECODER* pDecoder)
{
   uint8_t nValue;
   uint8_t nTag;

   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return 0;
   }
   /* Check type */
   if (nTag != (LIB_BEF_BASE_TYPE_UINT8 | LIB_BEF_FLAG_VALUE_EXPLICIT))
   {
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return 0;
   }
   /* Read the value */
   static_libBEFDecoderReadUint8(pDecoder, &nValue);
   static_libBEFDecoderInvalidateCache(pDecoder);
   return nValue;
}

#ifndef INCLUDE_MINIMAL_BEF
uint16_t libBEFDecoderReadUint16(
      LIB_BEF_DECODER* pDecoder)
{
   uint16_t nValue;
   uint8_t nTag;

   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return 0;
   }
   /* Check type */
   if (nTag != (LIB_BEF_BASE_TYPE_UINT16 | LIB_BEF_FLAG_VALUE_EXPLICIT))
   {
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return 0;
   }
   /* Read the value */
   static_libBEFDecoderReadUint16(pDecoder, &nValue);
   static_libBEFDecoderInvalidateCache(pDecoder);
   return nValue;
}
#endif /* INCLUDE_MINIMAL_BEF */

uint32_t libBEFDecoderReadUint32(
      LIB_BEF_DECODER* pDecoder)
{
   uint32_t nValue;
   uint8_t nTag;

   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return 0;
   }
   /* Check type */
   if (nTag != (LIB_BEF_BASE_TYPE_UINT32 | LIB_BEF_FLAG_VALUE_EXPLICIT))
   {
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return 0;
   }
   /* Read the value */
   static_libBEFDecoderReadUint32(pDecoder, &nValue);
   static_libBEFDecoderInvalidateCache(pDecoder);
   return nValue;
}

#ifndef INCLUDE_MINIMAL_BEF
S_HANDLE libBEFDecoderReadHandle(
      LIB_BEF_DECODER* pDecoder)
{
   S_HANDLE nValue;
   uint8_t nTag;

   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return 0;
   }
   /* Check type */
   if (nTag != (LIB_BEF_BASE_TYPE_HANDLE | LIB_BEF_FLAG_VALUE_EXPLICIT))
   {
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return 0;
   }
   /* Read the value */
   static_libBEFDecoderReadUint32(pDecoder, &nValue);
   static_libBEFDecoderInvalidateCache(pDecoder);
   return nValue;
}
#endif /* INCLUDE_MINIMAL_BEF */

/*-----------------------------------------------------------------
 * Simple arrays
 *-----------------------------------------------------------------*/
/*
 * Copy an array of a given type (except Strings).
 * Used to factorize code.
 * Usage is the same than libBEFDecoderCopyXXXArray.
 * nArrayType indicates the expected type (ex: LIB_BEF_BASE_TYPE_BOOLEAN)
 */
static uint32_t static_libBEFDecoderCopyArray(
      LIB_BEF_DECODER* pDecoder,
      uint32_t nIndex,
      uint32_t nMaxLength,
      uint8_t nArrayBaseType,
      OUT void* pArray)
{
   uint32_t nArrayCount;
   uint32_t nArrayTotalByteCount = 0;
   uint32_t nSkipSize = 0;
   uint32_t nOriginOffset;
   uint8_t nTag;
   uint32_t nResult;

   assert(pDecoder != NULL);

   nOriginOffset = CURRENT_OFFSET(pDecoder);

   /* Get the tag of the current element (possibly from the cache) */
   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return LIB_BEF_NULL_ELEMENT;
   }

   if (nArrayBaseType == LIB_BEF_BASE_TYPE_STRING)
   {
      if (   (LIB_BEF_GET_BASE_TYPE(nTag) != nArrayBaseType)
          || (LIB_BEF_GET_FLAG(nTag) != LIB_BEF_FLAG_NULL_ARRAY &&
              LIB_BEF_GET_FLAG(nTag) != LIB_BEF_FLAG_VALUE_EXPLICIT))
      {
         /* Not an array of the correct type */
         goto bad_format;
      }
   }
   else
   {
      if (   (LIB_BEF_GET_BASE_TYPE(nTag) != nArrayBaseType)
          || (LIB_BEF_GET_FLAG(nTag) != LIB_BEF_FLAG_NULL_ARRAY &&
              LIB_BEF_GET_FLAG(nTag) != LIB_BEF_FLAG_NON_NULL_ARRAY))
      {
         /* Not an array of the correct type */
         goto bad_format;
      }
   }

   if (LIB_BEF_GET_FLAG(nTag) == LIB_BEF_FLAG_NULL_ARRAY)
   {
      /* NULL array. */
      nResult = LIB_BEF_NULL_ELEMENT;
      goto copy_array_end;
   }

   /* Get the number of elements in the array (possibly from the cache) */
   if (static_libBEFDecoderReadCachedWord(pDecoder, &nArrayCount, 0) == false)
   {
      return LIB_BEF_NULL_ELEMENT;
   }

   if (nIndex >= nArrayCount)
   {
      /* Nothing to copy */
      nResult = 0;
      goto copy_array_end;
   }

   /* The subtraction nArrayCount - nIndex cannot underflow because
      we know that nIndex >= nArrayCount */
   if (nMaxLength > nArrayCount - nIndex)
   {
      nMaxLength = nArrayCount - nIndex;
   }
   nResult = nMaxLength;

   /* Check that nArrayCount won't overflow the current sequence */
   switch (nArrayBaseType)
   {
#ifndef INCLUDE_MINIMAL_BEF
   case LIB_BEF_BASE_TYPE_BOOLEAN:
#endif /* INCLUDE_MINIMAL_BEF */
#ifdef EXCLUDE_UTF8
   case LIB_BEF_BASE_TYPE_STRING: /* string encoded as byte arrays */
#endif /* INCLUDE_MINIMAL_BEF */
   case LIB_BEF_BASE_TYPE_UINT8:
      /* Each element has 1 byte */
      nArrayTotalByteCount = nArrayCount;
      nSkipSize = nIndex;
      break;
#ifndef INCLUDE_MINIMAL_BEF
   case LIB_BEF_BASE_TYPE_UINT16:
      /* Mind the overflow! */
      if ((nArrayCount & 0x80000000) != 0)
      {
         goto bad_format;
      }
      nArrayTotalByteCount = nArrayCount << 1;
      nSkipSize = nIndex << 1;
      break;
#endif /* INCLUDE_MINIMAL_BEF */
   case LIB_BEF_BASE_TYPE_UINT32:
#ifndef INCLUDE_MINIMAL_BEF
   case LIB_BEF_BASE_TYPE_HANDLE:
#endif /* INCLUDE_MINIMAL_BEF */
   default:
      /* Mind the overflow! */
      if ((nArrayCount & 0xC0000000) != 0)
      {
         goto bad_format;
      }
      nArrayTotalByteCount = nArrayCount << 2;
      nSkipSize = nIndex << 2;
      break;
   }
   if (PARENT_OFFSET(pDecoder) - CURRENT_OFFSET(pDecoder) < nArrayTotalByteCount)
   {
      /* overflow */
      goto bad_format;
   }

   CURRENT_OFFSET(pDecoder) += nSkipSize;

   if(nArrayBaseType == LIB_BEF_BASE_TYPE_UINT8
#ifdef EXCLUDE_UTF8
      ||nArrayBaseType == LIB_BEF_BASE_TYPE_STRING /* strings encoded as byte array */
#endif /* EXCLUDE_UTF8 */
      )
   {
      /* Special case: we can do a mere copy */
      memcpy(pArray, &pDecoder->pEncodedData[CURRENT_OFFSET(pDecoder)], nMaxLength);
   }
   else
   {
      uint32_t nTmpIndex;
      for (nTmpIndex = 0; nTmpIndex < nMaxLength; nTmpIndex++)
      {
         /* Read the value */
         switch (nArrayBaseType)
         {
#ifndef INCLUDE_MINIMAL_BEF
         case LIB_BEF_BASE_TYPE_BOOLEAN:
         {
            uint8_t nElement;
            static_libBEFDecoderReadUint8(pDecoder, &nElement);
            if (!(nElement == LIB_BEF_TAG_TRUE || nElement == LIB_BEF_TAG_FALSE))
            {
               goto bad_format;
            }
            *((bool*)pArray) = (bool)nElement;
            pArray = ((bool*)pArray) + 1;
            break;
         }
         case LIB_BEF_BASE_TYPE_UINT16:
            static_libBEFDecoderReadUint16(pDecoder, (uint16_t*)pArray);
            pArray = ((uint16_t*)pArray) + 1;
            break;
         case LIB_BEF_BASE_TYPE_HANDLE:
#endif /* INCLUDE_MINIMAL_BEF */
         case LIB_BEF_BASE_TYPE_UINT32:
            static_libBEFDecoderReadUint32(pDecoder, (uint32_t*)pArray);
            pArray = ((uint32_t*)pArray) + 1;
            break;
         }
      }
   }

copy_array_end:
   CURRENT_OFFSET(pDecoder) = nOriginOffset;
   return nResult;

bad_format:
   pDecoder->nError = S_ERROR_BAD_FORMAT;
   return LIB_BEF_NULL_ELEMENT;
}


uint32_t libBEFDecoderCopyUint8Array(
      LIB_BEF_DECODER* pDecoder,
      uint32_t nIndex,
      uint32_t nMaxLength,
      OUT uint8_t* pnArray)
{
   return static_libBEFDecoderCopyArray(
      pDecoder,
      nIndex,
      nMaxLength,
      LIB_BEF_BASE_TYPE_UINT8,
      pnArray);
}


#ifndef INCLUDE_MINIMAL_BEF
uint32_t libBEFDecoderCopyUint16Array(
      LIB_BEF_DECODER* pDecoder,
      uint32_t nIndex,
      uint32_t nMaxLength,
      OUT uint16_t* pnArray)
{
   return static_libBEFDecoderCopyArray(
      pDecoder,
      nIndex,
      nMaxLength,
      LIB_BEF_BASE_TYPE_UINT16,
      pnArray);
}


uint32_t libBEFDecoderCopyUint32Array(
      LIB_BEF_DECODER* pDecoder,
      uint32_t nIndex,
      uint32_t nMaxLength,
      OUT uint32_t* pnArray)
{
   return static_libBEFDecoderCopyArray(
      pDecoder,
      nIndex,
      nMaxLength,
      LIB_BEF_BASE_TYPE_UINT32,
      pnArray);
}

uint32_t libBEFDecoderCopyHandleArray(
      LIB_BEF_DECODER* pDecoder,
      uint32_t nIndex,
      uint32_t nMaxLength,
      OUT S_HANDLE* phArray)
{
   return static_libBEFDecoderCopyArray(
      pDecoder,
      nIndex,
      nMaxLength,
      LIB_BEF_BASE_TYPE_HANDLE,
      phArray);
}
#endif

#ifndef EXCLUDE_UTF8
/*-----------------------------------------------------------------
 * Strings and UTF8<->wchar_t conversions
 *-----------------------------------------------------------------*/

/**
 * Generic UTF-8 decoding function. This function reads and checks a UTF-8 string
 * and performs one or more of the following actions:
 * - copy the UTF-8 bytes into a buffer;
 * - count the number of wchar_t necessary to represent the Unicode string
 * - copy the Unicode characters into a wchar_t buffer
 *
 * The UTF-8 checks are the following:
 * - no UTF-8 sequence extends beyond the end of buffer
 * - no character value is beyond 0x110000
 * - no character value is between 0xD800 and 0xDFFF (those values are reserved for UTF-16 surrogates)
 * - there is no NUL (U+0000) character
 * - there is no overlong sequence
 *
 * Note on UTF-8->wchar_t conversion:
 * - If wchar_t has more than 2 bytes, we assume a UTF-32 encoding
 *  (one-to-one mapping between the Unicode character value and the wchar_t)
 * - If wchar_t has 2 bytes, we assume a UTF-16 encoding and use surrogate
 *    pairs for Unicode characters above 0x10000
 *
 * Returns the number of items (UTF-8 bytes or wchar_t elements) actually written
 * in pOutput or LIB_BEF_NULL_ELEMENT if the element was a NULL string
 **/
static uint32_t static_libBEFDecoderProcessUTF8String(
         LIB_BEF_DECODER* pDecoder,
         uint32_t nMaxOffset,
         bool bCopyToWChar,
         bool bUseCache,
         void* pOutput)
{
   uint8_t  nTag;
   uint32_t nSrcUTF8Length;
   uint32_t nCurrentChar = 0;
   uint8_t  nCurrentSequenceCount = 0;
   uint32_t nItemCount = 0;
   uint8_t* pSrcUTF8;
   uint8_t* pSrcUTF8End;
   wchar_t* pWCharOutput = NULL;
   char* pCharOutput = NULL;

   if (bCopyToWChar)
   {
      pWCharOutput = (wchar_t*)pOutput;
   }
   else
   {
      pCharOutput = (char*)pOutput;
   }

   if (bUseCache)
   {
      if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
      {
         return LIB_BEF_NULL_ELEMENT;
      }
   }
   else
   {
      if (static_libBEFDecoderReadUint8(pDecoder, &nTag) == false)
      {
         return LIB_BEF_NULL_ELEMENT;
      }
   }
   if (nTag == (LIB_BEF_BASE_TYPE_STRING | LIB_BEF_FLAG_NULL_VALUE))
   {
      /* NULL String */
      return LIB_BEF_NULL_ELEMENT;
   }
   if (nTag != (LIB_BEF_BASE_TYPE_STRING | LIB_BEF_FLAG_VALUE_EXPLICIT))
   {
      /* Not a String */
      goto bad_format;
   }
   /* We now know it's a non-NULL String */
   if (bUseCache)
   {
      if (static_libBEFDecoderReadCachedWord(pDecoder, &nSrcUTF8Length, 0) == false)
      {
         return LIB_BEF_NULL_ELEMENT;
      }
   }
   else
   {
      if (static_libBEFDecoderReadUint32(pDecoder, &nSrcUTF8Length) == false)
      {
         return LIB_BEF_NULL_ELEMENT;
      }
   }

   if (nMaxOffset - CURRENT_OFFSET(pDecoder) < nSrcUTF8Length)
   {
      /* The string overflows the maximum offset */
      goto bad_format;
   }

   pSrcUTF8 = pDecoder->pEncodedData + CURRENT_OFFSET(pDecoder);
   pSrcUTF8End = pSrcUTF8 + nSrcUTF8Length;
   CURRENT_OFFSET(pDecoder) += nSrcUTF8Length;

   while (pSrcUTF8 < pSrcUTF8End)
   {
      uint32_t x;
      x = *pSrcUTF8++;
      if ((x & 0x80) == 0)
      {
         if (nCurrentSequenceCount != 0)
         {
            /* We were expecting a 10xxxxxx byte: ill-formed UTF-8 */
            goto bad_format;
         }
         else if (x == 0)
         {
            /* The null character is forbidden */
            goto bad_format;
         }
         /* We have a well-formed Unicode character */
         nCurrentChar = x;
      }
      else if ((x & 0xC0) == 0xC0)
      {
         /* Start of a sequence */
         if (nCurrentSequenceCount != 0)
         {
            /* We were expecting a 10xxxxxx byte: ill-formed UTF-8 */
            goto bad_format;
         }
         else if ((x & 0xE0) == 0xC0)
         {
            /* 1 byte follows */
            nCurrentChar = x & 0x1F;
            nCurrentSequenceCount = 1;
            if ((x & 0x1E) == 0)
            {
               /* Illegal UTF-8: overlong encoding of character in the [0x00-0x7F] range
                  (must use 1-byte encoding, not a 2-byte encoding) */
               goto bad_format;
            }
         }
         else if ((x & 0xF0) == 0xE0)
         {
            /* 2 bytes follow */
            nCurrentChar = x & 0x0F;
            nCurrentSequenceCount = 2;
         }
         else if ((x & 0xF8) == 0xF0)
         {
            /* 3 bytes follow */
            nCurrentChar = x & 0x07;
            nCurrentSequenceCount = 3;
         }
         else
         {
            /* Illegal start of sequence */
            goto bad_format;
         }
      }
      else if ((x & 0xC0) == 0x80)
      {
         /* Continuation byte */
         if (nCurrentSequenceCount == 0)
         {
            /* We were expecting a sequence start, not a continuation byte */
            goto bad_format;
         }
         else
         {
            if (nCurrentSequenceCount == 2)
            {
               /* We're in a 3-byte sequence, check that we're not using an overlong sequence */
               if (nCurrentChar == 0 && (x & 0x20) == 0)
               {
                  /* The character starts with at least 5 zero bits, so has fewer than 11 bits. It should
                     have used a 2-byte sequence, not a 3-byte sequence */
                  goto bad_format;
               }
            }
            else if (nCurrentSequenceCount == 3)
            {
               if (nCurrentChar == 0 && (x & 0x30) == 0)
               {
                  /* The character starts with at least 5 zero bits, so has fewer than 16 bits. It should
                     have used a 3-byte sequence, not a 4-byte sequence */
                  goto bad_format;
               }
            }
            nCurrentSequenceCount--;
            nCurrentChar = (nCurrentChar << 6) | (x & 0x3F);
         }
      }
      else
      {
         /* Illegal byte */
         goto bad_format;
      }
      if (nCurrentSequenceCount == 0)
      {
         /* nCurrentChar contains the current Unicode character */
         /* check character */
         if ((nCurrentChar >= 0xD800 && nCurrentChar < 0xE000) || nCurrentChar >= 0x110000)
         {
            /* Illegal code point */
            goto bad_format;
         }
         if (bCopyToWChar)
         {
#if WCHAR_MAX > 0xFFFF
            /* Assume UTF-32 encoding */
            if (pWCharOutput != NULL)
            {
               *pWCharOutput++ = (wchar_t)nCurrentChar;
            }
            nItemCount++;
#else
            /* Assume UTF-16 encoding */
            if (nCurrentChar >= 0x10000)
            {
               /* Use two 16-bit items */
               if (pWCharOutput != NULL)
               {
                  *pWCharOutput++ = (wchar_t)(0xD800 | ((nCurrentChar - 0x10000) >> 10));
                  *pWCharOutput++ = (wchar_t)(0xDC00 | ((nCurrentChar - 0x10000) & 0x3FF));
               }
               nItemCount += 2;
            }
            else
            {
               /* Use one 16-bit item */
               if (pWCharOutput != NULL)
               {
                  *pWCharOutput++ = (wchar_t)nCurrentChar;
               }
               nItemCount++;
            }
#endif
         }
      }
      if (!bCopyToWChar)
      {
         if (pCharOutput != NULL)
         {
            *pCharOutput++ = x;
         }
         nItemCount++;
      }
   }

   if (nCurrentSequenceCount != 0)
   {
      /* UTF-8 buffer ends in the middle of a sequence. That's invalid UTF-8 */
      goto bad_format;
   }

   /* Add final zero */
   if (bCopyToWChar)
   {
      if (pWCharOutput != NULL)
      {
         *pWCharOutput++ = (wchar_t)0;
      }
      nItemCount++;
   }
   else
   {
      if (pCharOutput != NULL)
      {
         *pCharOutput++ = (uint8_t)0;
      }
      nItemCount++;
   }

   return nItemCount;

bad_format:
   pDecoder->nError = S_ERROR_BAD_FORMAT;
   return LIB_BEF_NULL_ELEMENT;
}
#endif /* INCLUDE_MINIMAL_BEF */

uint32_t libBEFDecoderReadStringLength(
      LIB_BEF_DECODER* pDecoder)
{
   uint32_t nStringLength;
   uint32_t nOriginOffset;
   uint8_t  nTag;

   nOriginOffset = CURRENT_OFFSET(pDecoder);

   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return LIB_BEF_NULL_ELEMENT;
   }
   switch (nTag)
   {
   case LIB_BEF_BASE_TYPE_STRING | LIB_BEF_FLAG_NULL_VALUE:
      /* NULL String */
      nStringLength = LIB_BEF_NULL_ELEMENT;
      break;
   case LIB_BEF_BASE_TYPE_STRING | LIB_BEF_FLAG_VALUE_EXPLICIT:
      /* Non-NULL String */
      if (static_libBEFDecoderReadCachedWord(pDecoder, &nStringLength, 0) == false)
      {
         return LIB_BEF_NULL_ELEMENT;
      }
      nStringLength++; /* For the terminal zero */
      break;
   default:
      /* Not a string */
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return LIB_BEF_NULL_ELEMENT;
   }
   CURRENT_OFFSET(pDecoder) = nOriginOffset;
   return nStringLength;
}

#ifndef EXCLUDE_UTF8
uint32_t libBEFDecoderCopyStringAsUTF8(
      LIB_BEF_DECODER* pDecoder,
      OUT char* pString)
{
   uint32_t nOriginOffset;
   uint32_t nResult;

   nOriginOffset = CURRENT_OFFSET(pDecoder);

   nResult = static_libBEFDecoderProcessUTF8String(
      pDecoder,
      PARENT_OFFSET(pDecoder),
      false,
      true,
      pString);

   CURRENT_OFFSET(pDecoder) = nOriginOffset;
   return nResult;
}

uint32_t libBEFDecoderReadWStringLength(
      LIB_BEF_DECODER* pDecoder)
{
   uint32_t nOriginOffset;
   uint32_t nResult;

   nOriginOffset = CURRENT_OFFSET(pDecoder);

   nResult = static_libBEFDecoderProcessUTF8String(
      pDecoder,
      PARENT_OFFSET(pDecoder),
      true,
      true,
      NULL);

   CURRENT_OFFSET(pDecoder) = nOriginOffset;
   return nResult;
}

#ifdef WCHAR_SUPPORTED
uint32_t libBEFDecoderCopyStringAsWchar(
      LIB_BEF_DECODER* pDecoder,
      OUT wchar_t* pString)
{
   uint32_t nOriginOffset;
   uint32_t nResult;

   nOriginOffset = CURRENT_OFFSET(pDecoder);

   nResult = static_libBEFDecoderProcessUTF8String(
      pDecoder,
      PARENT_OFFSET(pDecoder),
      true,
      true,
      pString);

   CURRENT_OFFSET(pDecoder) = nOriginOffset;
   return nResult;
}
#endif
#else
uint32_t libBEFDecoderCopyString(
      LIB_BEF_DECODER* pDecoder,
      uint32_t nMaxLength,
      OUT char* pString)
{
   pString[nMaxLength-1]=0; /* '0' terminated string */
   return static_libBEFDecoderCopyArray(
         pDecoder,
         0,
         nMaxLength-1,
         LIB_BEF_BASE_TYPE_STRING,
         pString);
}

#endif /* EXCLUDE_UTF8 */

/*-----------------------------------------------------------------
 * Sequences
 *-----------------------------------------------------------------*/
void libBEFDecoderReadSequence(
      LIB_BEF_DECODER* pDecoder,
      OUT LIB_BEF_DECODER* pSequenceDecoder)
{
   uint32_t nEncodedSequenceSize=0;
   uint8_t nTag;

   assert(pDecoder!=NULL);
   assert(pSequenceDecoder!=NULL);

   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return;
   }

   if (nTag != LIB_BEF_TAG_SEQUENCE)
   {
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return;
   }

   if (static_libBEFDecoderReadCachedWord(pDecoder, &nEncodedSequenceSize, 0) == false)
   {
      return;
   }
   if (PARENT_OFFSET(pDecoder) - CURRENT_OFFSET(pDecoder) < nEncodedSequenceSize)
   {
      /* nEncodedSequenceSize overflows the current sequence */
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return;
   }

   pSequenceDecoder->pEncodedData=&pDecoder->pEncodedData[CURRENT_OFFSET(pDecoder)];
   pSequenceDecoder->nEncodedSize=nEncodedSequenceSize;

   libBEFDecoderInit(pSequenceDecoder);

   CURRENT_OFFSET(pDecoder) += nEncodedSequenceSize;
   static_libBEFDecoderInvalidateCache(pDecoder);
}

void libBEFDecoderOpenSequence(
      LIB_BEF_DECODER* pDecoder)
{
   uint32_t nEncodedSequenceSize;
   uint8_t nTag;

   if (pDecoder->nError != S_SUCCESS)
   {
      return;
   }

   /* If we reach the max depth, set an error
      Depth starts at 0, so max depth is LIB_BEF_DECODER_MAX_SEQUENCE_DEPTH-1 */
   if (pDecoder->nOffsetStackIndex == LIB_BEF_DECODER_MAX_SEQUENCE_DEPTH-1)
   {
      pDecoder->nError = S_ERROR_OUT_OF_MEMORY;
      return;
   }

   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return;
   }
   if (nTag != LIB_BEF_TAG_SEQUENCE)
   {
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return;
   }

   if (static_libBEFDecoderReadCachedWord(pDecoder, &nEncodedSequenceSize, 0) == false)
   {
      return;
   }

   if (PARENT_OFFSET(pDecoder) - CURRENT_OFFSET(pDecoder) < nEncodedSequenceSize)
   {
      /* Sub-sequence overflows current sequence */
      pDecoder->nError = S_ERROR_BAD_FORMAT;
      return;
   }

   /* open the new sequence */
   pDecoder->nOffsetStack[pDecoder->nOffsetStackIndex+1] = CURRENT_OFFSET(pDecoder);
   /* The following addition cannot overflow, because we have checked that the result
      is within the total buffer size */
   pDecoder->nOffsetStack[pDecoder->nOffsetStackIndex] += nEncodedSequenceSize;
   pDecoder->nOffsetStackIndex++;
   static_libBEFDecoderInvalidateCache(pDecoder);
}

void libBEFDecoderCloseSequence(
      LIB_BEF_DECODER* pDecoder)
{
   /* Check we are in a sequence, ie nOffsetStackIndex > 0 */
   if (pDecoder->nOffsetStackIndex == 0)
   {
      pDecoder->nError = S_ERROR_BAD_STATE;
      return;
   }
   pDecoder->nOffsetStackIndex--;
   pDecoder->nError=S_SUCCESS;
   static_libBEFDecoderInvalidateCache(pDecoder);
}

/*-----------------------------------------------------------------
 * Composite types
 *-----------------------------------------------------------------*/
void libBEFDecoderReadUUID(
      IN  LIB_BEF_DECODER* pDecoder,
      OUT S_UUID* pIdentifier)
{
   uint8_t nTag;
   uint8_t nSubTag;

   assert(pIdentifier != NULL);

   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return;
   }

   if (nTag != LIB_BEF_TAG_COMPOSITE)
   {
      /* Not a composite type */
      goto bad_format;
   }

   if (static_libBEFDecoderReadSubTag(pDecoder, &nSubTag) == false)
   {
      return;
   }
   if (nSubTag != LIB_BEF_SUBTAG_UUID)
   {
      /* Not a UUID */
      goto bad_format;
   }

   /* Here, we assume that the target platform is little-endian */
   static_libBEFDecoderReadRaw(pDecoder, 16, (uint8_t *)pIdentifier);

   static_libBEFDecoderInvalidateCache(pDecoder);
   return;

bad_format:
   pDecoder->nError = S_ERROR_BAD_FORMAT;
   return;
}

void libBEFDecoderReadMemoryReference(
      LIB_BEF_DECODER* pDecoder,
      S_HANDLE* phBlock,
      uint32_t* pnOffset,
      uint32_t* pnLength,
      uint32_t* pnFlags)
{
   uint8_t nTag;
   uint8_t nSubTag;

   assert(phBlock != NULL);
   assert(pnOffset != NULL);
   assert(pnLength != NULL);
   assert(pnFlags != NULL);

   if (static_libBEFDecoderReadTag(pDecoder, &nTag) == false)
   {
      return;
   }

   if (nTag != LIB_BEF_TAG_COMPOSITE)
   {
      /* Not a composite type */
      goto bad_format;
   }

   if (static_libBEFDecoderReadSubTag(pDecoder, &nSubTag) == false)
   {
      return;
   }
   if (nSubTag != LIB_BEF_SUBTAG_MEM_REF)
   {
      /* Not a Memory Reference */
      goto bad_format;
   }

   static_libBEFDecoderReadUint32(pDecoder, (uint32_t*)phBlock);
   static_libBEFDecoderReadUint32(pDecoder, pnOffset);
   static_libBEFDecoderReadUint32(pDecoder, pnLength);
   static_libBEFDecoderReadUint32(pDecoder, pnFlags);

   static_libBEFDecoderInvalidateCache(pDecoder);
   return;

bad_format:
   pDecoder->nError = S_ERROR_BAD_FORMAT;
   return;
}
