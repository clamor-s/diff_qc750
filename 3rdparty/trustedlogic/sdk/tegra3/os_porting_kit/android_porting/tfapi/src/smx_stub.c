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
#define MODULE_NAME  "SMAPI"

#include <assert.h>
#include <string.h>
#define SM_EXPORT_IMPLEMENTATION
#include "smapi.h"

#include "smx_encoding.h"
#include "smx_utils.h"


/*----------------------------------------------------------------------------
 * Encoder Write UUID
 *----------------------------------------------------------------------------*/

SM_EXPORT void SMStubEncoderWriteUUID(
      SM_HANDLE hEncoder,
      const SM_UUID *pUUID)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }
   pBEFEncoder=&pEncoder->sBEFEncoder;

   if(pUUID == NULL)
   {
      pEncoder->sBEFEncoder.nError = SM_ERROR_ILLEGAL_ARGUMENT;
      nError = SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }

   libBEFEncoderWriteUUID(
      &pEncoder->sBEFEncoder,
      (S_UUID*)pUUID);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteUUID(
         &pEncoder->sBEFEncoder,
         (S_UUID*)pUUID);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteUUID(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}


/*----------------------------------------------------------------------------
 * Decoder Read UUID
 *----------------------------------------------------------------------------*/

SM_EXPORT void SMStubDecoderReadUUID(
      SM_HANDLE hDecoder,
      SM_UUID *pUUID)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;

   assert(pUUID!=NULL);

   memset((SM_UUID*)pUUID, 0, sizeof(S_UUID));

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   libBEFDecoderReadUUID(
      &pDecoder->sBEFDecoder,
      pUUID);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError != SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadUUID(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

