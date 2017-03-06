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

#include <stdlib.h>
#include <assert.h>

#define SM_EXPORT_IMPLEMENTATION
#include "smapi.h"

#include "lib_bef2_encoder.h"
#include "lib_bef2_decoder.h"

#include "smx_heap.h"

#include "smx_encoding.h"
#include "smx_utils.h"


/*----------------------------------------------------------------------------
 * Internal Structures and Constants
 *----------------------------------------------------------------------------*/
/*
* The default initial capacity, in bytes, of the encoder buffers.
*/
#define SMX_ENCODER_DEFAULT_INITIAL_CAPACITY  32

/*
* The default capacity increment, in bytes, of the encoder buffers.
*/
#define SMX_ENCODER_DEFAULT_CAPACITY_INCREMENT  32

/*----------------------------------------------------------------------------
 * Initialize Encoder
 *----------------------------------------------------------------------------*/

SM_ERROR SMXEncoderInit(
      SM_OPERATION* pOperation,
      uint32_t nInitialCapacity)
{
   SM_DEVICE_CONTEXT *pDevice;
   SM_ERROR nError;
   SMX_ENCODER* pEncoder = NULL;

   assert(pOperation != NULL);

   pDevice=pOperation->pSession->pDeviceContext;

   /*
    * Initialize the encoder structure.
    */
   pEncoder = &(pOperation->sEncoder);

   pEncoder->sNode.nType=TFAPI_OBJECT_TYPE_ENCODER;
   pEncoder->pOperation = pOperation;
   pEncoder->sBEFEncoder.nError = SM_SUCCESS;

   /*
    * Create the underlying BEF encoder.
    */
   if (nInitialCapacity == 0)
   {
      nInitialCapacity=TFAPI_DEFAULT_ENCODER_SIZE;
   }
   else if (nInitialCapacity == 0xffffffff)
   {
      nInitialCapacity=0;
   }

   if ((nInitialCapacity > TFAPI_DEFAULT_ENCODER_SIZE) ||
       (pDevice->bInnerEncoderBufferUsed))
   {
      /* note that the buffer is the only thing that should be allocated in
         the device context heap */
      if (nInitialCapacity==0)
      {
         pEncoder->sBEFEncoder.pBuffer = NULL;
      }
      else
      {
         pEncoder->sBEFEncoder.pBuffer= (uint8_t*)(SMXHeapAlloc(
            pDevice->hDeviceHeapStructure,
            nInitialCapacity));

         if (pEncoder->sBEFEncoder.pBuffer == NULL)
         {
            nError = SM_ERROR_OUT_OF_MEMORY;
            goto error;
         }
      }
   }
   else
   {
      pEncoder->sBEFEncoder.pBuffer = pDevice->pInnerEncoderBuffer;
      pDevice->bInnerEncoderBufferUsed=true;
   }
   pEncoder->sBEFEncoder.nCapacity = nInitialCapacity;
   libBEFEncoderInit(&pEncoder->sBEFEncoder);

   /*
    * Successful completion.
    */

   return SM_SUCCESS;

   /*
    * Error handling.
    */

error:
   (void)SMXEncoderUninit(pEncoder);
   return nError;
}


/*----------------------------------------------------------------------------
 * Uninitialize Encoder
 *----------------------------------------------------------------------------*/

SM_ERROR SMXEncoderUninit(
      SMX_ENCODER* pEncoder)
{
   SM_DEVICE_CONTEXT* pDevice;
   SM_ERROR nError = SM_SUCCESS;

   /* Conditions evaluation is done from the left to the right */
   if ((pEncoder == NULL) || (pEncoder->pOperation == NULL))
   {
      return SM_SUCCESS;
   }
   pDevice=pEncoder->pOperation->pSession->pDeviceContext;

   if (pEncoder->sBEFEncoder.pBuffer != pDevice->pInnerEncoderBuffer)
   {
      SMXHeapFree(pDevice->hDeviceHeapStructure, pEncoder->sBEFEncoder.pBuffer);
   }
   else
   {
      pDevice->bInnerEncoderBufferUsed=false;
   }
   pEncoder->sBEFEncoder.pBuffer = NULL;

   /* Encoder is not valid anymore*/
   pEncoder->pOperation = NULL;

   return nError;
}

/*----------------------------------------------------------------------------
 * Encoder Error
 *----------------------------------------------------------------------------*/

void SMXEncoderSetError(
      SMX_ENCODER* pEncoder,
      SM_ERROR nError)
{
   if ((nError != SM_SUCCESS) && (pEncoder->sBEFEncoder.nError == SM_SUCCESS))
   {
      pEncoder->sBEFEncoder.nError = nError;
   }
}

SM_ERROR SMXEncoderGetError(
      SMX_ENCODER* pEncoder)
{
   return pEncoder->sBEFEncoder.nError;
}

/*----------------------------------------------------------------------------
 * Create Decoder
 *----------------------------------------------------------------------------*/

SM_ERROR SMXDecoderCreate(
      SM_OPERATION* pOperation,
      const void *pEncodedData,
      uint32_t nEncodedDataSize,
      SMX_DECODER** ppDecoder)
{
   SM_ERROR nError;
   SM_DEVICE_CONTEXT* pDevice;
   SMX_DECODER* pDecoder;
   bool bMustReleaseDecoder = false;

   assert(pOperation != NULL);

   pDevice=pOperation->pSession->pDeviceContext;

   /*
    * Allocate (if required) and initialize the decoder structure.
    */

   if (ppDecoder == NULL)
   {
      /* The decoder is the one associated with the operation */
      pDecoder = &(pOperation->sDecoder);
   }
   else
   {
      *ppDecoder = NULL;

      /* A new decoder must be allocated, and returned */
      pDecoder = (SMX_DECODER*)SMXDeviceContextAlloc(pDevice, sizeof(SMX_DECODER));
      if (pDecoder == NULL)
      {
         TRACE_ERROR("SMXDecoderCreate: AllocMem failed");
         nError = SM_ERROR_OUT_OF_MEMORY;
         goto error;
      }

      bMustReleaseDecoder = true;
   }

   pDecoder->sNode.nType=TFAPI_OBJECT_TYPE_DECODER;
   pDecoder->pOperation = pOperation;
   pDecoder->sBEFDecoder.nError = SM_SUCCESS;
   pDecoder->pNextSibling = NULL;
   pDecoder->pParentDecoder = NULL;

   /*
    * Create the underlying BEF decoder.
    */
   if (pEncodedData!=NULL)
   {
      pDecoder->sBEFDecoder.pEncodedData=(uint8_t*)pEncodedData;
   }
   else
   {
      if (nEncodedDataSize == 0)
      {
         nEncodedDataSize=TFAPI_DEFAULT_DECODER_SIZE;
      }
      else if (nEncodedDataSize == 0xffffffff)
      {
         nEncodedDataSize=0;
      }

      if ((nEncodedDataSize > TFAPI_DEFAULT_DECODER_SIZE) ||
          (pDevice->bInnerDecoderBufferUsed))
      {
         pDecoder->sBEFDecoder.pEncodedData=(uint8_t*)(SMXHeapAlloc(
            pDevice->hDeviceHeapStructure,
            nEncodedDataSize));
         if (pDecoder->sBEFDecoder.pEncodedData == NULL)
         {
            if (pOperation->nMessageType!=TFAPI_CLOSE_CLIENT_SESSION)
            {
               nError = SM_ERROR_OUT_OF_MEMORY;
               goto error;
            }
            else
            {
               nEncodedDataSize=0;
            }
         }
      }
      else
      {
         pDecoder->sBEFDecoder.pEncodedData=pDevice->pInnerDecoderBuffer;
         pDevice->bInnerDecoderBufferUsed=true;
      }
   }
   pDecoder->sBEFDecoder.nEncodedSize=nEncodedDataSize;

   libBEFDecoderInit(&pDecoder->sBEFDecoder);

   if (ppDecoder != NULL)
   {
      *ppDecoder = pDecoder;
   }

   /*
    * Successful completion.
    */

   return SM_SUCCESS;

   /*
    * Error handling.
    */

error:
   (void)SMXDecoderDestroy(pDecoder, bMustReleaseDecoder);
   return nError;
}

/*----------------------------------------------------------------------------
 * Destroy Decoder
 *----------------------------------------------------------------------------*/

SM_ERROR SMXDecoderDestroy(
      SMX_DECODER* pRootDecoder,
      bool bMustReleaseDecoder)
{
   SM_DEVICE_CONTEXT *pDevice;
   SM_ERROR nError = SM_SUCCESS;
   SMX_DECODER* pNextDecoder;

   /* Conditions evaluation is done from the left to the right */
   if ((pRootDecoder == NULL) || (pRootDecoder->pOperation == NULL))
   {
      return SM_SUCCESS;
   }
   pDevice=pRootDecoder->pOperation->pSession->pDeviceContext;

   while (pRootDecoder->pNextSibling!=NULL)
   {
      pNextDecoder = pRootDecoder->pNextSibling;

      pRootDecoder->pNextSibling=pNextDecoder->pNextSibling;

      SMXDeviceContextFree(pDevice, (SMX_NODE*)pNextDecoder);
   }

   if (pRootDecoder->pParentDecoder==NULL)
   {
      if (pRootDecoder->sBEFDecoder.pEncodedData!=pDevice->pInnerDecoderBuffer)
      {
         SMXHeapFree(pDevice->hDeviceHeapStructure, pRootDecoder->sBEFDecoder.pEncodedData);
      }
      else
      {
         pDevice->bInnerDecoderBufferUsed=false;
      }
      pRootDecoder->sBEFDecoder.pEncodedData = NULL;

      /* Decoder is not valid anymore*/
      pRootDecoder->pOperation = NULL;
   }
   else
   {
      pRootDecoder->pParentDecoder->pNextSibling=NULL;
   }

   if (bMustReleaseDecoder)
   {
      SMXDeviceContextFree(pDevice, (SMX_NODE*)pRootDecoder);
   }

   return nError;
}

/*----------------------------------------------------------------------------
 * Decoder Error
 *----------------------------------------------------------------------------*/

SM_ERROR SMXDecoderGetError(
      SMX_DECODER* pDecoder)
{
   return pDecoder->sBEFDecoder.nError;
}


void SMXDecoderSetError(
      SMX_DECODER* pDecoder,
      SM_ERROR nError)
{
   if (nError != SM_SUCCESS && pDecoder->sBEFDecoder.nError == SM_SUCCESS)
   {
      pDecoder->sBEFDecoder.nError = nError;
   }
}
