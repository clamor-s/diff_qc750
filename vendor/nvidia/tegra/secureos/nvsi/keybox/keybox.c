#include <string.h>
#include "ssdi_hw_ext.h"
#include "sddi.h"
#include "nv_service.h"

/*--------------------------------------------------------
 * Internal APIs
 *-------------------------------------------------------*/

#define min(a,b) (a < b ? a : b)
#define max(a,b) (a < b ? b : a)

#ifdef ENABLE_INSTALL_KEYBOX

#define FILENAME "nvsi_keybox.dat"

#else

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
            nvsi_log("Error in converting from ASCII to binary");
            return;
        }
        c = *(ptr+1);
        if (c >= 'a' && c <= 'f') l = c - 'a' + 0xa;
        else if (c <= '9' && c >= '0') l = c - '0' + 0x0;
        else {
            nvsi_log("Error in converting from ASCII to binary");
            return;
        }
        ptr+=2;
        *s = (h<<4)|l;
        s++;
    }
}

#endif

/*
 * Retrieve TF_SBK key
 */
S_RESULT getWrappingKey(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE *phKey)
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

    //nvsi_log("getWrappingKey Enter");

    *phKey = CK_INVALID_HANDLE;

    /* Find  the TF_SBK */
    findTemplate[0].pValue = pKeyId;
    nError = C_FindObjectsInit(hSession, findTemplate, 1);
    if (nError != CKR_OK) {
        nvsi_log("getWrappingKey Error :  cannot initialize key search [0x%x].", nError);
        return nError;
    }

    nError = C_FindObjects(hSession, &hSBK, 1, &nCount);
    if (nError != CKR_OK) {
        nvsi_log ("getWrappingKey Error :  cannot find the device key [0x%x].", nError);
        return nError;
    }

    nError = C_FindObjectsFinal(hSession);
    if (nError != CKR_OK) {
        nvsi_log("getWrappingKey Error :  cannot terminate search [0x%x].", nError);
        return nError;
    }

    if (nCount == 0) {
        nError = CKR_DEVICE_ERROR;
        nvsi_log("getWrappingKey Error : Did not find any key matching the template[0x%x].", nError);
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
        nvsi_log("getWrappingKey Error :  cannot retrieve TF_SBK value [0x%x].", nError);
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
        nvsi_log("getWrappingKey Error :  AES key creation failed [0x%x]", nError);
        return nError;
    }
    //nvsi_log("getWrappingKey Exit");

    return nError;
}

/*
 * Decrypt encrypted WV key
 */
S_RESULT dataUnwrap(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hKey,
                           uint8_t *pData, uint32_t *pnDataLen)
{
    S_RESULT nError;
    //uint8_t pad;
    CK_MECHANISM aes_ecb;
    /*Key mechanism for decrypt WV key : AES-ECB*/
    aes_ecb.mechanism = CKM_AES_ECB;
    aes_ecb.pParameter = NULL;
    aes_ecb.ulParameterLen = 0;

    //nvsi_log("dataUnwrap Enter");

    nError = C_DecryptInit(hSession, &aes_ecb, hKey);
    if (nError != CKR_OK) {
        nvsi_log("dataUnwrap Error:  cannot initialize decryption [0x%x].", nError);
        return nError;
    }

    nError = C_Decrypt(hSession, (CK_BYTE *)pData, *pnDataLen,
                       (CK_BYTE *)pData, pnDataLen);
    if (nError != CKR_OK) {
        nvsi_log("dataUnwrap Error :  cannot decrypt encrypted WV key [0x%x].", nError);
        return nError;
    }

/*TODO : Need to figure it out whether padding is necessary or not*/
#if 0
    /* Remove padding (OpenSSL use PKCS#5 Padding) */
    pad = *(pData + *pnDataLen - 1);
    /* The value of the padding is actualy the number of bytes added by the padding operation */
    if ((pad <= 0x10) && (pad > 0x00)) {
        *pnDataLen -= (uint32_t)pad;
    } else {
        nvsi_log("dataUnwrap Error: PKCS#5 padding of data is not correct.");
        return S_ERROR_BAD_FORMAT;
    }
#endif

    //nvsi_log("dataUnwrap Exit");
    return nError;
}

S_RESULT oemcrypto_get_nvsi_keybox(/*OUT*/ uint8_t* buf, /*INOUT*/ uint32_t* length)
{

#ifdef ENABLE_INSTALL_KEYBOX
  S_HANDLE hFileHandle;
  uint32_t nNVSIKeyBoxLen;
#endif

  S_RESULT nError = S_SUCCESS;
  CK_SESSION_HANDLE hSession = S_HANDLE_NULL;
  CK_OBJECT_HANDLE hSBKKey = NULL;  
  uint8_t *pNVSIKey = NULL;
  uint32_t nNVSIKeyBoxSize = 0;
  
  nError = C_Initialize(NULL);
  if (nError != CKR_OK) {
      nvsi_log("oemcrypto_get_nvsi_keybox : library initialization failed [0x%x].", nError);
      return nError;
  }

  /*Open session to decrypt WV key */
  if ((nError = C_OpenSession(S_CRYPTOKI_KEYSTORE_HW_TOKEN, CKF_SERIAL_SESSION, NULL, NULL, &hSession)) != CKR_OK) {
      nvsi_log("oemcrypto_get_nvsi_keybox : open primary session failed [0x%x].",nError);
      return nError;
  }

  /* Retrieve the encryption key from TF_SBK */
  nError = getWrappingKey(hSession, &hSBKKey);
  if (nError != CKR_OK) {
      nvsi_log("oemcrypto_get_nvsi_keybox : Could not get wrapped Key[0x%x].",nError);
      return nError;
  }

#ifdef ENABLE_INSTALL_KEYBOX

  /* Retrieve encrypted nvsi keybox from the secure file system */
  nError = SFileOpen(
  S_FILE_STORAGE_PRIVATE,
  FILENAME,
  S_FILE_FLAG_ACCESS_READ,
  0,
  &hFileHandle);
  if (nError == S_ERROR_ITEM_NOT_FOUND)
  {
     /* There's no key. We need to install the keybox*/
     nvsi_log("oemcrypto_get_nvsi_keybox : SFileOpen() failed with S_ERROR_ITEM_NOT_FOUND");
     /* Nothing to do here*/
     return S_SUCCESS;
  }

  if ((nError != S_SUCCESS)&&(nError != S_ERROR_ITEM_NOT_FOUND))
  {
      nvsi_log("oemcrypto_get_nvsi_keybox : SFileOpen() failed (%08x).", nError);
      return nError;
  }

  /* Get keybox Length  */
  nError = SFileGetSize(S_FILE_STORAGE_PRIVATE, FILENAME, &nNVSIKeyBoxLen);
  if(nError!= S_SUCCESS)
  {
      nvsi_log("oemcrypto_get_nvsi_keybox : SFileGetSize() failed (%08x)." ,nError);
      return nError;
  }

  pNVSIKey = SMemAlloc(nNVSIKeyBoxLen);
  if(pNVSIKey == NULL)
  {
      nvsi_log("oemcrypto_get_nvsi_keybox : pWidevineKey SMemAlloc failed.");
      return nError;
  }
  /* File successfully opened. Read it*/
  nError = SFileRead(hFileHandle, pNVSIKey, nNVSIKeyBoxLen, &nNVSIKeyBoxSize);
  if (nError != S_SUCCESS)
  {
      nvsi_log("oemcrypto_get_nvsi_keybox : SFileRead() failed (%08x).", nError);
      return nError;
  }
  if (nNVSIKeyBoxSize != nNVSIKeyBoxLen)
  {
      nvsi_log("oemcrypto_get_nvsi_keybox : SFileRead(): File size is wrong.");
      return S_ERROR_GENERIC;
  }
  SHandleClose(hFileHandle);

#else

  /* Retrieve encrypted nvsi keybox from manifest */
  nError = SServiceGetProperty("config.data.nvsi_key", (char **)&pNVSIKey);
  if (nError != S_SUCCESS) {
      nvsi_log("oemcrypto_get_nvsi_keybox : cannot retrieve nvsi encrypted keybox [0x%x]", nError);
      return nError;
  }

  nNVSIKeyBoxSize = strlen((const char*)pNVSIKey)>>1;
  char_to_bin((uint8_t*)pNVSIKey, nNVSIKeyBoxSize);

#endif

  /* Decrypt nvsi key */
  nError = dataUnwrap(hSession, hSBKKey, pNVSIKey, &nNVSIKeyBoxSize);
  if (nError != CKR_OK) {
      nvsi_log("oemcrypto_get_nvsi_keybox : cannot unwrap nvsi encrypted keybox [0x%x]", nError);
      return nError;
  }
  
  // !!! DO NOT LOG SENSITIVE INFORMATION IN PRODUCTION BUILDS !!!

  /*nvsi_log("ID: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    pNVSIKey[0],pNVSIKey[1],pNVSIKey[2],pNVSIKey[3],
    pNVSIKey[4],pNVSIKey[5],pNVSIKey[6],pNVSIKey[7],
    pNVSIKey[8],pNVSIKey[9],pNVSIKey[10],pNVSIKey[11],
    pNVSIKey[12],pNVSIKey[13],pNVSIKey[14],pNVSIKey[15]);
  nvsi_log("KEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    pNVSIKey[16+0],pNVSIKey[16+1],pNVSIKey[16+2],pNVSIKey[16+3],
    pNVSIKey[16+4],pNVSIKey[16+5],pNVSIKey[16+6],pNVSIKey[16+7],
    pNVSIKey[16+8],pNVSIKey[16+9],pNVSIKey[16+10],pNVSIKey[16+11],
    pNVSIKey[16+12],pNVSIKey[16+13],pNVSIKey[16+14],pNVSIKey[16+15]);
  nvsi_log("     %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    pNVSIKey[32+0],pNVSIKey[32+1],pNVSIKey[32+2],pNVSIKey[32+3],
    pNVSIKey[32+4],pNVSIKey[32+5],pNVSIKey[32+6],pNVSIKey[32+7],
    pNVSIKey[32+8],pNVSIKey[32+9],pNVSIKey[32+10],pNVSIKey[32+11],
    pNVSIKey[32+12],pNVSIKey[32+13],pNVSIKey[32+14],pNVSIKey[32+15]);
  nvsi_log("MAGIC: %02x %02x %02x %02x",pNVSIKey[48],pNVSIKey[49],pNVSIKey[50],pNVSIKey[51]);*/
  
  // 16B id, 32B OEM key and 4B magic constant
  SMemMove(buf,pNVSIKey,min(52,*length));  

  /*Destroy TF-SBK key*/
  nError = C_DestroyObject(hSession, hSBKKey);
  if (nError != CKR_OK) {
      _SLogError("oemcrypto_get_nvsi_keybox : C_DestroyObject() failed (%08x).", nError);
      return nError;
  }
  
  (void)C_CloseSession(hSession);  

  return nError;
}
