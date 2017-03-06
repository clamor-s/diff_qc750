/*
 * Copyright (c) 2007-2011 Trusted Logic S.A.
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

/*
* Copyright (c) 2011-2012 NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property
* and proprietary rights in and to this software and related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA Corporation is strictly prohibited.
*/

#include <stdio.h>
#include <string.h>

#include "sddi.h"
#include "ssdi_hw_ext.h"
#include "oemcrypto_secure_protocol.h"
#include "otf_secure_protocol.h"
#include "OEMCrypto.h"

/*
* ENABLE_INSTALL_KEYBOX
*
* This define is for new wv keybox provisioning method.
*/
//#define ENABLE_INSTALL_KEYBOX
#ifdef ENABLE_INSTALL_KEYBOX
#define FILENAME "wv_keybox.dat"
#endif

/*Session handle and key objects*/
typedef struct {
    CK_SESSION_HANDLE hCryptoSession;
    CK_SESSION_HANDLE hOTFSession;
    CK_OBJECT_HANDLE  hWidevineKey;
    CK_OBJECT_HANDLE  hEMMKey;
    CK_OBJECT_HANDLE  hControlWord;
    uint8_t hdeviceID[32];
    uint8_t hdeviceKey[16];
    uint8_t hkeyData[72];
    uint8_t hmagic[4];
    uint8_t hcrc[4];
    CK_ULONG FirstPacketOffset;
} SESSION_CONTEXT;

/*Service ID for using OTF driver*/
static const S_UUID OTF_UUID = SERVICE_OTF_UUID;
static const uint32_t crc32Table[256] = {
    0x00000000UL, 0x04c11db7UL, 0x09823b6eUL, 0x0d4326d9UL,
    0x130476dcUL, 0x17c56b6bUL, 0x1a864db2UL, 0x1e475005UL,
    0x2608edb8UL, 0x22c9f00fUL, 0x2f8ad6d6UL, 0x2b4bcb61UL,
    0x350c9b64UL, 0x31cd86d3UL, 0x3c8ea00aUL, 0x384fbdbdUL,
    0x4c11db70UL, 0x48d0c6c7UL, 0x4593e01eUL, 0x4152fda9UL,
    0x5f15adacUL, 0x5bd4b01bUL, 0x569796c2UL, 0x52568b75UL,
    0x6a1936c8UL, 0x6ed82b7fUL, 0x639b0da6UL, 0x675a1011UL,
    0x791d4014UL, 0x7ddc5da3UL, 0x709f7b7aUL, 0x745e66cdUL,
    0x9823b6e0UL, 0x9ce2ab57UL, 0x91a18d8eUL, 0x95609039UL,
    0x8b27c03cUL, 0x8fe6dd8bUL, 0x82a5fb52UL, 0x8664e6e5UL,
    0xbe2b5b58UL, 0xbaea46efUL, 0xb7a96036UL, 0xb3687d81UL,
    0xad2f2d84UL, 0xa9ee3033UL, 0xa4ad16eaUL, 0xa06c0b5dUL,
    0xd4326d90UL, 0xd0f37027UL, 0xddb056feUL, 0xd9714b49UL,
    0xc7361b4cUL, 0xc3f706fbUL, 0xceb42022UL, 0xca753d95UL,
    0xf23a8028UL, 0xf6fb9d9fUL, 0xfbb8bb46UL, 0xff79a6f1UL,
    0xe13ef6f4UL, 0xe5ffeb43UL, 0xe8bccd9aUL, 0xec7dd02dUL,
    0x34867077UL, 0x30476dc0UL, 0x3d044b19UL, 0x39c556aeUL,
    0x278206abUL, 0x23431b1cUL, 0x2e003dc5UL, 0x2ac12072UL,
    0x128e9dcfUL, 0x164f8078UL, 0x1b0ca6a1UL, 0x1fcdbb16UL,
    0x018aeb13UL, 0x054bf6a4UL, 0x0808d07dUL, 0x0cc9cdcaUL,
    0x7897ab07UL, 0x7c56b6b0UL, 0x71159069UL, 0x75d48ddeUL,
    0x6b93dddbUL, 0x6f52c06cUL, 0x6211e6b5UL, 0x66d0fb02UL,
    0x5e9f46bfUL, 0x5a5e5b08UL, 0x571d7dd1UL, 0x53dc6066UL,
    0x4d9b3063UL, 0x495a2dd4UL, 0x44190b0dUL, 0x40d816baUL,
    0xaca5c697UL, 0xa864db20UL, 0xa527fdf9UL, 0xa1e6e04eUL,
    0xbfa1b04bUL, 0xbb60adfcUL, 0xb6238b25UL, 0xb2e29692UL,
    0x8aad2b2fUL, 0x8e6c3698UL, 0x832f1041UL, 0x87ee0df6UL,
    0x99a95df3UL, 0x9d684044UL, 0x902b669dUL, 0x94ea7b2aUL,
    0xe0b41de7UL, 0xe4750050UL, 0xe9362689UL, 0xedf73b3eUL,
    0xf3b06b3bUL, 0xf771768cUL, 0xfa325055UL, 0xfef34de2UL,
    0xc6bcf05fUL, 0xc27dede8UL, 0xcf3ecb31UL, 0xcbffd686UL,
    0xd5b88683UL, 0xd1799b34UL, 0xdc3abdedUL, 0xd8fba05aUL,
    0x690ce0eeUL, 0x6dcdfd59UL, 0x608edb80UL, 0x644fc637UL,
    0x7a089632UL, 0x7ec98b85UL, 0x738aad5cUL, 0x774bb0ebUL,
    0x4f040d56UL, 0x4bc510e1UL, 0x46863638UL, 0x42472b8fUL,
    0x5c007b8aUL, 0x58c1663dUL, 0x558240e4UL, 0x51435d53UL,
    0x251d3b9eUL, 0x21dc2629UL, 0x2c9f00f0UL, 0x285e1d47UL,
    0x36194d42UL, 0x32d850f5UL, 0x3f9b762cUL, 0x3b5a6b9bUL,
    0x0315d626UL, 0x07d4cb91UL, 0x0a97ed48UL, 0x0e56f0ffUL,
    0x1011a0faUL, 0x14d0bd4dUL, 0x19939b94UL, 0x1d528623UL,
    0xf12f560eUL, 0xf5ee4bb9UL, 0xf8ad6d60UL, 0xfc6c70d7UL,
    0xe22b20d2UL, 0xe6ea3d65UL, 0xeba91bbcUL, 0xef68060bUL,
    0xd727bbb6UL, 0xd3e6a601UL, 0xdea580d8UL, 0xda649d6fUL,
    0xc423cd6aUL, 0xc0e2d0ddUL, 0xcda1f604UL, 0xc960ebb3UL,
    0xbd3e8d7eUL, 0xb9ff90c9UL, 0xb4bcb610UL, 0xb07daba7UL,
    0xae3afba2UL, 0xaafbe615UL, 0xa7b8c0ccUL, 0xa379dd7bUL,
    0x9b3660c6UL, 0x9ff77d71UL, 0x92b45ba8UL, 0x9675461fUL,
    0x8832161aUL, 0x8cf30badUL, 0x81b02d74UL, 0x857130c3UL,
    0x5d8a9099UL, 0x594b8d2eUL, 0x5408abf7UL, 0x50c9b640UL,
    0x4e8ee645UL, 0x4a4ffbf2UL, 0x470cdd2bUL, 0x43cdc09cUL,
    0x7b827d21UL, 0x7f436096UL, 0x7200464fUL, 0x76c15bf8UL,
    0x68860bfdUL, 0x6c47164aUL, 0x61043093UL, 0x65c52d24UL,
    0x119b4be9UL, 0x155a565eUL, 0x18197087UL, 0x1cd86d30UL,
    0x029f3d35UL, 0x065e2082UL, 0x0b1d065bUL, 0x0fdc1becUL,
    0x3793a651UL, 0x3352bbe6UL, 0x3e119d3fUL, 0x3ad08088UL,
    0x2497d08dUL, 0x2056cd3aUL, 0x2d15ebe3UL, 0x29d4f654UL,
    0xc5a92679UL, 0xc1683bceUL, 0xcc2b1d17UL, 0xc8ea00a0UL,
    0xd6ad50a5UL, 0xd26c4d12UL, 0xdf2f6bcbUL, 0xdbee767cUL,
    0xe3a1cbc1UL, 0xe760d676UL, 0xea23f0afUL, 0xeee2ed18UL,
    0xf0a5bd1dUL, 0xf464a0aaUL, 0xf9278673UL, 0xfde69bc4UL,
    0x89b8fd09UL, 0x8d79e0beUL, 0x803ac667UL, 0x84fbdbd0UL,
    0x9abc8bd5UL, 0x9e7d9662UL, 0x933eb0bbUL, 0x97ffad0cUL,
    0xafb010b1UL, 0xab710d06UL, 0xa6322bdfUL, 0xa2f33668UL,
    0xbcb4666dUL, 0xb8757bdaUL, 0xb5365d03UL, 0xb1f740b4UL
};

/*--------------------------------------------------------
 * Internal APIs
 *-------------------------------------------------------*/

static void char_to_bin(uint8_t *s, int size)
{
    int i;
    char c;
    char l;
    char h;
    uint8_t *ptr = s;

    for (i = 0; i < size; i++) {
        c = *ptr;
        if (c >= 'a' && c <= 'f') h = c - 'a' + 0xa;
        else if (c <= '9' && c >= '0') h = c - '0' + 0x0;
        else {
            _SLogTrace("Error in converting from ASCII to binary");
            return;
        }
        c = *(ptr+1);
        if (c >= 'a' && c <= 'f') l = c - 'a' + 0xa;
        else if (c <= '9' && c >= '0') l = c - '0' + 0x0;
        else {
            _SLogTrace("Error in converting from ASCII to binary");
            return;
        }
        ptr+=2;
        *s = (h<<4)|l;
        s++;
    }
}

static uint16_t bswap_16(uint16_t x)
{
    return(x>>8) | (x<<8);
}

static uint32_t bswap_32(uint32_t x)
{
    return(bswap_16(x&0xffff)<<16) | (bswap_16(x>>16));
}

static uint32_t get_crc32(const uint8_t * buffer, unsigned long size)
{
    uint32_t crc = 0xffffffff;
    const uint8_t *p = buffer;
    for (; size > 0; ++p, --size )
        crc = ( crc << 8 ) ^ crc32Table[ ( crc >> 24 ) ^ *p ];
    return crc;
}

/*
 * Retrieve TF_SBK key
 */
static S_RESULT getWrappingKey(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE *phKey)
{
    S_RESULT nError;
    const CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
    const CK_KEY_TYPE keyType = CKK_AES;
    const CK_BBOOL bTrue = TRUE;
    CK_ATTRIBUTE keyTemplate[4];
    uint8_t key[16];
    uint32_t keylen = 16;
    CK_OBJECT_HANDLE hSBK;
    CK_ULONG nCount;
    CK_BYTE pKeyId[2] = {0x00, 0x01};
    CK_ATTRIBUTE findTemplate[1] =
    {
        { CKA_ID, NULL, sizeof(pKeyId) },
    };

    _SLogTrace("getWrappingKey Enter");

    *phKey = CK_INVALID_HANDLE;

    /* Find  the TF_SBK */
    findTemplate[0].pValue = pKeyId;
    nError = C_FindObjectsInit(hSession, findTemplate, 1);
    if (nError != CKR_OK) {
        _SLogTrace("getWrappingKey Error : cannot initialize key search [0x%x].", nError);
        return nError;
    }

    nError = C_FindObjects(hSession, &hSBK, 1, &nCount);
    if (nError != CKR_OK) {
        _SLogTrace ("getWrappingKey Error : cannot find the device key [0x%x].", nError);
        return nError;
    }

    nError = C_FindObjectsFinal(hSession);
    if (nError != CKR_OK) {
        _SLogTrace("getWrappingKey Error : cannot terminate search [0x%x].", nError);
        return nError;
    }

    if (nCount == 0) {
        nError = CKR_DEVICE_ERROR;
        _SLogTrace("getWrappingKey Error : Did not find any key matching the template[0x%x].", nError);
        *phKey = CK_INVALID_HANDLE;
        return nError;
    }

    /* Retrieve TF_SBK KEY value */
    keyTemplate[0].type       = CKA_VALUE_LEN;
    keyTemplate[0].pValue     = &keylen;
    keyTemplate[0].ulValueLen = sizeof(keylen);

    keyTemplate[1].type       = CKA_VALUE;
    keyTemplate[1].pValue     = (CK_VOID_PTR)key;
    keyTemplate[1].ulValueLen = 16;

    nError = C_GetAttributeValue(hSession, hSBK, keyTemplate, 2);
    if (nError != CKR_OK) {
        _SLogTrace("getWrappingKey Error :  cannot retrieve TF_SBK value [0x%x].", nError);
        return nError;
    }

    /* Import TF-SBK key into the key store */
    keyTemplate[0].type = CKA_CLASS;
    keyTemplate[0].pValue = (CK_VOID_PTR)&keyClass;
    keyTemplate[0].ulValueLen = sizeof(keyClass);

    keyTemplate[1].type = CKA_KEY_TYPE;
    keyTemplate[1].pValue = (CK_VOID_PTR)&keyType;
    keyTemplate[1].ulValueLen = sizeof(keyType);

    keyTemplate[2].type = CKA_DECRYPT;
    keyTemplate[2].pValue = (CK_VOID_PTR)&bTrue;
    keyTemplate[2].ulValueLen = sizeof(bTrue);

    keyTemplate[3].type = CKA_VALUE;
    keyTemplate[3].pValue = key;
    keyTemplate[3].ulValueLen = keylen;

    nError = C_CreateObject(hSession, keyTemplate, 4, phKey);
    if (nError != CKR_OK) {
        _SLogTrace("getWrappingKey Error :  AES key creation failed [0x%x]", nError);
        return nError;
    }
    _SLogTrace("getWrappingKey Exit");

    return nError;
}

/*
 * Decrypt encrypted WV key
 */
static S_RESULT dataUnwrap(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hKey,
                           uint8_t *pData, uint32_t *pnDataLen)
{
    S_RESULT nError;
    CK_MECHANISM aes_ecb;
    /*Key mechanism for decrypt WV key : AES-ECB*/
    aes_ecb.mechanism = CKM_AES_ECB;
    aes_ecb.pParameter = NULL;
    aes_ecb.ulParameterLen = 0;

    _SLogTrace("dataUnwrap Enter");

    nError = C_DecryptInit(hSession, &aes_ecb, hKey);
    if (nError != CKR_OK) {
        _SLogTrace("dataUnwrap Error:  cannot initialize decryption [0x%x].", nError);
        return nError;
    }

    nError = C_Decrypt(hSession, (CK_BYTE *)pData, *pnDataLen,
                       (CK_BYTE *)pData, pnDataLen);
    if (nError != CKR_OK) {
        _SLogTrace("dataUnwrap Error :  cannot decrypt encrypted WV key [0x%x].", nError);
        return nError;
    }
    _SLogTrace("dataUnwrap Exit");
    return nError;
}

/*
 * Decrypt encrypted Entitlement key and create key template for entitlement key
 */
static S_RESULT dataUnwrap_entitlementkey(SESSION_CONTEXT* pSessionContext,
                                          CK_BYTE*  pData, uint32_t pnDataLen)
{
    S_RESULT nError;
    CK_MECHANISM aes_ecb;
    CK_BYTE*          pClearEMMKeyData = NULL;
    CK_ULONG        clearEMMKeyLen = 16;
    CK_RV ckr;

    /* AES's key class (secret key) */
    static const CK_OBJECT_CLASS   aesKeyClass  = CKO_SECRET_KEY;
    /* AES key type (AES) */
    static const CK_KEY_TYPE       aesKeyType   = CKK_AES;
    /* Dummy variable which we can use for &"TRUE" */
    static const CK_BBOOL          bAesTrue = TRUE;
    /* Object ID for this key for this session. */
    static const CK_BYTE           pObjId3[] = {0x00, 0x03};
    /* Create a key template for the AES key. The attribute CKA_VALUE will be
      filled with the key material later on */
    CK_ATTRIBUTE aesKeyTemplate[] = {
        {CKA_CLASS,    (void*)&aesKeyClass, sizeof(aesKeyClass)},
        {CKA_ID,       (void*)pObjId3,      sizeof(pObjId3)},
        {CKA_KEY_TYPE, (void*)&aesKeyType,  sizeof(aesKeyType)},
        {CKA_DECRYPT,  (void*)&bAesTrue,    sizeof(bAesTrue)},
        {CKA_VALUE,    (void*)NULL,         0}
    };

    /*Key mechanism for decrypt EMM key : AES-ECB*/
    aes_ecb.mechanism = CKM_AES_ECB;
    aes_ecb.pParameter = NULL;
    aes_ecb.ulParameterLen = 0;

    _SLogTrace("dataUnwrap_entitlementkey Enter");

    pClearEMMKeyData = (CK_BYTE_PTR)SMemAlloc(clearEMMKeyLen);
    if (pClearEMMKeyData == NULL) {
        _SLogTrace("dataUnwrap_entitlementkey Error :  Malloc failed\n");
        nError = S_ERROR_OUT_OF_MEMORY;
    }

    nError = C_DecryptInit(pSessionContext->hCryptoSession, &aes_ecb, pSessionContext->hWidevineKey);
    if (nError != CKR_OK) {
        _SLogTrace("dataUnwrap_entitlementkey Error :  cannot initialize decryption [0x%x].", nError);
        goto error_closeSession;
    }

    nError = C_Decrypt(pSessionContext->hCryptoSession, pData, pnDataLen,
                       pClearEMMKeyData, &clearEMMKeyLen);

    if (nError != CKR_OK) {
        _SLogTrace("dataUnwrap_entitlementkey Error :  cannot decrypt "
                   "WMDRMPD certificate template [0x%x].", nError);
        goto error_closeSession;
    }

    if(pSessionContext->hEMMKey!=CK_INVALID_HANDLE){
        nError = C_DestroyObject(pSessionContext->hCryptoSession, pSessionContext->hEMMKey);
        if (nError != CKR_OK) {
            _SLogError("dataUnwrap_entitlementkey Error : C_DestroyObject() failed (%08x).", nError);
            goto error_closeSession;
        }
        pSessionContext->hEMMKey = CK_INVALID_HANDLE;
    }
    /* Import the session key into the key store */
    aesKeyTemplate[4].pValue = pClearEMMKeyData;
    aesKeyTemplate[4].ulValueLen = clearEMMKeyLen;
    if ((ckr = C_CreateObject(pSessionContext->hCryptoSession,
                              aesKeyTemplate,
                              sizeof(aesKeyTemplate)/sizeof(CK_ATTRIBUTE),
                              &pSessionContext->hEMMKey)) != CKR_OK) {
        _SLogTrace( "dataUnwrap_entitlementkey Error :  license key import failed [0x%x].\n", ckr);
        nError = OEMCRYPTO_AGENT_ERROR_CRYPTO;
        goto error_closeSession;
    }

    SMemFree(pClearEMMKeyData);

    _SLogTrace("dataUnwrap_entitlementkey Exit");
    return nError;

    error_closeSession:
    SMemFree(pClearEMMKeyData);
    if (pSessionContext->hEMMKey != CK_INVALID_HANDLE) C_DestroyObject(pSessionContext->hCryptoSession, pSessionContext->hEMMKey);

    pSessionContext->hEMMKey = CK_INVALID_HANDLE;
    return nError;
}

/*
 * Get Control words and flag data
 * 1. Decrypt ECM data with clear EMM key to get control word and flag data
 * 2. Open session for OTF Driver to set control word
 * 3. Set control word to OTF driver and return flags to normal world
 */
static S_RESULT dataUnwrap_ecm(SESSION_CONTEXT* pSessionContext,
                               uint8_t *pData, uint32_t pnDataLen, uint32_t *flags)
{
    S_RESULT nError;
    const S_UUID*    pServiceUUID;
    uint32_t nParamTypes;
    S_PARAM pParams[4];
    CK_MECHANISM aes_cbc;
    CK_BYTE*          pClearECMData = NULL;
    CK_ULONG        clearECMDataLen = 32;
    CK_ULONG        clearControlWordLen = 16;
    CK_ULONG        clearControlWordOffset = 4;
    CK_RV ckr;

    /* AES's key class (secret key) */
    static const CK_OBJECT_CLASS   aesKeyClass  = CKO_SECRET_KEY;
    /* AES key type (AES) */
    static const CK_KEY_TYPE       aesKeyType   = CKK_AES;
    /* Dummy variable which we can use for &"TRUE" */
    static const CK_BBOOL          bAesTrue = TRUE;
    /* Object ID for this key for this session. */
    static const CK_BYTE           pObjId4[] = {0x00, 0x04};
    /* Create a key template for the AES key. The attribute CKA_VALUE will be
      filled with the key material later on */
    CK_ATTRIBUTE aesKeyTemplate[] = {
        {CKA_CLASS,    (void*)&aesKeyClass, sizeof(aesKeyClass)},
        {CKA_ID,       (void*)pObjId4,      sizeof(pObjId4)},
        {CKA_KEY_TYPE, (void*)&aesKeyType,  sizeof(aesKeyType)},
        {CKA_DECRYPT,  (void*)&bAesTrue,    sizeof(bAesTrue)},
        {CKA_VALUE,    (void*)NULL,         0}
    };

    /*Initial vector for AES-CBC*/
    static const CK_BYTE iv[16] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    _SLogTrace("dataUnwrap_ecm Enter3");

    /*Key mechanism for decrypt ECM data : AES-CBC*/
    aes_cbc.mechanism = CKM_AES_CBC;
    aes_cbc.pParameter = (CK_VOID_PTR)iv;
    aes_cbc.ulParameterLen = sizeof(iv);

    pClearECMData = (CK_BYTE_PTR)SMemAlloc(clearECMDataLen);
    if (pClearECMData == NULL) {
        SLogError("dataUnwrap_ecm Error : allocate result buffer\n");
        nError = S_ERROR_OUT_OF_MEMORY;
    }

    nError = C_DecryptInit(pSessionContext->hCryptoSession, &aes_cbc, pSessionContext->hEMMKey);
    if (nError != CKR_OK) {
        _SLogTrace("dataUnwrap_ecm Error : cannot initialize decryption [0x%x].", nError);
        goto error_closeSession;
    }

    nError = C_Decrypt(pSessionContext->hCryptoSession, (CK_BYTE *)pData, pnDataLen,
                       pClearECMData, &clearECMDataLen);
    if (nError != CKR_OK) {
        _SLogTrace("dataUnwrap_ecm Error : cannot decrypt ECM data [0x%x].", nError);
        goto error_closeSession;
    }

    /*Set flag data to return to normal world*/
    *flags = *(uint32_t*)pClearECMData;

    if(pSessionContext->hControlWord!=CK_INVALID_HANDLE){
        nError = C_DestroyObject(pSessionContext->hCryptoSession, pSessionContext->hControlWord);
        if (nError != CKR_OK) {
            _SLogError("dataUnwrap_ecm Error : C_DestroyObject() failed (%08x).", nError);
            goto error_closeSession;
        }
        pSessionContext->hControlWord = CK_INVALID_HANDLE;
    }

    /* Import the session key into the key store */
    aesKeyTemplate[4].pValue = pClearECMData+clearControlWordOffset;
    aesKeyTemplate[4].ulValueLen = clearControlWordLen;
    if ((ckr = C_CreateObject(pSessionContext->hCryptoSession,
                              aesKeyTemplate,
                              sizeof(aesKeyTemplate)/sizeof(CK_ATTRIBUTE),
                              &pSessionContext->hControlWord)) != CKR_OK) {
        _SLogTrace( "dataUnwrap_ecm Error : control word import failed [0x%x].\n", ckr);
        nError = OEMCRYPTO_AGENT_ERROR_CRYPTO;
        goto error_closeSession;
    }

    /*Service ID for OTF driver*/
    pServiceUUID = &OTF_UUID;
    nParamTypes = S_PARAM_TYPES(S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE);

    if (pSessionContext->hOTFSession!= S_HANDLE_NULL){
        SHandleClose(pSessionContext->hOTFSession);
        pSessionContext->hOTFSession = S_HANDLE_NULL;
    }

    /* Open OTF driver session */
    ckr = SXControlOpenClientSession(pServiceUUID,
                                     NULL,
                                     nParamTypes,
                                     pParams,
                                     &pSessionContext->hOTFSession,
                                     NULL);
    if (ckr != CKR_OK) {
        _SLogTrace("dataUnwrap_ecm Error : open otf session failed [0x%x].\n", ckr);
        nError = OTFDRIVER_AGENT_ERROR_CRYPTO;
        goto error_closeSession;
    }

    /* Login */
    ckr = C_Login(pSessionContext->hOTFSession, CKU_USER, NULL, 0);
    if (ckr != CKR_OK) {
        _SLogTrace("dataUnwrap_ecm Error : otf session login failed [0x%x].\n", ckr);
        C_CloseSession(pSessionContext->hOTFSession);
        return OTFDRIVER_AGENT_ERROR_CRYPTO;
    }

    nParamTypes = S_PARAM_TYPES(S_PARAM_TYPE_MEMREF_INPUT, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE);

    pParams[0].memref.pBuffer = aesKeyTemplate[4].pValue;
    pParams[0].memref.nSize = 16;
    ckr = SXControlInvokeCommand(pSessionContext->hOTFSession,
                                 NULL,
                                 OTF_SET_AES_KEY,
                                 nParamTypes,
                                 pParams,
                                 NULL);
    if (ckr != CKR_OK) {
        _SLogTrace("dataUnwrap_ecm Error : otf invoke command failed [0x%x].\n", ckr);
        nError = OTFDRIVER_AGENT_ERROR_CRYPTO;
        goto error_closeSession;
    }

    SMemFree(pClearECMData);

    _SLogTrace("dataUnwrap_ecm Exit");
    return nError;

    error_closeSession:
    SMemFree(pClearECMData);
    if (pSessionContext->hControlWord != CK_INVALID_HANDLE) C_DestroyObject(pSessionContext->hCryptoSession, pSessionContext->hControlWord);
    if (pSessionContext->hOTFSession != S_HANDLE_NULL) SHandleClose(pSessionContext->hOTFSession);

    pSessionContext->hControlWord = CK_INVALID_HANDLE;
    pSessionContext->hOTFSession = CK_INVALID_HANDLE;
    return nError;
}

/*
 * Create Service
 */
S_RESULT SRVX_EXPORT SRVXCreate(void)
{
    S_RESULT nError = S_SUCCESS;
    uint8_t *pWidevineKeyBox = NULL;
    uint32_t nWidevineKeyBoxSize = 0;
#ifdef ENABLE_INSTALL_KEYBOX
    uint32_t nWidevineKeyBoxLen = 0;
#endif
    uint32_t nWidevineKeyOffset = 32;
    CK_SESSION_HANDLE hSession = S_HANDLE_NULL;
    CK_OBJECT_HANDLE hSBKKey = NULL;
    CK_ULONG nWidevineKeyLen = 16;
    SESSION_CONTEXT *pSessionContext = NULL;
    CK_RV ckr;
    CK_BYTE WidevineKey[16];

    /* AES's key class (secret key) */
    static const CK_OBJECT_CLASS   aesKeyClass  = CKO_SECRET_KEY;
    /* AES key type (AES) */
    static const CK_KEY_TYPE       aesKeyType   = CKK_AES;
    /* Dummy variable which we can use for &"TRUE" */
    static const CK_BBOOL          bAesTrue = TRUE;
    /* Object ID for this key for this session. */
    static const CK_BYTE           pObjId2[] = {0x00, 0x02};
    /* Create a key template for the AES key. The attribute CKA_VALUE will be
      filled with the key material later on */
    CK_ATTRIBUTE aesKeyTemplate[] = {
        {CKA_CLASS,    (void*)&aesKeyClass, sizeof(aesKeyClass)},
        {CKA_ID,       (void*)pObjId2,      sizeof(pObjId2)},
        {CKA_KEY_TYPE, (void*)&aesKeyType,  sizeof(aesKeyType)},
        {CKA_DECRYPT,  (void*)&bAesTrue,    sizeof(bAesTrue)},
        {CKA_VALUE,    (void*)NULL,         0}
    };

#ifdef ENABLE_INSTALL_KEYBOX
    S_HANDLE hFileHandle;
#endif

    _SLogTrace("SRVXCreate Enter");

    nError = C_Initialize(NULL);
    if (nError != CKR_OK) {
        _SLogTrace("SRVXCreate Error : library initialization failed [0x%x].", nError);
        goto error_closeSession;
    }

    pSessionContext = SMemAlloc(sizeof(SESSION_CONTEXT));
    if (pSessionContext == NULL) {
        _SLogTrace("SRVXCreate Error : Malloc for session context failed.");
        return S_ERROR_OUT_OF_MEMORY;
    }
    SMemFill(pSessionContext, 0, sizeof(SESSION_CONTEXT));

    pSessionContext->hWidevineKey = CK_INVALID_HANDLE;
    pSessionContext->hEMMKey = CK_INVALID_HANDLE;
    pSessionContext->hControlWord = CK_INVALID_HANDLE;

    /*Open session to decrypt WV key */
    if ((nError = C_OpenSession(S_CRYPTOKI_KEYSTORE_HW_TOKEN, CKF_SERIAL_SESSION, NULL, NULL, &hSession)) != CKR_OK) {
        _SLogTrace("SRVXCreate Error : open primary session failed [0x%x].",nError);
        goto error_closeSession;
    }

#ifdef ENABLE_INSTALL_KEYBOX
    nError = SFileOpen(
    S_FILE_STORAGE_PRIVATE,
    FILENAME,
    S_FILE_FLAG_ACCESS_READ,
    0,
    &hFileHandle);
    if (nError == S_ERROR_ITEM_NOT_FOUND)
    {
       /* There's no key. We need to install the keybox*/
       _SLogTrace("SRVXCreate Error : SFileOpen() failed with S_ERROR_ITEM_NOT_FOUND");
       /* Nothing to do here*/
       return S_SUCCESS;
    }

    if ((nError != S_SUCCESS)&&(nError != S_ERROR_ITEM_NOT_FOUND))
    {
        _SLogTrace("SRVXCreate Error : SFileOpen() failed (%08x).", nError);
        goto error_closeSession;
    }

    /* Get keybox Length  */
    nError = SFileGetSize(S_FILE_STORAGE_PRIVATE, FILENAME, &nWidevineKeyBoxLen);
    if(nError!= S_SUCCESS)
    {
        _SLogTrace("SRVXCreate Error : SFileGetSize() failed (%08x)." ,nError);
        goto error_closeSession;
    }

    pWidevineKeyBox = SMemAlloc(nWidevineKeyBoxLen);
    if(pWidevineKeyBox == NULL){
        _SLogTrace("SRVXCreate Error : pWidevineKey SMemAlloc failed.");
        goto error_closeSession;
    }

    /* File successfully opened. Read it*/
    nError = SFileRead(hFileHandle, pWidevineKeyBox, nWidevineKeyBoxLen, &nWidevineKeyBoxSize);
    if (nError != S_SUCCESS)
    {
        _SLogTrace("SRVXCreate Error : SFileRead() failed (%08x).", nError);
        goto error_closeSession;
    }
    if (nWidevineKeyBoxSize != nWidevineKeyBoxLen)
    {
        _SLogTrace("SRVXCreate Error : SFileRead(): File size is wrong.");
        goto error_closeSession;
    }
    SHandleClose(hFileHandle);
#else
    /* Retrieve encrypted widevine keybox from manifest */
    nError = SServiceGetProperty("config.data.widevine_key", (char **)&pWidevineKeyBox);
    if (nError != S_SUCCESS) {
        _SLogTrace("SRVXCreate Error : cannot retrieve widevine encrypted keybox [0x%x]", nError);
        goto error_closeSession;
    }

    nWidevineKeyBoxSize = strlen((const char*)pWidevineKeyBox)>>1;
    char_to_bin((uint8_t*)pWidevineKeyBox, nWidevineKeyBoxSize);
#endif

    /* Retrieve the encryption key from TF_SBK */
    nError = getWrappingKey(hSession, &hSBKKey);
    if (nError != CKR_OK) {
        _SLogTrace("SRVXCreate Error : Could not get wrapped Key[0x%x].",nError);
        goto error_closeSession;
    }

    /* Decrypt widevine key */
    nError = dataUnwrap(hSession, hSBKKey, pWidevineKeyBox, &nWidevineKeyBoxSize);
    if (nError != CKR_OK) {
        _SLogTrace("SRVXCreate Error : cannot unwrap widevine encrypted keybox [0x%x]", nError);
        goto error_closeSession;
    }

    SMemMove(pSessionContext->hdeviceID, pWidevineKeyBox, 32);
    SMemMove(pSessionContext->hdeviceKey, (const void *)(pWidevineKeyBox + 32), 16);
    SMemMove(pSessionContext->hkeyData, (const void *)(pWidevineKeyBox + 48), 72);
    SMemMove(pSessionContext->hmagic, (const void *)(pWidevineKeyBox + 120), 4);
    SMemMove(pSessionContext->hcrc, (const void *)(pWidevineKeyBox + 124), 4);
    SMemMove(WidevineKey, pWidevineKeyBox+nWidevineKeyOffset, nWidevineKeyLen);

    SMemFree(pWidevineKeyBox);

    /*Destroy TF-SBK key*/
    nError = C_DestroyObject(hSession, hSBKKey);
    if (nError != CKR_OK) {
        _SLogError("SRVXCreate Error : C_DestroyObject() failed (%08x).", nError);
        goto error_closeSession;
    }

    /* Close the current session to create clear widevine key in another token*/
    (void)C_CloseSession(hSession);
    hSession = S_HANDLE_NULL;

    /*Open session to import WV key into another token. We should import the key to another session for security*/
    if ((nError = C_OpenSession(S_CRYPTOKI_KEYSTORE_PRIVATE, CKF_SERIAL_SESSION  | CKF_RW_SESSION, NULL, NULL, &hSession)) != CKR_OK) {
        _SLogTrace("SRVXCreate Error :  open secondary session failed [0x%x].",nError);
        goto error_closeSession;
    }

    pSessionContext->hCryptoSession = hSession;

    /* Import the WV key into the key store */
    aesKeyTemplate[4].pValue = WidevineKey;
    aesKeyTemplate[4].ulValueLen = nWidevineKeyLen;
    if ((ckr = C_CreateObject(pSessionContext->hCryptoSession,
                              aesKeyTemplate,
                              sizeof(aesKeyTemplate)/sizeof(CK_ATTRIBUTE),
                              &pSessionContext->hWidevineKey)) != CKR_OK) {
        _SLogTrace( "SRVXCreate Error :  license key import failed [0x%x].\n", ckr);
        nError = OEMCRYPTO_AGENT_ERROR_CRYPTO;
        goto error_closeSession;
    }

    /* Save the session context */
    SInstanceSetData( pSessionContext );

    _SLogTrace("SRVXCreate Exit");

    /* Nothing to do */
    return S_SUCCESS;

    error_closeSession:
    if (pSessionContext->hWidevineKey != CK_INVALID_HANDLE) C_DestroyObject(pSessionContext->hCryptoSession, pSessionContext->hWidevineKey);
    if (pSessionContext->hCryptoSession != S_HANDLE_NULL) C_CloseSession(pSessionContext->hCryptoSession);
#ifdef ENABLE_INSTALL_KEYBOX
    if (hFileHandle != S_HANDLE_NULL) SHandleClose(hFileHandle);
#endif
    if(pWidevineKeyBox != NULL) SMemFree(pWidevineKeyBox);
    if(pSessionContext != NULL) SMemFree(pSessionContext);
    C_Finalize(NULL);
    return nError;
}

/*
 * Destroy Service
 */
void SRVX_EXPORT SRVXDestroy(void)
{
    SESSION_CONTEXT *pSessionContext = NULL;

    _SLogTrace("SRVXDestroy");

    pSessionContext = SInstanceGetData();

    if(pSessionContext != NULL){
        _SLogTrace("==============================");
        _SLogTrace("SRVXDestroy : pSessionContext (0x%x)", pSessionContext);
        _SLogTrace("==============================");

        if (pSessionContext->hCryptoSession != S_HANDLE_NULL){
            C_CloseSession(pSessionContext->hCryptoSession);
        }

        if (pSessionContext->hOTFSession!= S_HANDLE_NULL){
            SHandleClose(pSessionContext->hOTFSession);
        }
        SMemFree(pSessionContext);
    }
    (void)C_Finalize(NULL);
    return;
}

/*
 * Get clear emm key(asset key) with clear WV key
 */
static S_RESULT SetEntitlementKey(SESSION_CONTEXT* pSessionContext,
                                  uint32_t nParamTypes,
                                  IN OUT S_PARAM pParams[4])

{
    S_RESULT nError = S_SUCCESS;
    uint8_t *pEMMKey = NULL;
    uint32_t nEMMKeyCMLen;

    _SLogTrace("SetEntitlementKey Enter");

    pEMMKey = pParams[0].memref.pBuffer;
    nEMMKeyCMLen = pParams[0].memref.nSize;

    if (pEMMKey == NULL) {
        _SLogTrace("SetEntitlementKey Error : EMM data is NULL.");
        return S_ERROR_BAD_PARAMETERS;
    }

    /* Decrypt certificate */
    nError = dataUnwrap_entitlementkey(pSessionContext, pEMMKey, nEMMKeyCMLen);
    if (nError != CKR_OK) {
        _SLogTrace("SetEntitlementKey Error : dataUnwrap_entitlementkey failed[0x%x]", nError);
        return nError;
    }
    _SLogTrace("SetEntitlementKey Exit");

    return S_SUCCESS;
}

/*
 * Get ECM data(Control words and flags) with clear EMM key
 */
static S_RESULT DeriveControlWord(SESSION_CONTEXT* pSessionContext,
                                  uint32_t nParamTypes,
                                  IN OUT S_PARAM pParams[4])
{
    S_RESULT nError = S_SUCCESS;
    uint8_t *pECM = NULL;
    uint32_t nECMLen;
    uint32_t pFlags = 0;

    _SLogTrace("SRVXDeriveControlWord Enter");

    pECM = pParams[0].memref.pBuffer;
    nECMLen = pParams[0].memref.nSize;
    if (pECM == NULL) {
        _SLogTrace("DeriveControlWord Error : ECM data is NULL.");
        return S_ERROR_BAD_PARAMETERS;
    }

    /* Decrypt encrypted control words */
    nError = dataUnwrap_ecm(pSessionContext, pECM, nECMLen, &pFlags);
    if (nError != CKR_OK) {
        return nError;
    }
    /* Byte swap because of ntohl */
    pParams[1].value.a = bswap_32(pFlags);
    if (pFlags == NULL) {
        _SLogTrace("DeriveControlWord Error : Flag is NULL.");
        return S_ERROR_BAD_PARAMETERS;
    }

    _SLogTrace("DeriveControlWord Exit");
    return S_SUCCESS;
}

static S_RESULT DecryptAudio(SESSION_CONTEXT* pSessionContext,
                             uint32_t nParamTypes,
                             IN OUT S_PARAM pParams[4])
{
    S_RESULT nError = S_SUCCESS;
    uint32_t retVal;
    //uint32_t length;

    retVal = DecryptAES_CBC_CTS(pSessionContext->hCryptoSession,
                                pSessionContext->hControlWord,
                                pParams[0].memref.pBuffer,
                                pParams[1].memref.pBuffer,
                                pParams[1].memref.nSize,
                                pParams[2].memref.pBuffer,
                                &pParams[3].value.a);
/*
    for (length = 0; length < pParams[3].value.a; length++) {
        _SLogTrace("DecryptAudio:Decrypted data:%x", ((uint8_t *)pParams[2].memref.pBuffer)[length]);
    }
*/
    if (retVal != CKR_OK) {
        return OEMCrypto_ERROR_DECRYPT_FAILED;
    }
    return nError;
}

static S_RESULT DecryptVideo(SESSION_CONTEXT* pSessionContext,
                             uint32_t nParamTypes,
                             IN OUT S_PARAM pParams[4])
{
    S_RESULT nError = S_SUCCESS;
    CK_BYTE* input;
    CK_ULONG inputLength;
    CK_ULONG outputHandle;
    CK_ULONG outputOffset;
    CK_ULONG outputLength;
    CK_BYTE *pOutputBitStream = NULL;
    NvMMAesWvMetadata *pNvMMAesWvMetadata = NULL;
    CK_ULONG input_len_left;
    CK_ULONG outLength;
    CK_BYTE d_iv[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CK_BYTE *iv;

    outputOffset = pParams[0].value.b;
    input = pParams[1].memref.pBuffer;
    inputLength = pParams[1].memref.nSize;
    outputHandle = (CK_ULONG)pParams[2].memref.pBuffer;

    if(pParams[0].value.a == 0)
    {
        iv = NULL;
    }
    else
    {
        iv = d_iv;
    }

    outputLength = 0;

    if (0 == outputOffset )
    {
        pNvMMAesWvMetadata = (NvMMAesWvMetadata *)outputHandle;
        if (NULL == iv)
        {
            /* Indicates clear stream.*/
            pNvMMAesWvMetadata->AlgorithmID = NvMMMAesAlgorithmID_AES_ALGO_ID_NOT_ENCRYPTED;
        }
        else
        {
            /* iv should be zero for start of encrypted stream */
            pNvMMAesWvMetadata->AlgorithmID = NvMMMAesAlgorithmID_AES_ALGO_ID_CBC_MODE;
            SMemMove(pNvMMAesWvMetadata->Iv, iv, 16);
            pSessionContext->FirstPacketOffset = inputLength;
            pParams[0].value.b = 0;
        }
        pOutputBitStream = (CK_BYTE *)(((CK_ULONG)pNvMMAesWvMetadata + sizeof(NvMMAesWvMetadata) + 3) & (~3));
        //pOutputBitStream = (NvU8 *)(pNvMMAesWvMetadata);
        outputLength = (((CK_ULONG)pNvMMAesWvMetadata + sizeof(NvMMAesWvMetadata) + 3) & (~3)) -
                (CK_ULONG)pNvMMAesWvMetadata;
    }
    else
    {
        /* Intermediate input packet  */
        if (iv != NULL)
        {
            pParams[0].value.b = pSessionContext->FirstPacketOffset;
        }
        pOutputBitStream = (CK_BYTE *)outputHandle;
    }

    outputLength += inputLength;
    input_len_left = inputLength%16;

    pParams[0].value.a = outputLength;

    if (iv == NULL || (outputOffset == 0 && input_len_left == 0) ) {
        SMemMove(pOutputBitStream, input, inputLength);
        return S_SUCCESS;
    }

    if ( outputOffset != 0 ) {
        outLength = inputLength;
        nError = DecryptAES_CBC_CTS(pSessionContext->hCryptoSession,
                                    pSessionContext->hControlWord,
                                    iv, // iv
                                    input, // input
                                    inputLength, // size
                                    pOutputBitStream, // output
                                    &outLength); // output length
    }
    else if (input_len_left != 0){
        if(inputLength<32){
            outLength = inputLength;
            nError = DecryptAES_CBC_CTS(pSessionContext->hCryptoSession,
                                                                pSessionContext->hControlWord,
                                                                iv,
                                                                input, // input
                                                                inputLength, // size
                                                                pOutputBitStream, // output
                                                                &outLength); // output length
        }
        else{
            SMemMove(pOutputBitStream, input, inputLength);
            outLength = 16 + input_len_left;
            nError = DecryptAES_CBC_CTS(pSessionContext->hCryptoSession,
                                                                pSessionContext->hControlWord,
                                                                pOutputBitStream + inputLength - ( 32 + input_len_left ),
                                                                pOutputBitStream + inputLength - ( 16 + input_len_left ), // input
                                                                16 + input_len_left, // size
                                                                pOutputBitStream + inputLength - ( 16 + input_len_left ), // output
                                                                &outLength); // output length
        }
    }

    if (nError != S_SUCCESS) {
        _SLogTrace("DECRYPT_FAILED!!");
        return OEMCrypto_ERROR_DECRYPT_FAILED;
    }
    return nError;
}

/*
 * Get device ID
 */
static S_RESULT GetDeviceID(SESSION_CONTEXT* pSessionContext,
                            uint32_t nParamTypes,
                            IN OUT S_PARAM pParams[4])
{
    uint32_t length;

    _SLogTrace("GetDeviceID:Enter");

    length = pParams[1].value.a > 32? 32: pParams[1].value.a;
    SMemMove(pParams[0].memref.pBuffer, pSessionContext->hdeviceID, length);

    _SLogTrace("GetDeviceID:[%s]", pParams[0].memref.pBuffer);

    pParams[1].value.a = length;
    _SLogTrace("GetDeviceID:Exit");

    return S_SUCCESS;
}

/*
 * Get key data
 */
static S_RESULT GetKeyData(SESSION_CONTEXT* pSessionContext,
                           uint32_t nParamTypes,
                           IN OUT S_PARAM pParams[4])
{
    uint32_t length;

    length = pParams[1].value.a;

    _SLogTrace("GetKeyData:Enter");

    if (length != 72) {
        return S_ERROR_BAD_PARAMETERS;
    }
    SMemMove(pParams[0].memref.pBuffer, pSessionContext->hkeyData, length);

    pParams[1].value.a = length;

    _SLogTrace("GetKeyData:Exit");

    return S_SUCCESS;
}

/*
 * Is key box valid
 */
static S_RESULT IsKeyboxValid(SESSION_CONTEXT* pSessionContext,
                              uint32_t nParamTypes,
                              IN OUT S_PARAM pParams[4])
{
    uint8_t magic[] = {0x6b, 0x62, 0x6f, 0x78};
    uint32_t i;
    uint32_t crc;
    CK_BYTE_PTR keybox;
    uint32_t hcrc;
    _SLogTrace("IsKeyBoxValid:Enter");

    keybox = (CK_BYTE_PTR)SMemAlloc(124);

    if(!keybox) {
        _SLogTrace("IsKeyBoxValid: Out of Memory");
        return S_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0; i < 4; i++) {
        if (pSessionContext->hmagic[i] != magic[i])
        {
            SMemFree(keybox);
            return OEMCrypto_ERROR_BAD_MAGIC;
        }
    }

    SMemMove(keybox, pSessionContext->hdeviceID, 32);
    SMemMove(keybox + 32, pSessionContext->hdeviceKey, 16);
    SMemMove(keybox + 32 + 16, pSessionContext->hkeyData, 72);
    SMemMove(keybox + 32 + 16 + 72, pSessionContext->hmagic, 4);

    SMemMove(&hcrc, pSessionContext->hcrc, 4);

    crc = get_crc32((uint8_t *)keybox, 124);

    for(i = 0; i < 4; i++) {
        if ( *((uint8_t *)(&hcrc) + i) != *((uint8_t *)(&crc) + 3 - i) ) {
            /*TODO: return BAD crc error*/
            _SLogTrace("IsKeyBoxValid:Error: Bad CRC!");
            _SLogTrace("IsKeyBoxValid:Crc[%d]:0x%x", 3-i, *((uint8_t *)(&crc) + 3 - i) );
            _SLogTrace("IsKeyBoxValid:hcrc[%d]:0x%x", i, *((uint8_t *)(&hcrc) + i));
        }
    }

    SMemFree(keybox);

    _SLogTrace("IsKeyBoxValid:Exit");

    return OEMCrypto_SUCCESS;
}

static S_RESULT GetRandom(SESSION_CONTEXT* pSessionContext,
                          uint32_t nParamTypes,
                          IN OUT S_PARAM pParams[4])
{
    uint32_t retVal;

    _SLogTrace("GetRandom:Enter");

    retVal = C_GenerateRandom(pSessionContext->hCryptoSession, pParams[0].memref.pBuffer, pParams[1].value.a);

    if (retVal != CKR_OK) {
        _SLogTrace("GetRandom:Error in Generating Random num");
        return OEMCrypto_ERROR_RNG_FAILED;
    }

    _SLogTrace("GetRandom:Exit");

    return OEMCrypto_SUCCESS;
}

/*
 * Install WV keybox
 */
static S_RESULT InstallKeybox(SESSION_CONTEXT* pSessionContext,
                              uint32_t nParamTypes,
                              IN OUT S_PARAM pParams[4])
{
#ifdef ENABLE_INSTALL_KEYBOX
    S_RESULT nError = S_SUCCESS;
    S_HANDLE hFileHandle;
    uint8_t *pKeyBox = NULL;
    uint32_t nKeyBoxLen;

    _SLogTrace("InstallKeybox with ENABLE_INSTALL_KEYBOX : Enter");

    /* Read the certificate */
    pKeyBox       = pParams[0].memref.pBuffer;
    nKeyBoxLen = pParams[0].memref.nSize;
    if(pKeyBox == NULL)
    {
        _SLogTrace("InstallKeybox Error : S_ERROR_BAD_PARAMETERS");
        return S_ERROR_BAD_PARAMETERS;
    }

    nError = SFileOpen(
    S_FILE_STORAGE_PRIVATE,
    FILENAME,
    S_FILE_FLAG_ACCESS_READ | S_FILE_FLAG_ACCESS_WRITE | S_FILE_FLAG_CREATE,
    0,
    &hFileHandle);
    if (nError != S_SUCCESS)
    {
        _SLogTrace("InstallKeybox Error : SFileOpen() failed (%08x).", nError);
        goto error_closeHandle;
    }

    /* write key to file */
    nError = SFileWrite(hFileHandle, pKeyBox, nKeyBoxLen);
    if(nError != S_SUCCESS)
    {
        _SLogTrace("InstallKeybox Error : Cannot write to file [error 0x%X]", nError);
        goto error_closeHandle;
    }
    SHandleClose(hFileHandle);

    _SLogTrace("InstallKeybox with ENABLE_INSTALL_KEYBOX : Exit");

    return OEMCrypto_SUCCESS;

error_closeHandle:
    SHandleClose(hFileHandle);
    return nError;
#else
    _SLogTrace("InstallKeybox : Enter");
    _SLogTrace("InstallKeybox : Exit");
    /*Do nothing*/
    return OEMCrypto_SUCCESS;
#endif
}

static S_RESULT InstallControlWord(SESSION_CONTEXT* pSessionContext,
                                   uint32_t nParamTypes,
                                   IN S_PARAM pParams[4])
{
    S_RESULT nError = OEMCrypto_SUCCESS;
    const S_UUID*    pServiceUUID;
    uint32_t nOtfParamTypes;
    S_PARAM pOtfParams[4];
    CK_RV ckr;
    uint8_t *pControlWord = NULL;
    uint32_t nControlWordLen;

    /* AES's key class (secret key) */
    static const CK_OBJECT_CLASS   aesKeyClass  = CKO_SECRET_KEY;
    /* AES key type (AES) */
    static const CK_KEY_TYPE       aesKeyType   = CKK_AES;
    /* Dummy variable which we can use for &"TRUE" */
    static const CK_BBOOL          bAesTrue = TRUE;
    /* Object ID for this key for this session. */
    static const CK_BYTE           pObjId4[] = {0x00, 0x04};
    /* Create a key template for the AES key. The attribute CKA_VALUE will be
      filled with the key material later on */
    CK_ATTRIBUTE aesKeyTemplate[] = {
        {CKA_CLASS,    (void*)&aesKeyClass, sizeof(aesKeyClass)},
        {CKA_ID,       (void*)pObjId4,      sizeof(pObjId4)},
        {CKA_KEY_TYPE, (void*)&aesKeyType,  sizeof(aesKeyType)},
        {CKA_DECRYPT,  (void*)&bAesTrue,    sizeof(bAesTrue)},
        {CKA_VALUE,    (void*)NULL,         0}
    };

    _SLogTrace("SRVXInstallControlWord Enter");

    pControlWord = pParams[0].memref.pBuffer;
    nControlWordLen = pParams[0].memref.nSize;
    if (pControlWord == NULL) {
        _SLogTrace("InstallControlWord Error: Received NULL Control Word.");
        return S_ERROR_BAD_PARAMETERS;
    }

    if(pSessionContext->hControlWord!=CK_INVALID_HANDLE){
        nError = C_DestroyObject(pSessionContext->hCryptoSession, pSessionContext->hControlWord);
        if (nError != CKR_OK) {
            _SLogError("InstallControlWord Error : C_DestroyObject() failed (%08x).", nError);
            goto error_closeSession;
        }
        pSessionContext->hControlWord = CK_INVALID_HANDLE;
    }

    aesKeyTemplate[4].pValue = (CK_VOID_PTR)pControlWord;
    aesKeyTemplate[4].ulValueLen = nControlWordLen;

    if ((ckr = C_CreateObject(pSessionContext->hCryptoSession,
                              aesKeyTemplate,
                              sizeof(aesKeyTemplate)/sizeof(CK_ATTRIBUTE),
                              &pSessionContext->hControlWord)) != CKR_OK) {
        _SLogTrace( "InstallControlWord Error : control word import failed [0x%x].\n", ckr);
        nError = OEMCRYPTO_AGENT_ERROR_CRYPTO;
        goto error_closeSession;
    }

    /*Service ID for OTF driver*/
    pServiceUUID = &OTF_UUID;
    nOtfParamTypes = S_PARAM_TYPES(S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE);

    if (pSessionContext->hOTFSession!= S_HANDLE_NULL){
        SHandleClose(pSessionContext->hOTFSession);
        pSessionContext->hOTFSession = S_HANDLE_NULL;
    }

    /* Open OTF driver session */
    ckr = SXControlOpenClientSession(pServiceUUID,
                                     NULL,
                                     nOtfParamTypes,
                                     pOtfParams,
                                     &pSessionContext->hOTFSession,
                                     NULL);
    if (ckr != CKR_OK) {
        _SLogTrace("InstallControlWord Error : open otf session failed [0x%x].\n", ckr);
        nError = OTFDRIVER_AGENT_ERROR_CRYPTO;
        goto error_closeSession;
    }

    /* Login */
    ckr = C_Login(pSessionContext->hOTFSession, CKU_USER, NULL, 0);
    if (ckr != CKR_OK) {
        _SLogTrace("InstallControlWord Error : otf session login failed [0x%x].\n", ckr);
        C_CloseSession(pSessionContext->hOTFSession);
        return OTFDRIVER_AGENT_ERROR_CRYPTO;
    }

    nOtfParamTypes = S_PARAM_TYPES(S_PARAM_TYPE_MEMREF_INPUT, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE);

    pOtfParams[0].memref.pBuffer = aesKeyTemplate[4].pValue;
    pOtfParams[0].memref.nSize = 16;
    ckr = SXControlInvokeCommand(pSessionContext->hOTFSession,
                                 NULL,
                                 OTF_SET_AES_KEY,
                                 nOtfParamTypes,
                                 pOtfParams,
                                 NULL);
    if (ckr != CKR_OK) {
        _SLogTrace("InstallControlWord Error : otf invoke command failed [0x%x].\n", ckr);
        nError = OTFDRIVER_AGENT_ERROR_CRYPTO;
        goto error_closeSession;
    }

    _SLogTrace("InstallControlWord Exit");
    return OEMCrypto_SUCCESS;

error_closeSession:

    if (pSessionContext->hControlWord != CK_INVALID_HANDLE) C_DestroyObject(pSessionContext->hCryptoSession, pSessionContext->hControlWord);
    if (pSessionContext->hOTFSession != S_HANDLE_NULL) SHandleClose(pSessionContext->hOTFSession);

    pSessionContext->hControlWord = CK_INVALID_HANDLE;
    pSessionContext->hOTFSession = CK_INVALID_HANDLE;

    return nError;
}

S_RESULT SRVX_EXPORT SRVXOpenClientSession(
                                          uint32_t nParamTypes,
                                          IN OUT S_PARAM pParams[4],
                                          OUT void** ppSessionContext )
{
    _SLogTrace("SRVXOpenClientSession");

    *ppSessionContext = SInstanceGetData();

    return S_SUCCESS;
}

void SRVX_EXPORT SRVXCloseClientSession(IN OUT void* pSessionContext)
{
    SESSION_CONTEXT* session = (SESSION_CONTEXT*)pSessionContext;

    _SLogTrace("SRVXCloseClientSession");

    if(session != NULL){
        if (session->hOTFSession!= S_HANDLE_NULL){
            SHandleClose(session->hOTFSession);
            session->hOTFSession = S_HANDLE_NULL;
        }

        if(session->hControlWord!=CK_INVALID_HANDLE){
            C_DestroyObject(session->hCryptoSession, session->hControlWord);
            session->hControlWord = CK_INVALID_HANDLE;
        }

        if(session->hEMMKey!=CK_INVALID_HANDLE){
            C_DestroyObject(session->hCryptoSession, session->hEMMKey);
            session->hEMMKey = CK_INVALID_HANDLE;
        }
    }
}

S_RESULT SRVX_EXPORT SRVXInvokeCommand(IN OUT void* pSessionContext,
                                       uint32_t nCommandID,
                                       uint32_t nParamTypes,
                                       IN OUT S_PARAM pParams[4])
{
    switch (nCommandID) {
    case OEMCRYPTO_SET_ENTITLEMENT_KEY:
        if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_MEMREF_INPUT, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE)) {
            _SLogTrace("OEMCRYPTO_SET_ENTITLEMENT_KEY: bad parameter types (expected all none)");
            return S_ERROR_BAD_PARAMETERS;
        }
        return SetEntitlementKey((SESSION_CONTEXT*)pSessionContext, nParamTypes, pParams);
    case OEMCRYPTO_DERIVE_CONTROL_WORD:
        if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_MEMREF_INPUT, S_PARAM_TYPE_VALUE_OUTPUT, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE)) {
            _SLogTrace("OEMCRYPTO_DERIVE_CONTROL_WORD: bad parameter types (expected all none)");
            return S_ERROR_BAD_PARAMETERS;
        }
        return DeriveControlWord((SESSION_CONTEXT*)pSessionContext, nParamTypes, pParams);
    case OEMCRYPTO_DECRYPT_AUDIO:
        if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_MEMREF_INOUT, S_PARAM_TYPE_MEMREF_INPUT,  S_PARAM_TYPE_MEMREF_INOUT, S_PARAM_TYPE_VALUE_INOUT)) {
            _SLogTrace("OEMCRYPTO_DECRYPT_AUDIO: bad parameter types (expected all none)");
            return S_ERROR_BAD_PARAMETERS;
        }
        return DecryptAudio((SESSION_CONTEXT*)pSessionContext, nParamTypes, pParams);
    case OEMCRYPTO_DECRYPT_VIDEO:
        if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_VALUE_INOUT, S_PARAM_TYPE_MEMREF_INOUT,  S_PARAM_TYPE_MEMREF_INOUT, S_PARAM_TYPE_NONE)) {
            _SLogTrace("OEMCRYPTO_DECRYPT_VIDEO: bad parameter types (expected all none)");
            return S_ERROR_BAD_PARAMETERS;
        }
        return DecryptVideo((SESSION_CONTEXT*)pSessionContext, nParamTypes, pParams);
    case OEMCRYPTO_IS_KEYBOX_VALID:
        if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE)) {
            _SLogTrace("OEMCRYPTO_IS_KEYBOX_VALID: bad parameter types (expected all none)");
            return S_ERROR_BAD_PARAMETERS;
        }
        return IsKeyboxValid((SESSION_CONTEXT*)pSessionContext, nParamTypes, pParams);

    case OEMCRYPTO_GET_DEVICE_ID:
        if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_MEMREF_OUTPUT, S_PARAM_TYPE_VALUE_INOUT, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE)) {
            _SLogTrace("OEMCRYPTO_GET_DEVICE_ID: bad parameter types (expected all none)");
            return S_ERROR_BAD_PARAMETERS;
        }
        return GetDeviceID((SESSION_CONTEXT*)pSessionContext, nParamTypes, pParams);

    case OEMCRYPTO_GET_KEY_DATA:
        if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_MEMREF_OUTPUT, S_PARAM_TYPE_VALUE_INOUT, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE)) {
            _SLogTrace("OEMCRYPTO_GET_KEY_DATA: bad parameter types (expected all none)");
            return S_ERROR_BAD_PARAMETERS;
        }
        return GetKeyData((SESSION_CONTEXT*)pSessionContext, nParamTypes, pParams);
    case OEMCRYPTO_GET_RANDOM:
        if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_MEMREF_OUTPUT, S_PARAM_TYPE_VALUE_INPUT,  S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE)) {
            _SLogTrace("OEMCRYPTO_GET_RANDOM: bad parameter types (expected all none)");
            return S_ERROR_BAD_PARAMETERS;
        }
        return GetRandom((SESSION_CONTEXT*)pSessionContext, nParamTypes, pParams);
    case OEMCRYPTO_INSTALL_KEYBOX:
        if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_MEMREF_INPUT, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE)) {
            _SLogTrace("OEMCRYPTO_INSTALL_KEYBOX: bad parameter types (expected all none)");
            return S_ERROR_BAD_PARAMETERS;
        }
        return InstallKeybox((SESSION_CONTEXT*)pSessionContext, nParamTypes, pParams);
    case OEMCRYPTO_INSTALL_CONTROL_WORD:
        if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_MEMREF_INPUT, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE)) {
            _SLogTrace("OEMCRYPTO_INSTALL_CONTROL_WORD: bad parameter types");
            return S_ERROR_BAD_PARAMETERS;
        }
        return InstallControlWord((SESSION_CONTEXT*)pSessionContext, nParamTypes, pParams);
    default:
        _SLogTrace("invalid command ID: 0x%X", nCommandID);
        return S_ERROR_BAD_PARAMETERS;
    }
}
