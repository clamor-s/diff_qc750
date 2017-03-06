/**
 * Copyright (c) 2008-2010 Trusted Logic S.A.
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
/* ----------------------------------------------------------------------------
*   Includes
* ---------------------------------------------------------------------------- */
#include "ssdi.h"
#include "certstore_protocol.h"
#include "pkcs11.h"
#include <string.h>

#if defined(__SYMBIAN32__)
extern void symbianPrintf(const char* format, ...);
extern void symbianFprintf(FILE *stream, const char* format, ...);
extern void symbianPutc(int c, FILE *stream);
#define printf symbianPrintf
#define fprintf symbianFprintf
#define putc symbianPutc
#endif

/* ----------------------------------------------------------------------------
*   Constants or Global Data
* ---------------------------------------------------------------------------- */

/** Service Identifier **/
/* {01B70C61-5EAF-4aa2-B399-80F351C8F0E0} */

#define TAG_BIT_STRING           0x03
#define TAG_OBJECT_IDENTIFIER    0x06
#define TAG_SEQUENCE             0x30
#define TAG_SET                  0x31


/* all id-at oid begin with this prefix */
static const uint8_t X509idatPrefix[]={
   0x06,0x03,0x55,0x04
};

static const uint8_t X509EmailAddressOID[]={
   0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x09,0x01
};

/* IDs of supported signature algorithms */
static const uint8_t sha1WithRSAAlgID[]={
   0x6,0x9,0x2a,0x86,0x48,0x86,0xf7,0xd,0x1,0x1,0x5,0x5,0x00    
};

/* The name fo the "counter" file, used to generate unique object identifiers */
#define COUNTER_FILENAME "counter"

/* ----------------------------------------------------------------------------
*   Internal Types
* ---------------------------------------------------------------------------- */
/*
 *  The structure SESSION_CONTEXT is used to save some values during a client session:
 *  bManagerRights == true  when the client is authenticated and possesses the property
 *                          "example_certstore.allow_manager_operations: true" in its manifest.
 */
typedef struct {
   bool        bManagerRights;
} SESSION_CONTEXT;


/*
 *  The structure STRUCT_TLV is used to store data in DER format.
 */
typedef struct STRUCT_TLV
{
   uint32_t    nTag;
   uint32_t    nLength;
   uint8_t*    pValue;
} STRUCT_TLV;

/*
 *  The structure STRUCT_CERTIFICATE is used for storing certifcate fields from a DER format
 */
typedef struct STRUCT_CERTIFICATE
{
  uint8_t*    pCertDataTBS;
  uint32_t    nCertDataTBSLength;

  uint32_t    nSignatureAlgorithm;  
  uint8_t*    pSignature;
  uint32_t    nSignatureLength;
  
  uint8_t*    pModulus;
  uint32_t    nModulusLength;
  
  uint8_t*    pPublicExponent;
  uint32_t    nPublicExponentLength;

}STRUCT_CERTIFICATE;

/* ----------------------------------------------------------------------------
*   Client properties management Functions
* ---------------------------------------------------------------------------- */
/**
 *  Function printClientPropertiesAndUUID(void):
 *           Print the client properties and its UUID
 **/
static S_RESULT printClientPropertiesAndUUID(void)
{
   S_UUID      sClientID;
   uint32_t    nPropertyCount;
   S_PROPERTY* pPropertyArray;
   S_RESULT    nError;
   uint32_t    i;

   /* Get and print the Client UUID. */
   SSessionGetClientID(&sClientID);
   SLogTrace("CLIENT UUID: %{uuid}",&sClientID);

   /* Get and print all other client properties. */
   nError = SSessionGetAllClientProperties(&nPropertyCount, &pPropertyArray);
   if (nError != S_SUCCESS)
   {
      SLogError("SSessionGetAllClientProperties() failed (0x%08X).", nError);
      return nError;
   }
   SLogTrace("CLIENT PROPERTIES:");
   for (i = 0; i < nPropertyCount; i++)
   {
      SLogTrace(" %s: %s",pPropertyArray[i].pName, pPropertyArray[i].pValue);
   }

   SMemFree(pPropertyArray);
   return S_SUCCESS;
}

/**
 *  Function getClientEffectiveManagerRights:
 *  Description:
 *           Returns true if the client is authenticated AND possesses the property
 *           "example_certstore.allow_manager_operations: true" in its manifest
 **/
static bool getClientEffectiveManagerRights(void)
{
   uint32_t    nClientLoginMode;
   bool        bAllowManagerOperations;

   /* Note that the property "sm.client.login" is always set by the system */
   SSessionGetClientPropertyAsInt("sm.client.login", &nClientLoginMode);

   if (nClientLoginMode != S_LOGIN_AUTHENTICATION)
   {
      /* Client not authenticated */
      return false;
   }

   SSessionGetClientPropertyAsBool("example_certstore.allow_manager_operations", &bAllowManagerOperations);
   /* Note that if the property does not exist, bAllowManagerOperations is set to false */
   return bAllowManagerOperations;
}


/** 
 *  Function CertIDToFileName :
 *  Description:
 *        Converts an integer type data to uint8_t* representation
 * Input: uint32_t n = the value to convert
 * Output:char* s = the resulting zero-terminated string
**/

void CertIDToFileName(uint32_t nInteger, char* pString)
{
    uint32_t   i,j;
    uint32_t   nTemp;
    uint32_t   nStringLen = 0;
        
    i = 0;   

    if (nInteger == 0)
    {
        pString[i] = '0';
        nStringLen++;
    }
    
    while(nInteger != 0)
    {
        uint32_t nDigit = nInteger & 0x0F;
        if (nDigit < 10 )
        {
            pString[i] = nDigit + '0';
        }
        else
        {
            pString[i] = nDigit + 'A' - 10;
        }
        i++;
        nStringLen++;

        nInteger >>= 4;
    }
    pString[nStringLen]='\0';

    /* Revert the string  */
    for (i = 0, j = nStringLen-1; i<j; i++, j--) 
    {
        nTemp = pString[i];
        pString[i] = pString[j];
        pString[j] = nTemp;
    }
} 


/** 
 *  Function FileNameToCertID :
 *  Description:
 *         Reads the number value encoded in the string (hex encoded)
 *         and returns the uint32 representation.
 * Input:  const char* s = the string buffer to read
 * Output: uint32_t n = the decoded number
 **/

 S_RESULT FileNameToCertID (const char* pString, uint32_t* pValue)
{
    uint32_t   i;
    uint32_t   nValue, nDigit;
    uint32_t   nRadix = 16;
        
    nDigit = 0;    
    nValue = 0;
    
    if( pString == NULL)
    {
        return 0;
    }

    i=0;
    while (pString[i]!= '\0')
    {
        
        if (pString[i] >= '0' && pString[i] <= '9')
        {
            nDigit = pString[i] - '0';
        }
        else if(pString[i] >= 'A' && pString[i] <= 'F')
        {
           nDigit = pString[i] - 'A' + 10;
        }
        else if(pString[i] >= 'a' && pString[i] <= 'f')
        {
           nDigit = pString[i] - 'f'+ 10 ;
        }
        else
        {
            SLogError("The string \"%s\" is not the hexadecimal representation of a number.", pString);
            return S_ERROR_BAD_FORMAT;
        }
        i++;
        nValue = nValue * nRadix  + nDigit;

    }
    *pValue = nValue;
    return S_SUCCESS;
}

/* ----------------------------------------------------------------------------
 *   Parsing Certificate Functions
 * ---------------------------------------------------------------------------- */

/**
 *  Function getTLVFromBuffer:
 *  Description:
 *           Parses one TLV from a buffer.
 *  Input:   uint8_t* pBuffer        = buffer in DER format
 *           uint32_t nBufferLength  = buffer length in bytes (maximum size of the whole TLV)
 *  Output:  STRUCT_TLV* pTLV        = the TLV structure filled with the tag, length, and value buffer
 *
 **/
static S_RESULT getTLVFromBuffer(IN   uint8_t*       pBuffer,
                                 IN   uint32_t       nBufferLength,
                                 OUT  STRUCT_TLV*    pTLV)
{
   uint8_t nLength0;

   /* A TLV always contains at least 2 bytes: one for the tag and one for the length */
   if (nBufferLength < 2) goto buffer_too_short;

   pTLV->nTag = *pBuffer;
   pBuffer++;
   nBufferLength--;

   /* Parse the length */
   nLength0 = *pBuffer;
   pBuffer++;
   nBufferLength--;
   if ((nLength0 & 0x80) == 0)
   {
      /* Length is coded on the bits 7 to 1 of *pBuffer */
      pTLV->nLength = nLength0 & (~0x80);
   }
   else
   {
      /* The number of bytes needed to code Length is coded on the bits 7 to 1 of *pCurrentPointer */
      uint32_t nLengthLength = nLength0 & (~0x80);

      if (nLengthLength > 4)
      {
         /* The length can't be represented on 32 bits. This necessarily denotes an overflow */
         goto buffer_too_short;
      }

      /* Check that there is enough bytes in the buffer to represent the length */
      if (nBufferLength < nLengthLength) goto buffer_too_short;

      /* Parse the length (in big endian) */
      pTLV->nLength = 0;
      while (nLengthLength != 0)
      {
         nLengthLength--;
         pTLV->nLength |= (*pBuffer) << (8*nLengthLength);
         pBuffer++;
         nBufferLength--;
      }
   }

   /* Check that the value fits in the buffer */
   if (nBufferLength < pTLV->nLength) goto buffer_too_short;

   /* get pValue */
   pTLV->pValue = pBuffer;

   return S_SUCCESS;

buffer_too_short:
   SLogError("getTLVFromBuffer failed (Buffer too short).");
   return S_ERROR_SHORT_BUFFER;
}


/**
 *  Function skipNextTLV:
 *  Description:
 *           Parse the TLV immediately after pCurrentTLV.
 *           Precondition: the current TLV must belong to a sequence or a set.
 *  Input :  STRUCT_TLV* pCurrentTLV     = points to the current TLV structure
 *           STRUCT_TLV* pCertificateTLV = points to the entire certificate structure
 *  Output:  STRUCT_TLV* pNextTLV        = pNextTLV points to the next TLV of this sequence/set
 *
 *  Precondition:    *pCurrentTLV belongs to a sequence or a set.
 *
 **/
static S_RESULT skipNextTLV(IN  STRUCT_TLV* pCurrentTLV,
                            IN  STRUCT_TLV* pCertificateTLV,
                            OUT STRUCT_TLV* pNextTLV)
{
   S_RESULT nError;
   uint8_t* pBuffer;

   pBuffer = pCurrentTLV->pValue + pCurrentTLV->nLength;

   /* Check that we have not reached the end of the buffer */
   if ( pBuffer >= pCertificateTLV->pValue + pCertificateTLV->nLength)
   {
      SLogTrace("skipNextTLV: at the end of pCertificateTLV");
      return S_ERROR_BAD_PARAMETERS;
   }

   nError = getTLVFromBuffer(
      pBuffer, 
      pCertificateTLV->pValue + pCertificateTLV->nLength - pBuffer,
      pNextTLV);

   if (nError != S_SUCCESS)
   {
      SLogError("skipNextTLV failed.");
      return nError;
   }

   return S_SUCCESS;
}


/**
 *  Function skipInsideTLV:
 *  Description:
 *           If the current TLV belongs to a sequence or a set, this function gets the
 *           first TLV inside the current one, else the function returns S_ERROR_BAD_FORMAT.
 *  Input :  STRUCT_TLV* pCurrentTLV = points to the current TLV structure
 *  Output:  STRUCT_TLV* pInsideTLV  = pInsideTLV points to the first TLV of this sequence/set
 *
 *  Precondition:    *pCurrentTLV belongs to a sequence or a set.
 *
 **/
static S_RESULT skipInsideTLV(IN   STRUCT_TLV* pCurrentTLV,
                              OUT  STRUCT_TLV* pInsideTLV)
{
   S_RESULT nError;
   uint8_t* pBuffer;
   uint32_t nBufferLength;

   if ((pCurrentTLV->nTag != TAG_SEQUENCE)&&(pCurrentTLV->nTag != TAG_SET))
   {
      SLogError("skipInsideTLV failed (Current TLV is not a sequence or a set).");
      return S_ERROR_BAD_FORMAT;
   }

   pBuffer = pCurrentTLV->pValue;
   nBufferLength = pCurrentTLV->nLength;

   nError = getTLVFromBuffer(pBuffer, nBufferLength, pInsideTLV);
   if (nError != S_SUCCESS)
   {
      SLogError("skipInsideTLV failed.");
      return nError;
   }

   return S_SUCCESS;
}



/**
 *  Function getDistinguishedName:
 *  Description:
 *           Get the Distinguished Name TLV from the Certificate TLV.
 *  Input :  STRUCT_TLV* pCertificateTLV         = points to the TLV of the entire certificate
 *  Output:  STRUCT_TLV* pDistinguishedNameTLV   = points to the TLV of the distinguished name
 *
 *  Note : pCertificateTLV is the TLV containing the entire certificate.
 *
 **/
static S_RESULT getDistinguishedName(IN  STRUCT_TLV* pCertificateTLV,
                                     OUT STRUCT_TLV* pDistinguishedNameTLV)
{
   S_RESULT    nError;
   STRUCT_TLV  sCurrentTLV;
   

   /* tbsCertificate */
   nError = skipInsideTLV(pCertificateTLV, &sCurrentTLV); /* do not modify pCertificateTLV */
   if(nError != S_SUCCESS)
   {
      SLogError("tbsCertificate not found.");
      return nError;
   }
  
   
   /* CAUTION:
   * the first field "tbsCertificate->version" is optional
   *    case 1: version is not present (i.e. version 1)
   *    case 2: version is present     (i.e. version is 2 or 3)
   * the following field "tbsCertificate->serialNumber" is mandatory  and is not a sequence
   * the following field "tbsCertificate->signature" is mandatory and is a sequence
   *
   * CONSEQUENCE:
   * if the second field is a sequence, this is the signature thus version is not present
   * else version is present
   */

   /* case 1: tbsCertificate->version       */
   /* case 2: tbsCertificate->serialNumber  */
   nError = skipInsideTLV(&sCurrentTLV, &sCurrentTLV);
   if(nError != S_SUCCESS)
   {
      SLogError("the first tbsCertificate field is not found.");
      return nError;
   }

   /* case 1: tbsCertificate->serialNumber  */
   /* case 2: tbsCertificate->signature     */
   nError = skipNextTLV(&sCurrentTLV, pCertificateTLV,&sCurrentTLV);
   if(nError != S_SUCCESS)
   {
      SLogError("the second tbsCertificate field is not found.");
      return nError;
   }
   if (sCurrentTLV.nTag != TAG_SEQUENCE)
   {
      /* case 2: tbsCertificate->signature */
      nError = skipNextTLV(&sCurrentTLV, pCertificateTLV,&sCurrentTLV);
      if(nError != S_SUCCESS)
      {
         SLogError("tbsCertificate->signature not found.");
         return nError;
      }
   }

   /* tbsCertificate->issuer */
   nError = skipNextTLV(&sCurrentTLV, pCertificateTLV, &sCurrentTLV);
   if(nError != S_SUCCESS)
   {
      SLogError("tbsCertificate->issuer not found.");
      return nError;
   }
   /* tbsCertificate->validity */
   nError = skipNextTLV(&sCurrentTLV, pCertificateTLV, &sCurrentTLV);
   if(nError != S_SUCCESS)
   {
      SLogError("tbsCertificate->validity not found.");
      return nError;
   }
   /* tbsCertificate->subject : The certificate Distinguished Name.*/
   nError = skipNextTLV(&sCurrentTLV, pCertificateTLV, &sCurrentTLV);
   if(nError != S_SUCCESS)
   {
      SLogError("tbsCertificate->subject not found.");
      return nError;
   }

   /* populate pDistinguishedNameTLV */
   pDistinguishedNameTLV->nTag    = sCurrentTLV.nTag;
   pDistinguishedNameTLV->nLength = sCurrentTLV.nLength;
   pDistinguishedNameTLV->pValue  = sCurrentTLV.pValue;

   return S_SUCCESS;
}

/**
 *  Function readDistinguishedName:
 *  Description:
 *           Print and encode the distinguished name fields.
 *  Input :  STRUCT_TLV* pDistinguishedNameTLV = points to the TLV of the distinguished name
 *           STRUCT_TLV* pCertificateTLV       = points to the entire certificate structure
 *           S_HANDLE hEncoder                 = the encoder handle
 *
 **/
static S_RESULT readDistinguishedName( IN STRUCT_TLV*  pCertificateTLV,
                                       IN STRUCT_TLV*  pDistinguishedNameTLV,
                                       IN OUT S_PARAM pParams[4])
{
   S_RESULT       nError = S_SUCCESS;
   STRUCT_TLV     sCurrentTLV;
   STRUCT_TLV     sTempTLV;
   char*          pConcatString = NULL;
   char*          pCurrentChar; /* This points within pConcatString as it is filled */

   uint32_t       nMaxConcatLength;
   bool           bFirstField;
   uint8_t*       pOutput    = pParams[1].memref.pBuffer;
   uint32_t       nOutputBufferLen = pParams[1].memref.nSize;

   pParams[1].memref.nSize = 0;
   /* Allocate the string  pConcatString = "Type : Content" */
   nMaxConcatLength =
      10    /* Maximal Type length: 10 for instance */
      + 2   /* ": " */
      + pDistinguishedNameTLV->nLength /* Maximal Content length: sub-optimal but safe */
      + 1;  /* terminal 0 */
   pConcatString = SMemAlloc(nMaxConcatLength);
   if (pConcatString == NULL)
   {
      SLogError("Cannot allocate string");
      nError = S_ERROR_OUT_OF_MEMORY;
      goto cleanup;
   }

   /* Check this is a sequence */
   if(pDistinguishedNameTLV->nTag != TAG_SEQUENCE)
   {
      SLogError("pDistinguishedNameTLV must be a sequence.");
      nError = S_ERROR_BAD_FORMAT;
      goto cleanup;
   }

   SLogTrace("DISTINGUISHED NAME: ");

   bFirstField = true;
   do
   {
      pCurrentChar = pConcatString;
      if (bFirstField)
      {
         /* First field = sCurrentTLV */
         nError = skipInsideTLV(pDistinguishedNameTLV, &sCurrentTLV); /* save pDistinguishedNameTLV */
         if(nError != S_SUCCESS)
         {
            SLogError("readDistinguishedName() failed (0x%08X).", nError);
            goto cleanup;
         }
         bFirstField = false;
      }
      else
      {
         /* Go to the next field = sCurrentTLV */
         nError = skipNextTLV(&sCurrentTLV, pCertificateTLV,&sCurrentTLV);
         if(nError != S_SUCCESS)
         {
            SLogError("readDistinguishedName() failed (0x%08X).", nError);
            goto cleanup;
         }
      }

      /* Each field of the DN sequence has the following form :
      *     (31 L (30 L' (06 03 55 04 Type) (13 L" Content)))
      * i.e.(31 L (30 L' (id-atPrefix Type) (13 L" Content)))
      * or
      *     (31 L (30 L' (06 09 2a 86 48 86 f7 0d 01 09 01) (16 L" Email)))
      * i.e.(31 L (30 L' (emailAdressOid                  ) (16 L" Email)))
      */
      if(sCurrentTLV.nTag != TAG_SET)
      {
         SLogError("Tag must be (0x%08X).", TAG_SET);
         nError = S_ERROR_BAD_FORMAT;
         goto cleanup;
      }

      nError = skipInsideTLV(&sCurrentTLV, &sTempTLV); /* save sCurrentTLV */
      if(nError != S_SUCCESS)
      {
         SLogError("readDistinguishedName() failed (0x%08X).", nError);
         goto cleanup;
      }
      if(sTempTLV.nTag != TAG_SEQUENCE)
      {
         SLogError("Tag must be (0x%08X).", TAG_SEQUENCE);
         nError = S_ERROR_BAD_FORMAT;
         goto cleanup;
      }

      /* Form: (31 L (30 L' (emailAdressOid) (16 L" Email))) */
      if (SMemCompare(sTempTLV.pValue, X509EmailAddressOID, sizeof(X509EmailAddressOID))== 0)
      {
         nError = skipInsideTLV(&sTempTLV, &sTempTLV);
         if(nError != S_SUCCESS)
         {
            SLogError("readDistinguishedName() failed (0x%08X).", nError);
            goto cleanup;
         }
         nError = skipNextTLV(&sTempTLV, pCertificateTLV,&sTempTLV);
         if(nError != S_SUCCESS)
         {
            SLogError("readDistinguishedName() failed (0x%08X).", nError);
            goto cleanup;
         }
         *pCurrentChar++ = 'E';
         *pCurrentChar++ = ' ';
         *pCurrentChar++ = ':';
         *pCurrentChar++ = ' ';
         SMemMove(pCurrentChar, sTempTLV.pValue, sTempTLV.nLength);
         pCurrentChar += sTempTLV.nLength;
      }
      /* Form: (31 L (30 L' (id-atPrefix Type) (13 L" Content))) */
      else if(SMemCompare(sTempTLV.pValue, X509idatPrefix, sizeof(X509idatPrefix))== 0)
      {
         uint8_t nTypeCode = sTempTLV.pValue[sizeof(X509idatPrefix)];

         switch(nTypeCode)
         {
         case 3: /* Common Name */
            *pCurrentChar++ = 'C';
            *pCurrentChar++ = 'N';
            break;
         case 6: /* Country Name */
            *pCurrentChar++ = 'C';
            *pCurrentChar++ = ' ';
            break;
         case 7: /* Locality Name */
            *pCurrentChar++ = 'L';
            *pCurrentChar++ = ' ';
            break;
         case 8:/* State Or Province Name */
            *pCurrentChar++ = 'S';
            *pCurrentChar++ = 'T';
            break;
         case 10: /* Organization Name */
            *pCurrentChar++ = 'O';
            *pCurrentChar++ = ' ';
            break;
         case 11: /* Organizational Unit Name */
            *pCurrentChar++ = 'O';
            *pCurrentChar++ = 'U';
            break;
         default :
            /* Unknown */
            *pCurrentChar++ = '?';
            *pCurrentChar++ = '?';
            break;
         }
         *pCurrentChar++ = ':';
         *pCurrentChar++ = ' ';
         nError = skipInsideTLV(&sTempTLV, &sTempTLV);
         if(nError != S_SUCCESS)
         {
            SLogError("readDistinguishedName() failed (0x%08X).", nError);
            goto cleanup;
         }
         nError = skipNextTLV(&sTempTLV, pCertificateTLV, &sTempTLV);
         if(nError != S_SUCCESS)
         {
            SLogError("readDistinguishedName() failed (0x%08X).", nError);
            goto cleanup;
         }
         SMemMove(pCurrentChar, sTempTLV.pValue, sTempTLV.nLength);
         pCurrentChar += sTempTLV.nLength;
      }
      else /* extended fields not supported here */
      {
         nError = S_ERROR_BAD_FORMAT;
         SLogError("readDistinguishedName() failed (0x%08X).", nError);
         goto cleanup;
      }

      /* Add terminal zero */
      *pCurrentChar++ = '\0';

      /* Print and encode the field */
      SLogTrace("   %s", pConcatString);
      {
         uint32_t nBufferLen = strlen(pConcatString)+1;
         if (nOutputBufferLen >= pParams[1].memref.nSize+nBufferLen)
         {
            memcpy(pOutput, pConcatString, nBufferLen);
            pParams[1].memref.nSize += nBufferLen;
            pOutput = &pOutput[nBufferLen];
   }
         else
         {
            pParams[1].memref.nSize += nBufferLen;
            nError = S_ERROR_SHORT_BUFFER;
         }
      }
   }
   while ((sCurrentTLV.pValue +  sCurrentTLV.nLength) <
      (pDistinguishedNameTLV->pValue +  pDistinguishedNameTLV->nLength));
cleanup:
   SMemFree(pConcatString);
   return nError;
}

/**
 *  Function parseAndEncodeDistinguishedName:
 *  Description:
 *           Find and Parse the distinguished name, then call readDistinguishedName.
 *  Input :  uint8_t*    pCertificate         = buffer containing a certificate
 *           uint32_t    nCertificateLength   = length of the certificate in bytes
 *           S_HANDLE    hEncoder             = the encoder handle
 *
 **/
static S_RESULT parseAndEncodeDistinguishedName(
                                       IN uint8_t*    pCertificate,
                                       IN uint32_t    nCertificateLength,
                                       IN OUT S_PARAM pParams[4])
{
   S_RESULT       nError;
   STRUCT_TLV     sCertificateTLV;
   STRUCT_TLV     sDistinguishedNameTLV;


   /* get  Distinguished Name */
   nError = getTLVFromBuffer( pCertificate, nCertificateLength, &sCertificateTLV);
   if (nError!= S_SUCCESS)
   {
      SLogError("parseAndEncodeDistinguishedName() failed.");
      return nError;
   }

   nError = getDistinguishedName(&sCertificateTLV, &sDistinguishedNameTLV);
   if (nError!= S_SUCCESS)
   {
      SLogError("parseAndEncodeDistinguishedName() failed.");
      return nError;
   }

   /* parse Distinguished Name and print the fields */
   nError = readDistinguishedName(&sCertificateTLV, &sDistinguishedNameTLV, pParams);
   if (nError!= S_SUCCESS)
   {
      SLogError("parseAndEncodeDistinguishedName() failed.");
      return nError;
   }

   return S_SUCCESS;
}

/**
 * Function skipInsideBitString:
 * Description:
 *          If the current TLV is a BIT STRING, this function gets the first TLV inside the current one,
 *          else the function returns S_ERROR_BAD_FORMAT.
 *  Input : STRUCT_TLV_PTR pCurrentTLV = points to the current TLV structure
 *  Output: STRUCT_TLV_PTR pInsideTLV  = pInsideTLV points to the first TLV of this bit string
 *
 * Precondition: pCurrentTLV must be a BIT STRING
 *
 **/
static S_RESULT skipInsideBitString(IN   STRUCT_TLV* pCurrentTLV,
                                    OUT  STRUCT_TLV* pInsideTLV)
{
   S_RESULT nError;
   uint8_t* pBuffer;
   uint32_t nBufferLength;

   if ((pCurrentTLV->nTag != TAG_BIT_STRING))
   {
      SLogError("skipInsideBitString failed (Current TLV is not a bit string).");
      return S_ERROR_BAD_FORMAT;
   }

   /* first byte from value is the number of unused bits in the final octet so we skip it*/
   if (pCurrentTLV->nLength >= 1)
   {
       pBuffer = pCurrentTLV->pValue + 1 ;
       nBufferLength = pCurrentTLV->nLength - 1;
   }
   else
   {
      SLogError("skipInsideBitString failed (BIT STRING has no value).");
      return S_ERROR_BAD_FORMAT;
   }

   nError = getTLVFromBuffer(pBuffer,nBufferLength,pInsideTLV);
   if (nError != S_SUCCESS)
   {
      SLogError("skipInsideBitString failed.");
      return nError;
   }

   return S_SUCCESS;
}
/**
 * Function getAlgorithm:
 * Description:
 *         This function returns the PKCS11 code corresponding to a TLV representation.
 *
 * Input:  pAlgorithm  = buffer containing the id of the algorithm
 * Output: pAlgoCode   = integer representing the PKCS11 code for the algorithm
 *         S_SUCCESS if the algorithm was recognized, S_ERROR_NOT_SUPPORTED otherwise.
 **/
static S_RESULT getAlgorithm(IN uint8_t*   pAlgorithm,
                             IN uint32_t   nAlgorithmLength,
                             OUT uint32_t* pAlgoCode){
  
   if(SMemCompare(pAlgorithm, sha1WithRSAAlgID, nAlgorithmLength)==0)
   {
       *pAlgoCode = CKM_SHA1_RSA_PKCS;
       return S_SUCCESS;
   }
   *pAlgoCode = 0xFFFFFFFF;

   SLogError("Algorithm not supported");
   return S_ERROR_NOT_SUPPORTED;    
   
}
/**
 * Function getPublicKey:
 * Description:
 *         Parses a certificate TLV for the public key item and stores it in a certificate structure.         
 * Input:  pCertificateTLV    =  points to the TLV of the entire certificate
 * Output: pCertificateStruct =  a certificate structure containing the parsed public key.
 **/
static S_RESULT getPublicKey(IN STRUCT_TLV* pCertificateTLV,                             
                             OUT STRUCT_CERTIFICATE* pCertificateStruct)
{
    S_RESULT    nError;
    STRUCT_TLV  sDistinguishedName;
    STRUCT_TLV  sCurrentTLV;

    /* Skip after the DistinguishedName*/
    nError = getDistinguishedName(pCertificateTLV,&sDistinguishedName);
    if(nError != S_SUCCESS)
    {
        SLogError("Distinguished Name not found.");
        return nError;
    }
  
    /* subjectPublicKeyInfo */
    nError = skipNextTLV(&sDistinguishedName,  pCertificateTLV, &sCurrentTLV); 
    if(nError != S_SUCCESS)
    {
        SLogError("subjectPublicKeyInfo not found.");
        return nError;
    }
    /*subjectPublicKeyInfo :  algorithm */
    nError = skipInsideTLV(&sCurrentTLV,&sCurrentTLV); 
    if(nError != S_SUCCESS)
    {
        SLogError(" Algorithm for the PublicKey not found.");
        return nError;
    }
    /*subjectPublicKeyInfo: subjectPublicKey - BIT STRING */
    nError = skipNextTLV(&sCurrentTLV,  pCertificateTLV, &sCurrentTLV); 
    if(nError != S_SUCCESS)
    {
        SLogError("PublicKey item not found.");
        return nError;
    }

    /* subjectPublicKeyData - SEQUENCE */
    nError = skipInsideBitString(&sCurrentTLV,&sCurrentTLV); 
    if(nError != S_SUCCESS)
    {
        SLogError("PublicKey item not found.");
        return nError;
    }
    /* subjectPublicKeyData : modulus */
    nError = skipInsideTLV(&sCurrentTLV,&sCurrentTLV); 
    if(nError != S_SUCCESS)
    {
        SLogError(" Modulus item of the PublicKey not found.");
        return nError;
    }
    /* Store the modulus */
    pCertificateStruct->nModulusLength = sCurrentTLV.nLength;
    pCertificateStruct->pModulus       = sCurrentTLV.pValue;
    

    /* subjectPublicKeyData : public exponent */
    nError = skipNextTLV(&sCurrentTLV,  pCertificateTLV, &sCurrentTLV); 
    if(nError != S_SUCCESS)
    {
        SLogError("Public Exponent item of the PublicKey  not found.");
        return nError;
    }
    /* Store the public exponent */
    pCertificateStruct->nPublicExponentLength = sCurrentTLV.nLength;
    pCertificateStruct->pPublicExponent       = sCurrentTLV.pValue;

    return S_SUCCESS;

}

/**
 * Function parseCertificate:
 * Description:
 *         Parses a certificate TLV for the following fields: 
 *            - tbsCertificate
 *            - publicKey
 *            - signature algorithm
 *            - signature
 * Input:  pCertificateTLV    = points to the TLV of the entire certificate
 * Output: pCertificateStruct = a certificate structure containing the parsed items.
 *
**/
static S_RESULT parseCertificate(IN  STRUCT_TLV* pCertificateTLV,
                                 OUT STRUCT_CERTIFICATE* pCertificateStruct)
{

       
   S_RESULT    nError;
   STRUCT_TLV  sCurrentTLV;
   STRUCT_TLV  sLastCertTLV;
   uint32_t    nAlgoCode;


   /* Parse  the Public Key */
   nError = getPublicKey(pCertificateTLV,pCertificateStruct);
   if(nError != S_SUCCESS)
   {
       SLogError("Public key field not found.");
       return nError;
   }
   
   /* tbsCertificate */
   nError = skipInsideTLV(pCertificateTLV, &sCurrentTLV); 
   if(nError != S_SUCCESS)
   {
      SLogError("tbsCertificate not found.");
      return nError;
   }
   
   /* Store the data to be signed */
   pCertificateStruct->pCertDataTBS = sCurrentTLV.pValue - 4; /* not only the content,V,but the bytes for Tag and Length */                                                          
   pCertificateStruct->nCertDataTBSLength = sCurrentTLV.nLength + 4;

   /* signatureAlgorithm */   
   nError = skipNextTLV(&sCurrentTLV, pCertificateTLV,&sLastCertTLV);
   if(nError != S_SUCCESS)
   {
      SLogError("signatureAlgorithm not found.");
      return nError;
   }

   /* interpret signatureAlgorithm */
   nError = getAlgorithm( sLastCertTLV.pValue,sLastCertTLV.nLength,&nAlgoCode);
   if(nError != S_SUCCESS)
   {
      SLogError("Can not parse the signatureAlgorithm field.");
      return nError;
   }
   /* store signatureAlgorithm */
   pCertificateStruct->nSignatureAlgorithm = nAlgoCode;  
   
     
   /* signature - is a BIT STRING */
   nError = skipNextTLV(&sLastCertTLV, pCertificateTLV, &sCurrentTLV);
   if(nError != S_SUCCESS)
   {
      SLogError("the second tbsCertificate field is not found.");
      return nError;
   }
   /* store signature */
   if(sCurrentTLV.nTag == TAG_BIT_STRING)
   {
       if ((sCurrentTLV.pValue != NULL)&&(sCurrentTLV.nLength >= 1 ))
       {
           if(sCurrentTLV.pValue[0] == 0x00) 
           {
               /* skip leading zero */
               pCertificateStruct->pSignature = sCurrentTLV.pValue + 1 ; 
               pCertificateStruct->nSignatureLength = sCurrentTLV.nLength -1 ;
           }
           else
           {
               pCertificateStruct->pSignature = sCurrentTLV.pValue;
               pCertificateStruct->nSignatureLength = sCurrentTLV.nLength;
           }
       }
       else
       {
           SLogError("Can not find signature - value filed is empty");
           return S_ERROR_BAD_FORMAT;
       }
   }
   else
   {
       SLogError("Can not find signature - current representation is not a BIT STRING");
       return S_ERROR_BAD_FORMAT;
   }
   
   return S_SUCCESS;
   
}


/**
 * Function verifySignature:
 * Description:
 *        Checks if the first certificate is signed by the second one. 
 *       
 * Input: pCertificateBuff          = buffer containing the certificate to be verified
 *        nCertificateBuffLen       = the size of the certificate to be verified
 *        pIssuerCertificateBuff    = buffer containing the issuer certificate
 *        pIssuerCertificateBuffLen = the size of the issuer certificate
 * Output:
 *       If the verification is successful, the function returns S_SUCCESS.
 **/

static S_RESULT verifySignature (IN uint8_t* pCertficateBuff,
                                 IN uint32_t nCertficateBuffLen,
                                 IN uint8_t* pIssuerCertificateBuff,
                                 IN uint32_t nIssuerCertificateBuffLen)
{

    STRUCT_CERTIFICATE   sCertificate;
    STRUCT_CERTIFICATE   sIssuerCertificate;
    STRUCT_TLV           sCertificateTLV;

    CK_MECHANISM         sMechanism;
    CK_SESSION_HANDLE    hPKCS11Session = CK_INVALID_HANDLE;
    CK_OBJECT_HANDLE     hPubKey = CK_INVALID_HANDLE;

    S_RESULT            nError;
    CK_RV               errorCode; /* Error code for PKCS#11 operations */

    static const CK_OBJECT_CLASS pubClass = CKO_PUBLIC_KEY;
    static const CK_KEY_TYPE keyType = CKK_RSA;
    static const CK_BBOOL bTrue = TRUE;
    static const CK_BBOOL bFalse = FALSE;

    /*Create a key template for the issuer public key */
    CK_ATTRIBUTE publicKeyTemplate[6] = { 
        {CKA_CLASS,           (void*)&pubClass,    sizeof(pubClass) },
        {CKA_TOKEN,           (void*)&bFalse,      sizeof(bFalse) },
        {CKA_KEY_TYPE,        (void*)&keyType,     sizeof(keyType) },
        {CKA_VERIFY,          (void*)&bTrue,       sizeof(bTrue) },
        {CKA_MODULUS,         NULL, 0  },   /* This attribute will be filled later on */
        {CKA_PUBLIC_EXPONENT, NULL, 0  }    /* This attribute will be filled later on */
    };


   /* Get TLV from the certificate */
   nError = getTLVFromBuffer( pCertficateBuff, nCertficateBuffLen, &sCertificateTLV);
   if (nError!= S_SUCCESS)
   {
      SLogError("Can not read TLV from certificate buffer.");
      return nError;
   }
   /* Parse the certificate TLV */
   nError = parseCertificate(&sCertificateTLV, &sCertificate);
   if (nError!= S_SUCCESS)
   {
      SLogError("Can not parse the certificate to be verified. [0x%x]", nError);
      return nError;
   }

   /* Get TLV from issuer certificate */
   nError = getTLVFromBuffer( pIssuerCertificateBuff, nIssuerCertificateBuffLen, &sCertificateTLV);
   if (nError!= S_SUCCESS)
   {
      SLogError("Can not read TLV from the issuer certificate.");
      return nError;
   }
   nError = parseCertificate(&sCertificateTLV, &sIssuerCertificate);
   if (nError!= S_SUCCESS)
   {
      SLogError("Can not parse the issuer certificate. [0x%x]", nError);
      return nError;
   }

   /* 
    * Check the certificate's signature against the issuer's public key
    */

   /* Initialize the PKCS#11 interface */
   if ((errorCode = C_Initialize(NULL)) != CKR_OK)
    {
        SLogError("Error: PKCS#11 library initialization failed [0x%x].",errorCode);        
        return errorCode;
    }

    /* Open a session within PKCS#11 */
    if ((errorCode = C_OpenSession(
           1, /* SlotID: service-private keystore */
           CKF_SERIAL_SESSION, /* Session is read-Only (otherwise, you must specify CKF_RW_SESSION) */
           NULL, 
           NULL, 
           &hPKCS11Session)) != CKR_OK)
    {
       SLogError("Error: PKCS#11 open session failed [0x%x]. ", errorCode);
       C_Finalize(NULL);
       return errorCode;
    }

    /* Populate modulus field in the  key template. */
    publicKeyTemplate[4].pValue     = sIssuerCertificate.pModulus; 
    publicKeyTemplate[4].ulValueLen = sIssuerCertificate.nModulusLength;

    /* Populate exponent field in the  key template. */
    publicKeyTemplate[5].pValue     = sIssuerCertificate.pPublicExponent;
    publicKeyTemplate[5].ulValueLen = sIssuerCertificate.nPublicExponentLength;

    /* Import the issuer public key into the PKCS#11 key store. */
    if ((errorCode = C_CreateObject(hPKCS11Session,
        publicKeyTemplate,
        sizeof(publicKeyTemplate)/sizeof(CK_ATTRIBUTE),
        &hPubKey)) != CKR_OK)
    {
       SLogError("Error: PKCS#11 public key creation failed [0x%x]. ",errorCode);                
       goto cleanup;
    }

    /* Set the mechanism */
    sMechanism.mechanism      = sCertificate.nSignatureAlgorithm;
    sMechanism.pParameter     = NULL;
    sMechanism.ulParameterLen = 0 ;
       
   /* Initialize the verify operation */
   if ((errorCode = C_VerifyInit(hPKCS11Session, &sMechanism, hPubKey)) != CKR_OK)
   {
      SLogError("Error: PKCS#11 cannot initialize verify operation [0x%x]", errorCode);
      goto cleanup;
   }

   /* Verify Signature */
   if ((errorCode = C_Verify( hPKCS11Session,
                              sCertificate.pCertDataTBS,
                              sCertificate.nCertDataTBSLength,
                              sCertificate.pSignature,
                              sCertificate.nSignatureLength)) != CKR_OK )
   {
      SLogError("Error: Verification failed [0x%x]", errorCode);
      goto cleanup;
   }
 
   if (errorCode == CKR_OK)
   {
      SLogTrace("Verification was successful.");
   }
   else
   {
      SLogWarning("Verification failed.");
   }

cleanup:

   if (hPubKey != CK_INVALID_HANDLE)
   {
      C_DestroyObject(hPKCS11Session, hPubKey);
   }
   if (hPKCS11Session != CK_INVALID_HANDLE)
   {
      C_CloseSession(hPKCS11Session);
   }
   C_Finalize(NULL);

   return errorCode;
}


/* ----------------------------------------------------------------------------
*   ObjectId management Function
* ---------------------------------------------------------------------------- */
/**
*  Function getNewObjectId:
*  Description:
*           A 32 bits counter, stored in a file "counter", is incremented
*           each time the function getNewObjectId is called. This function is
*           called when we need to store a new file on the file system.
*           The value of this counter will be the object identifier.
*  Output:  uint32_t* pnObjectId = points to the new value of the counter
**/
static S_RESULT getNewObjectId(OUT uint32_t* pnObjectId)
{
   S_RESULT       nError;
   uint32_t       nCounter;
   uint32_t       nLength;
   S_HANDLE       hFileHandle;

retry:
   /* Try to open the counter file. If the file does not exist, try to create it.
      Don't specify any sharing flags so that the error S_ERROR_ACCESS_CONFLICT
      will be returned if another instance of the service is currently using
      the file. */
   nError = SFileOpen(
      S_FILE_STORAGE_PRIVATE,
      COUNTER_FILENAME,
      S_FILE_FLAG_ACCESS_READ 
        | S_FILE_FLAG_ACCESS_WRITE 
        | S_FILE_FLAG_CREATE,
      0,
      &hFileHandle);
   if (nError == S_ERROR_ACCESS_CONFLICT)
   {
      /* Another instance is currently using the file. */
      SLogWarning("SFileOpen() failed with S_ERROR_ACCESS_CONFLICT: retrying.", nError);
      SThreadYield();
      goto retry;
   }
   else if (nError != S_SUCCESS)
   {
      SLogError("SFileOpen() failed (%08x).", nError);
      return nError;
   }
      /* File successfully opened. Read its content */
   nError = SFileRead(hFileHandle, (uint8_t*)&nCounter, 4, &nLength);
   if (nError != S_SUCCESS)
   {
      SLogError("SFileRead() failed (%08x).", nError);
      goto cleanup;
   }
   if (nLength == 0)
   {
      /* File is empty. This can happen if it has just been created. */
      nCounter = 0;
   }
   else if (nLength != 4)
   {
      /* There is definitely something wrong here because we never write
         but exactly 4 bytes in the file */
      SLogError("SFileRead(): File size is wrong.");
      nError = S_ERROR_CRITICAL;
      goto cleanup;
   }

   /* Set the file pointer at the beginning of the file */
   nError = SFileSeek(hFileHandle, 0, S_FILE_SEEK_SET);
   if (nError != S_SUCCESS)
   {
      SLogError("SFileSeek() failed (%08x).", nError);
      goto cleanup;
   }
   /* Now, write the new counter value */
   nCounter++;

   /* Update the file with the new counter value. Note that this is 
      an atomic operation */
   nError =  SFileWrite(hFileHandle, (uint8_t*)&nCounter, 4);
   if (nError != S_SUCCESS)
   {
      SLogError("SFileWrite() failed (%08x).", nError);
      goto cleanup;
   }
   SHandleClose(hFileHandle);

   /* New id = new counter value */
   *pnObjectId = nCounter;

   return S_SUCCESS;

cleanup:
   SHandleClose(hFileHandle);
   return nError;
}

/* ----------------------------------------------------------------------------
*   Certificate management Function
* ---------------------------------------------------------------------------- */
/**
*  Function getCertFromId:
*  Description:
*           Get the certificate from the certificate identifier.
*  Input :  SESSION_CONTEXT*  pSessionContext   = session context
*           uint32_t          nObjectId         = certificate identifier
*  Output:  uint8_t*          ppCertificate        = points to the certificate
*           uint32_t*         pnCertificateLength  = points to the certificate length
**/
/* Caution: This function allocates pCertificate if it succeeds  */
static S_RESULT getCertFromId(
                              IN   SESSION_CONTEXT*  pSessionContext,    
                              IN   uint32_t          nObjectId,
                              OUT  uint8_t**         ppCertificate,
                              OUT  uint32_t*         pnCertificateLength)
{
   
   S_RESULT        nError;
   char            pObjectName[9]; /* The file name can contain a maximum of 8 characters plus the terminal 0 */
   uint32_t        nCount;  
   S_HANDLE        hFileHandle;

   S_VAR_NOT_USED(pSessionContext);

   *ppCertificate = NULL;
   *pnCertificateLength = 0;

   /* Convert integer to string */
    CertIDToFileName(nObjectId, pObjectName);
   
retry:
   /* Open certificate for reading. Don't specify the sharing flag so
      that the error code S_ERROR_ACCESS_CONFLICT will be returned
      if another instance of the service currently uses the
      file. */
   nError = SFileOpen( S_FILE_STORAGE_PRIVATE,
       (char*)pObjectName,
       S_FILE_FLAG_ACCESS_READ,
       0,
       &hFileHandle);
   if (nError == S_ERROR_ACCESS_CONFLICT)
   {
      /* Another service instances is currently using the file. */
      SLogWarning("SFileOpen() failed with S_ERROR_ACCESS_CONFLICT: retrying.", nError);
      SThreadYield();
      goto retry;
   }   
   else if (nError != S_SUCCESS)
   {
       SLogError("SFileOpen() failed (%08x)." ,nError);
       return nError;
   }

   /* SFileOpen successful. We have effectively locked the file until we close the
      file handle */

   /* Get Certificate Length  */
   nError = SFileGetSize( S_FILE_STORAGE_PRIVATE, pObjectName, pnCertificateLength );
   if(nError!= S_SUCCESS)
   {
       SLogError("SFileGetSize() failed (%08x)." ,nError);
       goto cleanup;
   }

   /* Allocate memory for the certificate */
   *ppCertificate = SMemAlloc(*pnCertificateLength);
   if (*ppCertificate == NULL)
   {
       SLogError("Can not allocate certificate.");
       nError = S_ERROR_OUT_OF_MEMORY;
       goto cleanup;
   }

   /* Read the certificate */
   nError = SFileRead(hFileHandle, 
       *ppCertificate,
       *pnCertificateLength ,
       &nCount);
   if(nError != S_SUCCESS)
   {
       SLogError("SFileRead() failed." ,nError);
       SMemFree(*ppCertificate);
       goto cleanup;
   }

cleanup:   
   SHandleClose(hFileHandle);
   return nError;


}

/* ----------------------------------------------------------------------------
 *                             Main functions
 * ---------------------------------------------------------------------------- */

/**
 *  Function serviceInstallCertificate:
 *  Description:
 *           This function stores a certificate on the file system, parses
 *           the certificate to get the distinguished name fields, and returns to the
 *           client the certificate identifier and the distinguished name fields.
 *           The client must have the effective manager rights to call this function.
 *  Input :  SESSION_CONTEXT* pSessionContext = session context
 *           S_HANDLE hDecoder = handle of a decoder containing the certificate to install
 *           S_HANDLE hEncoder = handle of an encoder containing the certificate identifier
 *                             and the distinguished name fields.
 **/
static S_RESULT serviceInstallCertificate(SESSION_CONTEXT* pSessionContext, uint32_t nParamTypes, IN OUT S_PARAM pParams[4])
{
   S_RESULT       nError;
   uint8_t*       pCertificate;
   uint32_t       nCertificateLength;
   uint32_t       nObjectId;
   char           pObjectName[9];
   S_HANDLE       hFileHandle;

   S_VAR_NOT_USED(pSessionContext);

   SLogTrace("Start serviceInstallCertificate().");

   if ( (S_PARAM_TYPE_GET(nParamTypes, 0) != S_PARAM_TYPE_VALUE_OUTPUT)
      ||(S_PARAM_TYPE_GET(nParamTypes, 1) != S_PARAM_TYPE_MEMREF_OUTPUT)
      ||(pParams[1].memref.pBuffer == NULL)
      ||(pParams[1].memref.nSize == 0)
      ||(S_PARAM_TYPE_GET(nParamTypes, 2) != S_PARAM_TYPE_MEMREF_INPUT)
      ||(pParams[2].memref.pBuffer == NULL)
      ||(pParams[2].memref.nSize == 0)
      )
   {
      return S_ERROR_BAD_PARAMETERS;
   }

   /* Check that the client has manager rights */
   if (!pSessionContext->bManagerRights)
   {
      SLogError("The client must have manager rights.");
      return S_ERROR_ACCESS_DENIED;
   }

   /* Read the certificate */
   pCertificate       = pParams[2].memref.pBuffer;
   nCertificateLength = pParams[2].memref.nSize;
   if(pCertificate == NULL)
   {
      SLogError("SDecoderReadUint8Array() failed.");
      return S_ERROR_BAD_PARAMETERS;
   }

   /* Get a new Certificate Identifier = a new Object Id */
   nError = getNewObjectId(&nObjectId);
   if (nError!= S_SUCCESS)
   {
      SLogError("getNewObjectId() failed (%08x).", nError);
      SMemFree(pCertificate);
      return nError;
   }

   /* Send the certificate identifier to the client */
   SLogTrace("CERTIFICATE IDENTIFIER : (0x%08X).", nObjectId);
   pParams[0].value.a = nObjectId;

   /* Get, Print and Encode Distinguished Name */
   nError = parseAndEncodeDistinguishedName(pCertificate, nCertificateLength, pParams);
   if (nError!= S_SUCCESS)
   {
      SLogError("parseAndEncodeDistinguishedName() failed (%08x).", nError);
      if (nError == S_ERROR_BAD_FORMAT)
      {
         SLogError("The certificate has a bad format (distinguished name not found).");
         SLogError("CAUTION: Consequently the certificate is not installed.");
      }
      SMemFree(pCertificate);
      return nError;
   }
  
    /* Store the certificate */  
   CertIDToFileName(nObjectId, pObjectName);
   
   nError = SFileOpen(
       S_FILE_STORAGE_PRIVATE,
       pObjectName,
       S_FILE_FLAG_ACCESS_WRITE | S_FILE_FLAG_CREATE | S_FILE_FLAG_EXCLUSIVE,
       0,
       &hFileHandle);
   /* Note that there shouldn't be any access conflict here because
      we've just generated a new fresh object identifier */
   if (nError != S_SUCCESS)
   {
       SLogError("SFileOpen() failed (%08x).", nError);
       SMemFree(pCertificate);
       return nError;
   }

   /* write data to file */
   nError = SFileWrite(hFileHandle, pCertificate, nCertificateLength);
   if(nError != S_SUCCESS)
   {
       SLogError("Cannot write to file [error 0x%X]", nError);
   }

   SHandleClose(hFileHandle);
   return nError;
}


/**
 *  Function serviceListAllCertificates:
 *  Description:
 *           This function returns the identifiers list of the certificates that are
 *           present on the file system.
 *           Every client can call this function (no rights are required).
 *  Input :  SESSION_CONTEXT* pSessionContext = session context
 *           S_HANDLE hEncoder = handle of an encoder containing the list of all
 *                             certificate identifiers.
 **/
static S_RESULT serviceListAllCertificates(SESSION_CONTEXT* pSessionContext, uint32_t nParamTypes, IN OUT S_PARAM pParams[4])
{
   S_RESULT       nError;   
   uint32_t       nCertId = 0;
   S_HANDLE       hFileEnumeration;
   S_FILE_INFO*   pFileInfo;
   uint32_t       i = 0;
   
   S_VAR_NOT_USED(pSessionContext);

   SLogTrace("Start serviceListAllCertificates().");

   if ( (S_PARAM_TYPE_GET(nParamTypes, 0) != S_PARAM_TYPE_MEMREF_OUTPUT)
      ||(pParams[0].memref.pBuffer == NULL)
      ||(pParams[0].memref.nSize == 0))
   {
      return S_ERROR_BAD_PARAMETERS;
   }

   /* No need to check the manager rights : every one can list the certificates */

   nError = SFileEnumerationStart(
       S_FILE_STORAGE_PRIVATE,
       NULL,  /* all files are listed */
       0,   /* Reserved for future use */
       0,   /* Reserved for future use */
       &hFileEnumeration);
   if (nError != S_SUCCESS)
   {
       SLogError("SFileEnumerationStart() failed (%08x).", nError);
       return nError;
   }

   while((nError = SFileEnumerationGetNext(hFileEnumeration, &pFileInfo)) == S_SUCCESS)
   {
       nError = FileNameToCertID(pFileInfo->pName, &nCertId);
       if (nError != S_ERROR_BAD_FORMAT )
       {
          /* Note that the file "counter" will fail to convert to a certificate identifier */
          SLogTrace("(0x%08X)",nCertId);
          /* Send the certificate identifier to the client */
          if (i*4 > pParams[0].memref.nSize)
          {
             nError = S_ERROR_SHORT_BUFFER;
       }
          else
          {
            ((uint32_t *)pParams[0].memref.pBuffer)[i] = nCertId;
   }
          i++;
       }
   }

   pParams[0].memref.nSize = i*4;
   if(nError == S_ERROR_ITEM_NOT_FOUND)
   {
       nError = S_SUCCESS;
   }
   
   SHandleClose(hFileEnumeration);

   return nError;
}


/**
 *
 *  Function serviceDeleteCertificate:
 *  Description:
 *           This function removes from the file system the certificate with the
 *           identifier given in input.
 *           The client must have the effective manager rights to call this function.
 *  Input :  SESSION_CONTEXT* pSessionContext = session context
 *           S_HANDLE hDecoder = handle of a decoder containing the identifier of the
 *                             certificate to delete.
 *
 **/
static S_RESULT serviceDeleteCertificate(SESSION_CONTEXT* pSessionContext, uint32_t nParamTypes, IN OUT S_PARAM pParams[4])
{
   S_RESULT          nError;
   uint32_t          nObjectId;
   char              pObjectName[9];
   S_HANDLE          hFileHandle;

   S_VAR_NOT_USED(pSessionContext);

   SLogTrace("Start serviceDeleteCertificate().");

   if (S_PARAM_TYPE_GET(nParamTypes, 0) != S_PARAM_TYPE_VALUE_INPUT)
   {
      return S_ERROR_BAD_PARAMETERS;
   }

   /* Check that the client has manager rights */
   if (!pSessionContext->bManagerRights)
   {
      SLogError("The client must have manager rights.");
      return S_ERROR_ACCESS_DENIED;
   }

   /* Read the certificate ID  */
   nObjectId = pParams[0].value.a;

   /* Convert integer ID to string ID */
   CertIDToFileName(nObjectId, pObjectName);

   /* delete the certificate */
retry:   
   nError = SFileOpen(
       S_FILE_STORAGE_PRIVATE,
       (char*)pObjectName,       
       S_FILE_FLAG_ACCESS_WRITE_META,
       0,
       &hFileHandle);
   if (nError == S_ERROR_ACCESS_CONFLICT)
   {
      SLogWarning("SFileOpen() failed with S_ERROR_ACCESS_CONFLICT. Retrying.", nError);
      SThreadYield();         
      goto retry;
   }
   else if (nError == S_ERROR_ITEM_NOT_FOUND)
   {
       SLogError("The certificate is not installed, so it cannot be removed.");
       return nError;
   }
   else if (nError != S_SUCCESS)
   {
       SLogError("SFileOpen() failed (%08x).", nError);
       return nError;
   }

   nError = SFileCloseAndDelete(hFileHandle);
   if (nError != S_SUCCESS)
   {
       SLogError("SFileCloseAndDelete() failed (%08x).", nError);
       SHandleClose(hFileHandle);
       return nError;
   } 

   return S_SUCCESS;
}

/**
*  Function serviceDeleteAllCertificates:
*  Description:
*           This function deletes all the installed certificates.
*           The client must have the effective manager rights to call this function.
*  Input :  SESSION_CONTEXT* pSessionContext = session context
*
**/
static S_RESULT serviceDeleteAllCertificates(SESSION_CONTEXT* pSessionContext)
{
   S_RESULT       nError;   
   S_HANDLE       hFileEnumeration;
   S_FILE_INFO*   pFileInfo;
   S_HANDLE       hFileHandle;
   
   S_VAR_NOT_USED(pSessionContext);

   SLogTrace("Start serviceDeleteAllCertificates().");

   /* Check that the client has manager rights */
   if (!pSessionContext->bManagerRights)
   {
      SLogError("The client must have manager rights.");
      return S_ERROR_ACCESS_DENIED;
   }

   /* Start the file enumeration */
   nError = SFileEnumerationStart(
       S_FILE_STORAGE_PRIVATE,
       NULL,  /* all files are listed */
       0,   /* Reserved for future use */
       0,   /* Reserved for future use */
       &hFileEnumeration);
   if (nError != S_SUCCESS)
   {
       SLogError("SFileEnumerationStart() failed (%08x).", nError);
       return nError;
   }

   while(SFileEnumerationGetNext(hFileEnumeration, &pFileInfo)  !=  S_ERROR_ITEM_NOT_FOUND)
   {
       /* Delete each file except "counter" */
      if (SMemCompare(pFileInfo, COUNTER_FILENAME, sizeof(COUNTER_FILENAME)) == 0)
      {
         continue;
      }
retry:   
      nError = SFileOpen(
          S_FILE_STORAGE_PRIVATE,
          pFileInfo->pName,
          S_FILE_FLAG_ACCESS_WRITE_META,
          0,
          &hFileHandle);
      if (nError == S_ERROR_ACCESS_CONFLICT)
      {
         SLogError("SFileOpen() failed with S_ERROR_ACCESS_CONFLICT. Retrying.", nError);
         SThreadYield();
         goto retry;
      }          
      else if (nError != S_SUCCESS)
      {
         SLogError("SFileOpen() failed (%08x).", nError);
         return nError;
      }

      nError = SFileCloseAndDelete(hFileHandle);
      if (nError != S_SUCCESS)
      {
         SLogError("SFileCloseAndDelete() failed (%08x).", nError);
         SHandleClose(hFileHandle);
         return nError;
      }
   }

   return S_SUCCESS;
}

/**
 *
 *  Function serviceReadCertificateDistinguishedName:
 *  Description:
 *           This function partially parses a certificate to get the distinguished name
 *           and returns the distinguished name fields to the client.
 *           Every client can call this function (no rights are required).
 *  Input :  SESSION_CONTEXT* pSessionContext = session context
 *           S_HANDLE hDecoder = handle of a decoder containing the identifier of the
 *                             certificate to partially parse.
 *           S_HANDLE hEncoder = handle of a decoder containing the distinguished name fields.
 *
 **/
static S_RESULT serviceReadCertificateDistinguishedName(SESSION_CONTEXT* pSessionContext, uint32_t nParamTypes, IN OUT S_PARAM pParams[4])
{
   S_RESULT          nError;
   uint32_t          nObjectId;
   uint8_t*          pCertificate;
   uint32_t          nCertificateLength;

   SLogTrace("Start serviceReadCertificateDistinguishedName().");

   /* No need to check the manager rights : every one can read the certificate DN */
   if ( (S_PARAM_TYPE_GET(nParamTypes, 0) != S_PARAM_TYPE_VALUE_INPUT)
      ||(S_PARAM_TYPE_GET(nParamTypes, 1) != S_PARAM_TYPE_MEMREF_OUTPUT)
      ||(pParams[1].memref.pBuffer == NULL)
      ||(pParams[1].memref.nSize == 0)
      )
   {
      return S_ERROR_BAD_PARAMETERS;
   }
   /* Read the certificate ID  */
   nObjectId = pParams[0].value.a;

   /* Get the certificate */
   pCertificate = NULL;
   nError = getCertFromId(pSessionContext, nObjectId, &pCertificate, &nCertificateLength);
   if (nError!= S_SUCCESS)
   {
      SLogError("getCertFromId() failed (0x%80X).", nError);
      return nError; /* no need to free pCertificate */
   }

   /* Get, print and encode Distinguished Name */
   nError = parseAndEncodeDistinguishedName(pCertificate, nCertificateLength, pParams);
   if (nError!= S_SUCCESS)
   {
      SLogError("parseAndEncodeDistinguishedName() failed (%08x).", nError);
      if (nError == S_ERROR_BAD_FORMAT)
      {
         SLogError("The certificate has a bad format (distinguished name not found).");
      }
   }

   SMemFree(pCertificate);
   return nError;
}
/**
 *  Function serviceGetCertificate:
 *  Description:
 *           This function performs the following steps:
 *           - receives the identifier of a certificate from the client,
 *           - finds the matching certificate on the file system,
 *           - sends the certificate length and the certificate to the client.
 *  Input :  SESSION_CONTEXT* pSessionContext = session context
 *           S_HANDLE hDecoder = handle of a decoder containing the identifier of the
 *                             certificate to get, and a memory reference pointing to the buffer
 *                             where the certificate will be copied.
 *           S_HANDLE hEncoder = handle of a decoder containing the certificate length.
 * 
 **/
static S_RESULT serviceGetCertificate(SESSION_CONTEXT* pSessionContext, uint32_t nParamTypes, IN OUT S_PARAM pParams[4])
{
   S_RESULT          nError;
   uint32_t          nObjectId;
   uint8_t*          pCertificate;
   uint32_t          nCertificateLength;
   
   SLogTrace("Start serviceGetCertificate().");

   if ( (S_PARAM_TYPE_GET(nParamTypes, 0) != S_PARAM_TYPE_VALUE_INPUT)
      ||(S_PARAM_TYPE_GET(nParamTypes, 1) != S_PARAM_TYPE_MEMREF_OUTPUT)
      ||(pParams[1].memref.pBuffer == NULL)
      ||(pParams[1].memref.nSize == 0)
      )
   {
      return S_ERROR_BAD_PARAMETERS;
   }
   /* No need to check the manager rights : every one can get a certificate */

   /* Read the certificate ID  */
   nObjectId = pParams[0].value.a;

   /* Get the certificate */
   pCertificate = NULL;
   nError = getCertFromId(pSessionContext, nObjectId, &pCertificate, &nCertificateLength);
   if (nError!= S_SUCCESS)
   {
      SLogError("getCertFromId() failed (0x%80X).", nError);
      return nError; /* no need to free pCertificate */
   }

   /* Encode the certificate */
   if (pParams[1].memref.nSize >= nCertificateLength)
   {
      memcpy(pParams[1].memref.pBuffer, pCertificate, nCertificateLength);
   }
   else
   {
      nError = S_ERROR_SHORT_BUFFER;
   }
   pParams[1].memref.nSize = nCertificateLength;
   SMemFree(pCertificate);
   return nError;
}


/**
 *  Function serviceVerifyCertificate:
 *  Description:
 *           This function:
 *               - receives two certificate identifiers from the client,
 *               - retrieves the two certifcates,
 *               - checks if the first certificate is signed by the second certificate,
 *               - sends the verification verdict to the client
 *  Input:   SESSION_CONTEXT* pSessionContext = session context
 *           S_HANDLE hDecoder = handle of a decoder containing the identifier of the
 *                             certificate to get, and a memory reference pointing to the buffer
 *                             where the certificate will be copied.
 *           S_HANDLE hEncoder = handle of a decoder containing the certificate length.
 *          
 **/
static S_RESULT serviceVerifyCertificate(SESSION_CONTEXT* pSessionContext, uint32_t nParamTypes, IN OUT S_PARAM pParams[4])
{
   uint8_t*          pCertificate;
   uint32_t          nCertificateLength;
   uint8_t*          pIssuerCertificate;
   uint32_t          nIssuerCertificateLength;
   uint32_t          nObjectId;
   S_RESULT          nError;
   
   SLogTrace("Start serviceVerifyCertificate().");

   if ( (S_PARAM_TYPE_GET(nParamTypes, 0) != S_PARAM_TYPE_VALUE_INPUT)
      ||(S_PARAM_TYPE_GET(nParamTypes, 1) != S_PARAM_TYPE_VALUE_OUTPUT))
   {
      return S_ERROR_BAD_PARAMETERS;
   }
   /* No need to check the manager rights : every one can check a certificate */

   /* Read the certificate ID  */
   nObjectId = pParams[0].value.a;

   /* Get the certificate */
   pCertificate = NULL;
   nError = getCertFromId(pSessionContext,  nObjectId, &pCertificate, &nCertificateLength);
   if (nError!= S_SUCCESS)
   {
      SLogError("getCertFromId() failed (0x%80X).", nError);
      return nError; 
   }

   
   /* Read the Issuer certificate ID  */
   nObjectId = pParams[0].value.b;
  
   /* Get the Issuer certificate */
   pIssuerCertificate = NULL;
   nError = getCertFromId(pSessionContext,  nObjectId, &pIssuerCertificate, &nIssuerCertificateLength);
   if (nError!= S_SUCCESS)
   {
      SLogError("getCertFromId() failed (0x%80X).", nError);
      return nError; 
   }

    nError = verifySignature(pCertificate,nCertificateLength,
                             pIssuerCertificate, nIssuerCertificateLength);
   
   
    /* Encode the answer */
    pParams[1].value.a = nError;
    SMemFree(pCertificate);   
    SMemFree(pIssuerCertificate);   
    return nError;

}


/* ----------------------------------------------------------------------------
*   Service Entry Points
* ---------------------------------------------------------------------------- */
/**
 *  Function SRVXCreate:
 *  Description:
 *        The function SSPICreate is the service constructor, which the system
 *        calls when it creates a new instance of the service.
 *        Here this function implements nothing.
 **/
S_RESULT SRVX_EXPORT SRVXCreate(void)
{
   SLogTrace("SRVXCreate");

   return S_SUCCESS;
}

/**
 *  Function SRVXDestroy:
 *  Description:
 *        The function SSPIDestroy is the service destructor, which the system
 *        calls when the instance is being destroyed.
 *        Here this function implements nothing.
 **/
void SRVX_EXPORT SRVXDestroy(void)
{
   SLogTrace("SRVXDestroy");

   return;
}

/**
 *  Function SRVXOpenClientSession:
 *  Description:
 *        The system calls the function SSPIOpenClientSession when a new client
 *        connects to the service instance.
 *        Here this function performs the following steps:
 *        - Print client properties;       
 *        - Find out the effective manager rights;
 *        - Allocate and populate the session context with the effective manager rights.
 **/
S_RESULT SRVX_EXPORT SRVXOpenClientSession(
   uint32_t nParamTypes,
   IN OUT S_PARAM pParams[4],
   OUT void** ppSessionContext )
{
   S_RESULT             nError;
   bool                 bManagerRights;
   SESSION_CONTEXT *    pSessionContext;

   S_VAR_NOT_USED(nParamTypes);
   S_VAR_NOT_USED(pParams);

   SLogTrace("SRVXOpenClientSession");

   *ppSessionContext = NULL;

   /* Print client properties */
   nError = printClientPropertiesAndUUID();
   if (nError != S_SUCCESS)
   {
      SLogError("printClientPropertiesAndUUID() failed (%08x).", nError);
      return nError;
   }

  
   /* Check effective manager rights (authentication + potential manager rights in the client manifest) */
   bManagerRights = getClientEffectiveManagerRights();

   /* Allocate session context */
   pSessionContext = SMemAlloc(sizeof(SESSION_CONTEXT));
   if(pSessionContext == NULL)
   {
      SLogTrace("SSPIOpenClientSession : session context allocation failed");
      return S_ERROR_OUT_OF_MEMORY;
   }

   /* Populate session context */
   pSessionContext->bManagerRights = bManagerRights;

   *ppSessionContext = pSessionContext;

   return S_SUCCESS;
}

/**
 *  Function SRVXCloseClientSession:
 *  Description:
 *        The system calls this function to indicate that a session is being closed.
 **/
void SRVX_EXPORT SRVXCloseClientSession(IN OUT void* pSessionContext)
{
   SLogTrace("SRVXCloseClientSession");

   /* Free Session context */
   SMemFree(pSessionContext);
}

/**
 *  Function SRVXInvokeCommand:
 *  Description:
 *        The system calls this function when the client invokes a command within
 *        a session of the instance.
 *        Here this function perfoms a switch on the command Id received as input,
 *        and returns the main function matching the command id.
 *
 **/
S_RESULT SRVX_EXPORT SRVXInvokeCommand(IN OUT void* pSessionContext,
                                       uint32_t nCommandID,
                                       uint32_t nParamTypes,
                                       IN OUT S_PARAM pParams[4])
{
   SLogTrace("SRVXInvokeCommand");

   switch(nCommandID)
   {
   case CMD_INSTALL_CERT:
      return serviceInstallCertificate(pSessionContext, nParamTypes, pParams);
   case CMD_DELETE_CERT:
      return serviceDeleteCertificate(pSessionContext, nParamTypes, pParams);
   case CMD_DELETE_ALL:
      return serviceDeleteAllCertificates(pSessionContext);
   case CMD_LIST_ALL:
      return serviceListAllCertificates(pSessionContext, nParamTypes, pParams);
   case CMD_READ_DN:
      return serviceReadCertificateDistinguishedName(pSessionContext, nParamTypes, pParams);
   case CMD_GET_CERT:
      return serviceGetCertificate(pSessionContext, nParamTypes, pParams);
   case CMD_VERIFY_CERT:
       return serviceVerifyCertificate(pSessionContext, nParamTypes, pParams);
   default:
      SLogError("invalid command ID: 0x%X", nCommandID);
      return S_ERROR_BAD_PARAMETERS;
   }
}
