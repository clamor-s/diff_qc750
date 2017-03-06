/**
 * Copyright (c) 2008-2009 Trusted Logic S.A.
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
#include "cryptocatalogue_protocol.h"
#include "cryptocatalogue_service.h"

/* ----------------------------------------------------------------------------
*   Defines
* ---------------------------------------------------------------------------- */
#define ATTRIBUTES_SIZE(pArray) (sizeof(pArray)/sizeof(CK_ATTRIBUTE))

/* ----------------------------------------------------------------------------
 *   Key Creation / Generation
 * ---------------------------------------------------------------------------- */
/**
 *  REMARK:
 *  For all createXXX and generateXXX functions, when the input bIsToken == CK_FALSE,
 *  the keys are session objects, thus the CKA_ID attribute of the templates
 *  is ignored (it could be removed).
 *
 **/

/**
 *  Function createRsaKeyPair:
 *  Description: 
 *           Create a RSA key pair from templates. 
 *           The keys values are constants declared in cryptocatalogue_service.h.
 *  Input:   S_HANDLE  hCryptoSession      = cryptoki session handle
 *           CK_BYTE*   privateKeyId       = private key object id (2 bytes)
 *           CK_BYTE*   publicKeyId        = public key object id (2 bytes)
 *           CK_BBOOL   bIsToken           = true if and only if the keys are token objects
 *  Output:  CK_OBJECT_HANDLE* phPrivateKey = points to the private key handle
 *           CK_OBJECT_HANDLE* phPublicKey  = points to the public key handle
 **/
static CK_RV createRsaKeyPair(IN  S_HANDLE  hCryptoSession,
                                 IN  CK_BYTE*   privateKeyId, /* 2 bytes */
                                 IN CK_BYTE*   publicKeyId,  /* 2 bytes */
                                 IN CK_BBOOL   bIsToken,
                                 OUT CK_OBJECT_HANDLE* phPrivateKey,
                                 OUT CK_OBJECT_HANDLE* phPublicKey)
{
   CK_RV       nError;
//   static const CK_BYTE myID[2];

   /* Remark:
   *  At least one of CKA_SIGN or CKA_DECRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   CK_ATTRIBUTE privateKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_private_key_class,  sizeof(test_private_key_class) },
      {CKA_ID,                NULL,             2}, 
      {CKA_TOKEN,             NULL,             sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_rsa_key_type,       sizeof(test_rsa_key_type) },
      {CKA_SIGN,              (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_MODULUS,           (CK_VOID_PTR)test_rsa_1024_modulus,    sizeof(test_rsa_1024_modulus) },
      {CKA_PUBLIC_EXPONENT,   (CK_VOID_PTR)test_rsa_1024_pub_e,      sizeof(test_rsa_1024_pub_e) },
      {CKA_PRIVATE_EXPONENT,  (CK_VOID_PTR)test_rsa_1024_priv_d,     sizeof(test_rsa_1024_priv_d) },
      {CKA_PRIME_1,           (CK_VOID_PTR)test_rsa_1024_primP,      sizeof(test_rsa_1024_primP) },
      {CKA_PRIME_2,           (CK_VOID_PTR)test_rsa_1024_primQ,      sizeof(test_rsa_1024_primQ) },
      {CKA_EXPONENT_1,        (CK_VOID_PTR)test_rsa_1024_dP,         sizeof(test_rsa_1024_dP) },
      {CKA_EXPONENT_2,        (CK_VOID_PTR)test_rsa_1024_dQ,         sizeof(test_rsa_1024_dQ) },
      {CKA_COEFFICIENT,       (CK_VOID_PTR)test_rsa_1024_qInv,       sizeof(test_rsa_1024_qInv) }
   };

   /* Remark:
   *  At least one of CKA_VERIFY or CKA_ENCRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   CK_ATTRIBUTE publicKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_public_key_class,   sizeof(test_public_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_rsa_key_type,       sizeof(test_rsa_key_type) },
      {CKA_VERIFY,            (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_MODULUS,           (CK_VOID_PTR)test_rsa_1024_modulus,    sizeof(test_rsa_1024_modulus) },
      {CKA_PUBLIC_EXPONENT,   (CK_VOID_PTR)test_rsa_1024_pub_e,      sizeof(test_rsa_1024_pub_e) }
   };
   
   privateKeyTemplate[1].pValue = privateKeyId;
   privateKeyTemplate[2].pValue = &bIsToken;
   publicKeyTemplate[1].pValue = publicKeyId;
   publicKeyTemplate[2].pValue = &bIsToken;

   /* Create a RSA private key (1024 bits) */
   nError = C_CreateObject(
      hCryptoSession,
      privateKeyTemplate,
      ATTRIBUTES_SIZE(privateKeyTemplate),
      phPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }
   /* Create a RSA public key (1024 bits) */
   nError = C_CreateObject(
      hCryptoSession,
      publicKeyTemplate,
      ATTRIBUTES_SIZE(publicKeyTemplate),
      phPublicKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      C_DestroyObject(hCryptoSession, *phPrivateKey);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function createRsaPssKeyPair: like createRsaKeyPair but with PSS keys value.
 **/
static CK_RV createRsaPssKeyPair(IN  S_HANDLE  hCryptoSession,
                                    IN CK_BYTE*   privateKeyId, /* 2 bytes */
                                    IN CK_BYTE*   publicKeyId,  /* 2 bytes */
                                    IN CK_BBOOL   bIsToken,
                                    OUT CK_OBJECT_HANDLE* phPrivateKey,
                                    OUT CK_OBJECT_HANDLE* phPublicKey)
{
   CK_RV       nError;

   CK_ATTRIBUTE privateKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_private_key_class,     sizeof(test_private_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_rsa_key_type,          sizeof(test_rsa_key_type) },
      {CKA_SIGN,              (CK_VOID_PTR)&test_true,                  sizeof(test_true) }, 
      {CKA_MODULUS,           (CK_VOID_PTR)test_rsa_pss_1024_modulus,   sizeof(test_rsa_pss_1024_modulus) },
      {CKA_PUBLIC_EXPONENT,   (CK_VOID_PTR)test_rsa_pss_1024_pub_e,     sizeof(test_rsa_pss_1024_pub_e) },
      {CKA_PRIVATE_EXPONENT,  (CK_VOID_PTR)test_rsa_pss_1024_priv_d,    sizeof(test_rsa_pss_1024_priv_d) },
      {CKA_PRIME_1,           (CK_VOID_PTR)test_rsa_pss_1024_primP,     sizeof(test_rsa_pss_1024_primP) },
      {CKA_PRIME_2,           (CK_VOID_PTR)test_rsa_pss_1024_primQ,     sizeof(test_rsa_pss_1024_primQ) },
      {CKA_EXPONENT_1,        (CK_VOID_PTR)test_rsa_pss_1024_dP,        sizeof(test_rsa_pss_1024_dP) },
      {CKA_EXPONENT_2,        (CK_VOID_PTR)test_rsa_pss_1024_dQ,        sizeof(test_rsa_pss_1024_dQ) },
      {CKA_COEFFICIENT,       (CK_VOID_PTR)test_rsa_pss_1024_qInv,      sizeof(test_rsa_pss_1024_qInv) }
   };

   CK_ATTRIBUTE publicKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_public_key_class,      sizeof(test_public_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_rsa_key_type,          sizeof(test_rsa_key_type) },
      {CKA_VERIFY,            (CK_VOID_PTR)&test_true,                  sizeof(test_true) }, 
      {CKA_MODULUS,           (CK_VOID_PTR)test_rsa_pss_1024_modulus,   sizeof(test_rsa_pss_1024_modulus) },
      {CKA_PUBLIC_EXPONENT,   (CK_VOID_PTR)test_rsa_pss_1024_pub_e,     sizeof(test_rsa_pss_1024_pub_e) }
   };

   privateKeyTemplate[1].pValue = privateKeyId;
   privateKeyTemplate[2].pValue = &bIsToken;
   publicKeyTemplate[1].pValue = publicKeyId;
   publicKeyTemplate[2].pValue = &bIsToken;
   
   /* Create a RSA private key */
   nError = C_CreateObject(
      hCryptoSession,
      privateKeyTemplate,
      ATTRIBUTES_SIZE(privateKeyTemplate),
      phPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }
   /* Create a RSA public key */
   nError = C_CreateObject(
      hCryptoSession,
      publicKeyTemplate,
      ATTRIBUTES_SIZE(publicKeyTemplate),
      phPublicKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      C_DestroyObject(hCryptoSession, *phPrivateKey);
      return nError;
   }

   return CKR_OK;
}

/*
 *  Function createRsaOaepKeyPair: like createRsaKeyPair but with OAEP keys value.
 */
static CK_RV createRsaOaepKeyPair(IN  S_HANDLE  hCryptoSession,
                                     IN CK_BYTE*   privateKeyId, /* 2 bytes */
                                     IN CK_BYTE*   publicKeyId,  /* 2 bytes */
                                     IN CK_BBOOL   bIsToken,
                                     OUT CK_OBJECT_HANDLE* phPrivateKey,
                                     OUT CK_OBJECT_HANDLE* phPublicKey)
{
   CK_RV       nError;

   CK_ATTRIBUTE privateKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_private_key_class,     sizeof(test_private_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_rsa_key_type,          sizeof(test_rsa_key_type) },
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,                  sizeof(test_true) }, 
      {CKA_MODULUS,           (CK_VOID_PTR)test_rsa_oaep_1024_modulus,  sizeof(test_rsa_oaep_1024_modulus) },
      {CKA_PUBLIC_EXPONENT,   (CK_VOID_PTR)test_rsa_oaep_1024_pub_e,    sizeof(test_rsa_oaep_1024_pub_e) },
      {CKA_PRIVATE_EXPONENT,  (CK_VOID_PTR)test_rsa_oaep_1024_priv_d,   sizeof(test_rsa_oaep_1024_priv_d) },
      {CKA_PRIME_1,           (CK_VOID_PTR)test_rsa_oaep_1024_primP,    sizeof(test_rsa_oaep_1024_primP) },
      {CKA_PRIME_2,           (CK_VOID_PTR)test_rsa_oaep_1024_primQ,    sizeof(test_rsa_oaep_1024_primQ) },
      {CKA_EXPONENT_1,        (CK_VOID_PTR)test_rsa_oaep_1024_dP,       sizeof(test_rsa_oaep_1024_dP) },
      {CKA_EXPONENT_2,        (CK_VOID_PTR)test_rsa_oaep_1024_dQ,       sizeof(test_rsa_oaep_1024_dQ) },
      {CKA_COEFFICIENT,       (CK_VOID_PTR)test_rsa_oaep_1024_qInv,     sizeof(test_rsa_oaep_1024_qInv) }
   };

   CK_ATTRIBUTE publicKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_public_key_class,      sizeof(test_public_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_rsa_key_type,          sizeof(test_rsa_key_type) },
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,                  sizeof(test_true) }, 
      {CKA_MODULUS,           (CK_VOID_PTR)test_rsa_oaep_1024_modulus,  sizeof(test_rsa_oaep_1024_modulus) },
      {CKA_PUBLIC_EXPONENT,   (CK_VOID_PTR)test_rsa_oaep_1024_pub_e,    sizeof(test_rsa_oaep_1024_pub_e) }
   };
   
   privateKeyTemplate[1].pValue = privateKeyId;
   privateKeyTemplate[2].pValue = &bIsToken;
   publicKeyTemplate[1].pValue = publicKeyId;
   publicKeyTemplate[2].pValue = &bIsToken;   

   /* Create a RSA private key  */
   nError = C_CreateObject(
      hCryptoSession,
      privateKeyTemplate,
      ATTRIBUTES_SIZE(privateKeyTemplate),
      phPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }
   /* Create a RSA public key  */
   nError = C_CreateObject(
      hCryptoSession,
      publicKeyTemplate,
      ATTRIBUTES_SIZE(publicKeyTemplate),
      phPublicKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      C_DestroyObject(hCryptoSession, *phPrivateKey);
      return nError;
   }

   return CKR_OK;
}

/*
 *  Function generateRsaKeyPair:
 *  Description: 
 *           Generate a RSA key pair from templates.
 *  Input:   S_HANDLE  hCryptoSession      = cryptoki session handle
 *           CK_BYTE*   privateKeyId       = private key object id (2 bytes)
 *           CK_BYTE*   publicKeyId        = public key object id (2 bytes)
 *           CK_BBOOL   bIsToken           = true if and only if the keys are token objects
 *  Output:  CK_OBJECT_HANDLE* phPrivateKey = points to the private key handle
 *           CK_OBJECT_HANDLE* phPublicKey  = points to the public key handle
 *
 */
static CK_RV generateRsaKeyPair(IN  S_HANDLE  hCryptoSession,
                                   IN CK_BYTE*   privateKeyId, /* 2 bytes */
                                   IN CK_BYTE*   publicKeyId,  /* 2 bytes */
                                   IN CK_BBOOL   bIsToken,
                                   OUT CK_OBJECT_HANDLE* phPrivateKey,
                                   OUT CK_OBJECT_HANDLE* phPublicKey)
{
   CK_RV       nError;
   CK_MECHANISM mechanism = {CKM_RSA_PKCS_KEY_PAIR_GEN, NULL_PTR, 0};

   /* Remark:
   *  At least one of CKA_SIGN or CKA_DECRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   CK_ATTRIBUTE privateKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_private_key_class,  sizeof(test_private_key_class) }, /* optional */
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_rsa_key_type,       sizeof(test_rsa_key_type) },/* optional */
      {CKA_SIGN,              (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
   };

   /* Remark:
   *  At least one of CKA_VERIFY or CKA_ENCRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   static const CK_ULONG modulusBits = 1024;
   CK_ATTRIBUTE publicKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_public_key_class,   sizeof(test_public_key_class) }, /* optional */
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_rsa_key_type,       sizeof(test_rsa_key_type) },/* optional */
      {CKA_VERIFY,            (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_MODULUS_BITS,      (CK_VOID_PTR)&modulusBits,             sizeof(modulusBits) },
      {CKA_PUBLIC_EXPONENT,   (CK_VOID_PTR)test_rsa_1024_pub_e,      sizeof(test_rsa_1024_pub_e) } /* optional: default value of 0x10001 (65537) */
   };

   privateKeyTemplate[1].pValue = privateKeyId;
   privateKeyTemplate[2].pValue = &bIsToken;
   publicKeyTemplate[1].pValue = publicKeyId;
   publicKeyTemplate[2].pValue = &bIsToken;

   /* generate a RSA private key (1024 bits) */
   nError = C_GenerateKeyPair(
      hCryptoSession,
      &mechanism,
      publicKeyTemplate,
      ATTRIBUTES_SIZE(publicKeyTemplate),
      privateKeyTemplate,
      ATTRIBUTES_SIZE(privateKeyTemplate),
      phPublicKey,
      phPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_GenerateKeyPair returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/*
 *  Function createDsaKeyPair:
 *  Description: 
 *           Create a DSA key pair from templates.
 *           The keys values are constants declared in cryptocatalogue_service.h.
 *  Input:   S_HANDLE  hCryptoSession      = cryptoki session handle
 *           CK_BYTE*   privateKeyId       = private key object id (2 bytes)
 *           CK_BYTE*   publicKeyId        = public key object id (2 bytes)
 *           CK_BBOOL   bIsToken           = true if and only if the keys are token objects
 *  Output:  CK_OBJECT_HANDLE* phPrivateKey = points to the private key handle
 *           CK_OBJECT_HANDLE* phPublicKey  = points to the public key handle
 */
static CK_RV createDsaKeyPair(IN  S_HANDLE  hCryptoSession,
                                 IN CK_BYTE*   privateKeyId, /* 2 bytes */
                                 IN CK_BYTE*   publicKeyId,  /* 2 bytes */
                                 IN CK_BBOOL   bIsToken,
                                 OUT CK_OBJECT_HANDLE* phPrivateKey,
                                 OUT CK_OBJECT_HANDLE* phPublicKey)
{
   CK_RV       nError;

   CK_ATTRIBUTE privateKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_private_key_class,  sizeof(test_private_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_dsa_key_type,       sizeof(test_dsa_key_type) },
      {CKA_SIGN,              (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_PRIME,             (CK_VOID_PTR)&test_dsa_512_P,          sizeof(test_dsa_512_P) },
      {CKA_SUBPRIME,          (CK_VOID_PTR)&test_dsa_512_Q,          sizeof(test_dsa_512_Q) },
      {CKA_BASE,              (CK_VOID_PTR)&test_dsa_512_G,          sizeof(test_dsa_512_G) },
      {CKA_VALUE,             (CK_VOID_PTR)&test_dsa_512_private_X,  sizeof(test_dsa_512_private_X) },
   };

   CK_ATTRIBUTE publicKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_public_key_class,   sizeof(test_public_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_dsa_key_type,       sizeof(test_dsa_key_type) },
      {CKA_VERIFY,            (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_PRIME,             (CK_VOID_PTR)&test_dsa_512_P,          sizeof(test_dsa_512_P) },
      {CKA_SUBPRIME,          (CK_VOID_PTR)&test_dsa_512_Q,          sizeof(test_dsa_512_Q) },
      {CKA_BASE,              (CK_VOID_PTR)&test_dsa_512_G,          sizeof(test_dsa_512_G) },
      {CKA_VALUE,             (CK_VOID_PTR)&test_dsa_512_public_Y,   sizeof(test_dsa_512_public_Y) },
   };
   
   privateKeyTemplate[1].pValue = privateKeyId;
   privateKeyTemplate[2].pValue = &bIsToken;
   publicKeyTemplate[1].pValue = publicKeyId;
   publicKeyTemplate[2].pValue = &bIsToken;   

   /* Create a DSA private key */
   nError = C_CreateObject(
      hCryptoSession,
      privateKeyTemplate,
      ATTRIBUTES_SIZE(privateKeyTemplate),
      phPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }
   /* Create a DSA public key */
   nError = C_CreateObject(
      hCryptoSession,
      publicKeyTemplate,
      ATTRIBUTES_SIZE(publicKeyTemplate),
      phPublicKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      C_DestroyObject(hCryptoSession, *phPrivateKey);
      return nError;
   }

   return CKR_OK;
}

/*
 *  Function generateDsaKeyPair:
 *  Description: 
 *           Generate a DSA key pair from templates.
 *  Input:   S_HANDLE  hCryptoSession      = cryptoki session handle
 *           CK_BYTE*   privateKeyId       = private key object id (2 bytes)
 *           CK_BYTE*   publicKeyId        = public key object id (2 bytes)
 *           CK_BBOOL   bIsToken           = true if and only if the keys are token objects
 *  Output:  CK_OBJECT_HANDLE* phPrivateKey = points to the private key handle
 *           CK_OBJECT_HANDLE* phPublicKey  = points to the public key handle
 */
static CK_RV generateDsaKeyPair(IN  S_HANDLE  hCryptoSession,
                                   IN CK_BYTE*   privateKeyId, /* 2 bytes */
                                   IN CK_BYTE*   publicKeyId,  /* 2 bytes */
                                   IN CK_BBOOL   bIsToken,
                                   OUT CK_OBJECT_HANDLE* phPrivateKey,
                                   OUT CK_OBJECT_HANDLE* phPublicKey)
{
   CK_RV       nError;
   CK_MECHANISM mechanism = {CKM_DSA_KEY_PAIR_GEN, NULL_PTR, 0};

   CK_ATTRIBUTE privateKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_private_key_class,  sizeof(test_private_key_class) },/* optional */
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_dsa_key_type,       sizeof(test_dsa_key_type) },/* optional */
      {CKA_SIGN,              (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
   };

   CK_ATTRIBUTE publicKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_public_key_class,   sizeof(test_public_key_class) },/* optional */
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_dsa_key_type,       sizeof(test_dsa_key_type) },/* optional */
      {CKA_VERIFY,            (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_PRIME,             (CK_VOID_PTR)&test_dsa_512_P,          sizeof(test_dsa_512_P) },
      {CKA_SUBPRIME,          (CK_VOID_PTR)&test_dsa_512_Q,          sizeof(test_dsa_512_Q) },
      {CKA_BASE,              (CK_VOID_PTR)&test_dsa_512_G,          sizeof(test_dsa_512_G) },
   };
   
   privateKeyTemplate[1].pValue = privateKeyId;
   privateKeyTemplate[2].pValue = &bIsToken;
   publicKeyTemplate[1].pValue = publicKeyId;
   publicKeyTemplate[2].pValue = &bIsToken;   

   /* generate a DSA private key (512 bits) */
   nError = C_GenerateKeyPair(
      hCryptoSession,
      &mechanism,
      publicKeyTemplate,
      ATTRIBUTES_SIZE(publicKeyTemplate),
      privateKeyTemplate,
      ATTRIBUTES_SIZE(privateKeyTemplate),
      phPublicKey,
      phPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_GenerateKeyPair returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/*
 *  Function createAesKey128:
 *  Description:
 *           Create a AES key of 128 bits length from a template.
 *           The key value is a constant declared in cryptocatalogue_service.h.
 *  Input:   S_HANDLE  hCryptoSession      = cryptoki session handle
 *           CK_BYTE*   secretKeyId        = secret key object id (2 bytes)
 *           CK_BBOOL   bIsToken           = true if and only if the key is a token object
 *  Output:  CK_OBJECT_HANDLE* phSecretKey = points to the secret key handle
 */
static CK_RV createAesKey128(IN  S_HANDLE  hCryptoSession,
                                IN CK_BYTE*   secretKeyId, /* 2 bytes */
                                IN CK_BBOOL   bIsToken,
                                OUT CK_OBJECT_HANDLE* phSecretKey)
{
   CK_RV       nError;

   /* Remark:
   *  At least one of CKA_ENCRYPT or CKA_DECRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   CK_ATTRIBUTE secretKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) },  
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_aes_key_type,       sizeof(test_aes_key_type) },
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_VALUE,             (CK_VOID_PTR)test_aes_128_key,         sizeof(test_aes_128_key) },
   };

   secretKeyTemplate[1].pValue = secretKeyId;
   secretKeyTemplate[2].pValue = &bIsToken;   
   
   /* Create a AES secret key (128 bits) */
   nError = C_CreateObject(
      hCryptoSession,
      secretKeyTemplate,
      ATTRIBUTES_SIZE(secretKeyTemplate),
      phSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/*
 * Function generateAesKey128:
 *  Description:
 *           Generate a AES key of 128 bits length from a template.
 *  Input:   S_HANDLE  hCryptoSession      = cryptoki session handle
 *           CK_BYTE*   secretKeyId        = secret key object id (2 bytes)
 *           CK_BBOOL   bIsToken           = true if and only if the key is a token object
 *  Output:  CK_OBJECT_HANDLE* phSecretKey = points to the secret key handle
 */
static CK_RV generateAesKey128(IN  S_HANDLE  hCryptoSession,
                                  IN CK_BYTE*   secretKeyId, /* 2 bytes */
                                  IN CK_BBOOL   bIsToken,
                                  OUT CK_OBJECT_HANDLE* phSecretKey)
{
   CK_RV       nError;
   CK_MECHANISM mechanism = {CKM_AES_KEY_GEN, NULL_PTR, 0};

   /* Remark:
   *  At least one of CKA_ENCRYPT or CKA_DECRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   static const CK_ULONG secretKeyLength = 16; /* choose the key length : here 128 bits = 16 bytes */
   CK_ATTRIBUTE secretKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },/* optional */
      {CKA_ID,                NULL,              2},  /* mandatory if bIsToken=true, else ignored */
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_aes_key_type,       sizeof(test_aes_key_type) },/* optional */
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_VALUE_LEN,         (CK_VOID_PTR)&secretKeyLength,         sizeof(secretKeyLength) },
   };
   
   secretKeyTemplate[1].pValue = secretKeyId;
   secretKeyTemplate[2].pValue = &bIsToken;   

   /* Generate a AES secret key (128 bits) */
   nError = C_GenerateKey(
      hCryptoSession,
      &mechanism,
      secretKeyTemplate,
      ATTRIBUTES_SIZE(secretKeyTemplate),
      phSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_GenerateKey returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/*
 *  Function createAesKey192:
 *  Description:
 *           Create a AES key of 192 bits length from a template.
 *           The key value is a constant declared in cryptocatalogue_service.h.
 *  Inputs/Outputs: same as createAesKey128.
 */
static CK_RV createAesKey192(IN  S_HANDLE  hCryptoSession,
                                IN CK_BYTE*   secretKeyId, /* 2 bytes */
                                IN CK_BBOOL   bIsToken,
                                OUT CK_OBJECT_HANDLE* phSecretKey)
{
   CK_RV       nError;

   /* Remark:
   *  At least one of CKA_ENCRYPT or CKA_DECRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   CK_ATTRIBUTE secretKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_aes_key_type,       sizeof(test_aes_key_type) },
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_VALUE,             (CK_VOID_PTR)test_aes_192_key,         sizeof(test_aes_192_key) },
   };
   
   secretKeyTemplate[1].pValue = secretKeyId;
   secretKeyTemplate[2].pValue = &bIsToken; 

   /* Create a AES secret key (192 bits) */
   nError = C_CreateObject(
      hCryptoSession,
      secretKeyTemplate,
      ATTRIBUTES_SIZE(secretKeyTemplate),
      phSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/*
 *  Function createAesKey256:
 *  Description:
 *           Create a AES key of 256 bits length from a template.
 *           The key value is a constant declared in cryptocatalogue_service.h.
 *  Inputs/Outputs: same as createAesKey128.
 */
static CK_RV createAesKey256(IN  S_HANDLE  hCryptoSession,
                                IN CK_BYTE*   secretKeyId, /* 2 bytes */
                                IN CK_BBOOL   bIsToken,
                                OUT CK_OBJECT_HANDLE* phSecretKey)
{
   CK_RV       nError;

   /* Remark:
   *  At least one of CKA_ENCRYPT or CKA_DECRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   CK_ATTRIBUTE secretKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_aes_key_type,       sizeof(test_aes_key_type) },
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_VALUE,             (CK_VOID_PTR)test_aes_256_key,         sizeof(test_aes_256_key) },
   };
   
   secretKeyTemplate[1].pValue = secretKeyId;
   secretKeyTemplate[2].pValue = &bIsToken;  

   /* Create a AES secret key (256 bits) */
   nError = C_CreateObject(
      hCryptoSession,
      secretKeyTemplate,
      ATTRIBUTES_SIZE(secretKeyTemplate),
      phSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/*
 *  Function createDesKey:
 *  Description:
 *           Create a DES key of 64 bits length from a template.
 *           The key value is a constant declared in cryptocatalogue_service.h.
 *  Input:   S_HANDLE  hCryptoSession      = cryptoki session handle
 *           CK_BYTE*   secretKeyId        = secret key object id (2 bytes)
 *           CK_BBOOL   bIsToken           = true if and only if the key is a token object
 *  Output:  CK_OBJECT_HANDLE* phSecretKey = points to the secret key handle
 */
static CK_RV createDesKey(IN  S_HANDLE  hCryptoSession,
                                IN CK_BYTE*   secretKeyId, /* 2 bytes */
                                IN CK_BBOOL   bIsToken,
                                OUT CK_OBJECT_HANDLE* phSecretKey)
{
   CK_RV       nError;

   /* Remark:
   *  At least one of CKA_ENCRYPT or CKA_DECRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   CK_ATTRIBUTE secretKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_des_key_type,       sizeof(test_des_key_type) },
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_VALUE,             (CK_VOID_PTR)test_des_key,             sizeof(test_des_key) },
   };
   
   secretKeyTemplate[1].pValue = secretKeyId;
   secretKeyTemplate[2].pValue = &bIsToken;   

   /* Create a DES secret key (64 bits) */
   nError = C_CreateObject(
      hCryptoSession,
      secretKeyTemplate,
      ATTRIBUTES_SIZE(secretKeyTemplate),
      phSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/*
 *  Function createDes2Key:
 *  Description:
 *           Create a DES2 key of 128 bits length from a template.
 *           The key value is a constant declared in cryptocatalogue_service.h.
 *  Inputs/Outputs: same as createDesKey.
 */
static CK_RV createDes2Key(IN  S_HANDLE  hCryptoSession,
                                IN CK_BYTE*   secretKeyId, /* 2 bytes */
                                IN CK_BBOOL   bIsToken,
                                OUT CK_OBJECT_HANDLE* phSecretKey)
{
   CK_RV       nError;

   /* Remark:
   *  At least one of CKA_ENCRYPT or CKA_DECRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   CK_ATTRIBUTE secretKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_des2_key_type,      sizeof(test_des2_key_type) },
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_VALUE,             (CK_VOID_PTR)test_des2_key,            sizeof(test_des2_key) },
   };
   
   secretKeyTemplate[1].pValue = secretKeyId;
   secretKeyTemplate[2].pValue = &bIsToken;   

   /* Create a DES2 secret key (64*2 bits) */
   nError = C_CreateObject(
      hCryptoSession,
      secretKeyTemplate,
      ATTRIBUTES_SIZE(secretKeyTemplate),
      phSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/*
 *  Function generateDes2Key:
 *  Description:
 *           Generate a DES2 key of 128 bits length from a template.
 *  Input:   S_HANDLE  hCryptoSession      = cryptoki session handle
 *           CK_BYTE*   secretKeyId        = secret key object id (2 bytes)
 *           CK_BBOOL   bIsToken           = true if and only if the key is a token object
 *  Output:  CK_OBJECT_HANDLE* phSecretKey = points to the secret key handle
 */
static CK_RV generateDes2Key(IN  S_HANDLE  hCryptoSession,
                                IN CK_BYTE*   secretKeyId, /* 2 bytes */
                                IN CK_BBOOL   bIsToken,
                                OUT CK_OBJECT_HANDLE* phSecretKey)
{
   CK_RV       nError;


   /* Remark:
   *  if you want to generate  a DES key (respectively DES2 or DES3 key), 
   *  choose the mechanism CKM_DES_KEY_GEN (respectively CKM_DES2_KEY_GEN or 
   *  CKM_DES3_KEY_GEN) 
   */
   CK_MECHANISM mechanism = {CKM_DES2_KEY_GEN, NULL_PTR, 0};

   /* Remark:
   *  At least one of CKA_ENCRYPT or CKA_DECRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   CK_ATTRIBUTE secretKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },/* optional */
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_des2_key_type,      sizeof(test_des2_key_type) },/* optional */
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
   };
   
   secretKeyTemplate[1].pValue = secretKeyId;
   secretKeyTemplate[2].pValue = &bIsToken;   

   /* Generate a DES2 secret key (64*2 bits) */
   nError = C_GenerateKey(
      hCryptoSession,
      &mechanism,
      secretKeyTemplate,
      ATTRIBUTES_SIZE(secretKeyTemplate),
      phSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_GenerateKey returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}


/*
 *  Function createDes3Key:
 *  Description:
 *           Create a DES3 key of 192 bits length from a template.
 *           The key value is a constant declared in cryptocatalogue_service.h.
 *  Inputs/Outputs: same as createDesKey.
 */
static CK_RV createDes3Key(IN  S_HANDLE  hCryptoSession,
                                IN CK_BYTE*   secretKeyId, /* 2 bytes */
                                IN CK_BBOOL   bIsToken,
                                OUT CK_OBJECT_HANDLE* phSecretKey)
{
   CK_RV       nError;

   /* Remark:
   *  At least one of CKA_ENCRYPT or CKA_DECRYPT attribute must be present in the template 
   *  and set to CK_TRUE, depending on the key usage.
   */
   CK_ATTRIBUTE secretKeyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_des3_key_type,      sizeof(test_des3_key_type) },
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_VALUE,             (CK_VOID_PTR)test_des3_key,            sizeof(test_des3_key) },
   };
   
   secretKeyTemplate[1].pValue = secretKeyId;
   secretKeyTemplate[2].pValue = &bIsToken;   

   /* Create a DES3 secret key (64*3 bits) */
   nError = C_CreateObject(
      hCryptoSession,
      secretKeyTemplate,
      ATTRIBUTES_SIZE(secretKeyTemplate),
      phSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/* ----------------------------------------------------------------------------
*   Digest generic functions
* ---------------------------------------------------------------------------- */
/**
 *  Function digestOneStage:
 *  Description:
 *           Perform a single stage digest.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_MECHANISM* pMechanism      = points to the digest mechanism
 *           CK_BYTE*      pData           = data
 *           CK_ULONG      nDataLen        = data length
 *  Output:  CK_BYTE*      pDigest         = digest
 *  In/Out:  CK_ULONG*     pnDigestLen     = points to digest length
 *
 **/
static CK_RV digestOneStage(IN S_HANDLE      hCryptoSession,
                               IN CK_MECHANISM* pMechanism,
                               IN CK_BYTE*      pData,
                               IN CK_ULONG      nDataLen,
                               OUT CK_BYTE*     pDigest,
                               IN OUT CK_ULONG* pnDigestLen)
{
   CK_RV       nError;

   nError = C_DigestInit(hCryptoSession, pMechanism);
   if (nError != CKR_OK)
   {
      _SLogError("C_DigestInit returned (%08x).", nError);
      return nError;
   }
   
   nError = C_Digest(hCryptoSession, pData, nDataLen, pDigest, pnDigestLen);
   if (nError != CKR_OK)
   {
      _SLogError("C_Digest returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function digestMultiStages:
 *  Description:
 *           Perform a multi stages digest, with updates on data blocks of size nBlockLen. 
 *           The last block length can be less or equal to nBlockLen.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_MECHANISM* pMechanism      = points to the digest mechanism
 *           CK_BYTE*      pData           = data
 *           CK_ULONG      nDataLen        = data length
 *           CK_ULONG      nBlockLen       = data block length
 *  Output:  CK_BYTE*      pDigest         = digest
 *  In/Out:  CK_ULONG*     pnDigestLen     = points to digest length
 **/
static CK_RV digestMultiStages(IN S_HANDLE      hCryptoSession,
                                  IN CK_MECHANISM* pMechanism,
                                  IN CK_BYTE*      pData,
                                  IN CK_ULONG      nDataLen,
                                  IN CK_ULONG      nBlockLen,
                                  OUT CK_BYTE*     pDigest,
                                  IN OUT CK_ULONG* pnDigestLen)
{
   CK_RV          nError;
   CK_ULONG       nBlockCount;
   CK_ULONG       nLastBlockLen;

   nError = C_DigestInit(hCryptoSession, pMechanism);
   if (nError != CKR_OK)
   {
      _SLogError("C_DigestInit returned (%08x).", nError);
      return nError;
   }
   
   for(nBlockCount = 0; nBlockCount < (nDataLen/nBlockLen); nBlockCount++)
   {
      nError = C_DigestUpdate(hCryptoSession, pData, nBlockLen);
      if (nError != CKR_OK)
      {
         _SLogError("C_DigestUpdate returned (%08x).", nError);
         return nError;
      }
      pData += nBlockLen;
   }
   nLastBlockLen = (nDataLen % nBlockLen);
   if (nLastBlockLen !=0)
   {
      nError = C_DigestUpdate(hCryptoSession, pData, nLastBlockLen);
      if (nError != CKR_OK)
      {
         _SLogError("C_DigestUpdate returned (%08x).", nError);
         return nError;
      }
   }

   nError = C_DigestFinal(hCryptoSession, pDigest, pnDigestLen);
   if (nError != CKR_OK)
   {
      _SLogError("C_DigestFinal returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/* ----------------------------------------------------------------------------
 *   Encryption/Decryption generic functions
 * ---------------------------------------------------------------------------- */
/**
 *  Function encryptOneStage:
 *  Description:
 *           Perform a single stage encryption.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_MECHANISM* pMechanism      = points to the encryption mechanism
 *           CK_OBJECT_HANDLE   hKey       = encryption key handle
 *           CK_BYTE*      pData           = data
 *           CK_ULONG      nDataLen        = data length
 *  Output:  CK_BYTE*      pEncryptedData  = encrypted data
 *  In/Out:  CK_ULONG*     pnEncryptedDataLen = points to encrypted data length
 *  
 *  REMARK: 
 *  1) The encrypted data length can be different from the data length.
 *  For instance, for RSA OAEP encrytion the ciphertext length is superior to 
 *  the plaintext length (since there is padding). 
 *  2) For AES/DES/TripleDES algorithms, nDataLen MUST be a multiple 
 *  of the AES/DES block length (i.e. respectively 16 bytes and 8 bytes).
 **/
static CK_RV encryptOneStage(IN S_HANDLE           hCryptoSession, 
                                IN CK_MECHANISM*      pMechanism,
                                IN CK_OBJECT_HANDLE   hKey,
                                IN CK_BYTE*           pData,
                                IN CK_ULONG           nDataLen,
                                OUT CK_BYTE*          pEncryptedData,
                                IN OUT CK_ULONG*      pnEncryptedDataLen)
{
   CK_RV       nError;

   nError = C_EncryptInit(hCryptoSession, pMechanism, hKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_EncryptInit returned (%08x).", nError);
      return nError;
   }
   
   nError = C_Encrypt(hCryptoSession, pData, nDataLen, pEncryptedData, pnEncryptedDataLen);
   if (nError != CKR_OK)
   {
      _SLogError("C_Encrypt returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function encryptMultiStages:
 *  Description:
 *           Perform a multi stages encryption, with updates on data blocks of size nBlockLen. 
 *           The last block length can be less or equal to nBlockLen.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_MECHANISM* pMechanism      = points to the encryption mechanism
 *           CK_OBJECT_HANDLE   hKey       = encryption key handle
 *           CK_BYTE*      pData           = data
 *           CK_ULONG      nDataLen        = data length
 *           CK_ULONG      nBlockLen       = data block length
 *  Output:  CK_BYTE*      pEncryptedDataLen  = encrypted data
 *
 *  PRECONDITION: 
 *  The mechanism is such that the ciphertext length is equal to the plaintext length.
 *
 *  REMARKS: 
 *  1) The function encryptMultiStages() treats ONLY the case where, for each data part, 
 *  the input part size is equal to the output part size. 
 *  In particular, the encrypted data length MUST be equal to the data length.
 *  2) The parameter nBlockLen is the data block length. Do not confuse this length 
 *  with the block length of a block cipher algorithm like AES.
 *  For AES/DES/TripleDES algorithms, nDataLen and nBlockLen MUST be multiples 
 *  of the AES/DES block length (i.e. respectively 16 bytes and 8 bytes).
 **/
static CK_RV encryptMultiStages(IN S_HANDLE        hCryptoSession,
                                   IN CK_MECHANISM*    pMechanism,
                                   IN CK_OBJECT_HANDLE hKey,
                                   IN CK_BYTE*         pData,
                                   IN CK_ULONG         nDataLen,
                                   IN CK_ULONG         nBlockLen,
                                   OUT CK_BYTE*        pEncryptedData)
{
   CK_RV          nError;
   CK_ULONG       nBlockCount;
   CK_ULONG       nLastBlockLen;
   CK_ULONG       nOutputBlockLen;

   CK_BYTE        pDummy[1];
   CK_ULONG       nDummyLen=1;

   nError = C_EncryptInit(hCryptoSession, pMechanism, hKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_EncryptInit returned (%08x).", nError);
      return nError;
   }
   
   nOutputBlockLen = nBlockLen; /* caution: particular case */
   for(nBlockCount = 0; nBlockCount < (nDataLen/nBlockLen); nBlockCount++)
   {
      nError = C_EncryptUpdate(hCryptoSession, 
         pData, 
         nBlockLen, 
         pEncryptedData, 
         &nOutputBlockLen); 
      if (nError != CKR_OK)
      {
         _SLogError("C_EncryptUpdate returned (%08x).", nError);
         return nError;
      }
      pData += nBlockLen;
      pEncryptedData += nBlockLen;
   }
   nLastBlockLen = (nDataLen % nBlockLen);
   nOutputBlockLen = nLastBlockLen; /* caution: particular case  */
   if (nLastBlockLen !=0)
   {
      nOutputBlockLen = nLastBlockLen;
      nError = C_EncryptUpdate(hCryptoSession, 
         pData, 
         nLastBlockLen, 
         pEncryptedData, 
         &nOutputBlockLen);
      if (nError != CKR_OK)
      {
         _SLogError("C_EncryptUpdate returned (%08x).", nError);
         return nError;
      }
   }

   /* Remark:
   *  According to the Cryptographic API Specification, C_EncryptFinal (resp. C_DecryptFinal) 
   *  takes two parameters: a pointer on last encrypted part (resp. last decrypted part) and 
   *  a pointer on this part length.
   *  However, if no last part is needded, these two pointers must not be null, else 
   *  C_EncryptFinal (resp. C_DecryptFinal) returns CKR_ARGUMENTS_BAD and the operation is 
   *  not finalised. 
   *  The solution in this situation is to points to "dummy" locations, which will be ignored. 
   *  For instance, you can use the variables
   *     CK_BYTE  pDummy[1];
   *     CK_ULONG nDummyLen=1;
   *  and pass the parameters (pDummy, &nDummyLen) to C_EncryptFinal (resp. C_DecryptFinal).
   *  
   *  For now, no algorithm supported by the product needs a last part.
   */
   nError = C_EncryptFinal(hCryptoSession, pDummy, &nDummyLen);
   if (nError != CKR_OK)
   {
      _SLogError("C_EncryptFinal returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function decryptOneStage:
 *  Description:
 *           Perform a single stage decryption.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_MECHANISM* pMechanism      = points to the decryption mechanism
 *           CK_OBJECT_HANDLE   hKey       = decryption key handle
 *           CK_BYTE*      pEncryptedData     = encrypted data
 *           CK_ULONG      nEncryptedDataLen  = encrypted data length
 *  Output:  CK_BYTE*      pData           = data
 *  In/Out:  CK_ULONG*     pnDataLen       = points to data length
 *  
 *  REMARK: 
 *  1) The encrypted data length can be different from the data length.
 *  For instance, for RSA OAEP encrytion the ciphertext length is superior to 
 *  the plaintext length (since there is padding). 
 *  2) For AES/DES/TripleDES algorithms, nDataLen MUST be a multiple 
 *  of the AES/DES block length (i.e. respectively 16 bytes and 8 bytes).
 **/
static CK_RV decryptOneStage(IN S_HANDLE           hCryptoSession,
                                IN CK_MECHANISM*      pMechanism,
                                IN CK_OBJECT_HANDLE   hKey,
                                IN CK_BYTE*           pEncryptedData,
                                IN CK_ULONG           nEncryptedDataLen,
                                OUT CK_BYTE*          pData,
                                IN OUT CK_ULONG*      pnDataLen)
{
   CK_RV       nError;

   nError = C_DecryptInit(hCryptoSession, pMechanism, hKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DecryptInit returned (%08x).", nError);
      return nError;
   }
   
   nError = C_Decrypt(hCryptoSession, pEncryptedData, nEncryptedDataLen, pData, pnDataLen);
   if (nError != CKR_OK)
   {
      _SLogError("C_Decrypt returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function decryptMultiStages:
 *  Description:
 *           Perform a multi stages decryption, with updates on data blocks of size nBlockLen. 
 *           The last block length can be less or equal to nBlockLen.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_MECHANISM* pMechanism      = points to the decryption mechanism
 *           CK_OBJECT_HANDLE   hKey       = decryption key handle
 *           CK_BYTE*      pEncryptedData     = encrypted data
 *           CK_ULONG      nEncryptedDataLen  = encrypted data length
 *           CK_ULONG      nBlockLen       = data block length
 *  Output:  CK_BYTE*      pData           = data
 *
 *  PRECONDITION: 
 *  The mechanism is such that the ciphertext length is equal to the plaintext length.
 *
 *  REMARK: 
 *  1) The function decryptMultiStages() treats ONLY the case where, for each data part, 
 *  the input part size is equal to the output part size. 
 *  In particular, the encrypted data length MUST be equal to the data length.
 *  2) The parameter nBlockLen is the data block length. Do not confuse this length 
 *  with the block length of a block cipher algorithm like AES.
 *  For AES/DES/TripleDES algorithms, nDataLen and nBlockLen MUST be multiples 
 *  of the AES/DES block length (i.e. respectively 16 bytes and 8 bytes).
 */
static CK_RV decryptMultiStages(IN S_HANDLE        hCryptoSession,
                                   IN CK_MECHANISM*    pMechanism,
                                   IN CK_OBJECT_HANDLE hKey,
                                   IN CK_BYTE*         pEncryptedData,
                                   IN CK_ULONG         nEncryptedDataLen,
                                   IN CK_ULONG         nBlockLen,
                                   OUT CK_BYTE*        pData)
{
   CK_RV          nError;
   CK_ULONG       nBlockCount;
   CK_ULONG       nLastBlockLen;
   CK_ULONG       nOutputBlockLen;

   CK_BYTE        pDummy[1];
   CK_ULONG       nDummyLen=1;

   nError = C_DecryptInit(hCryptoSession, pMechanism, hKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DecryptInit returned (%08x).", nError);
      return nError;
   }
   
   nOutputBlockLen = nBlockLen; /* caution: particular case */
   for(nBlockCount = 0; nBlockCount < (nEncryptedDataLen/nBlockLen); nBlockCount++)
   {
      nError = C_DecryptUpdate(hCryptoSession, 
         pEncryptedData, 
         nBlockLen, 
         pData, 
         &nOutputBlockLen);
      if (nError != CKR_OK)
      {
         _SLogError("C_DecryptUpdate returned (%08x).", nError);
         return nError;
      }
      pData += nBlockLen;
      pEncryptedData += nBlockLen;
   }
   nLastBlockLen = (nEncryptedDataLen % nBlockLen);
   nOutputBlockLen = nLastBlockLen; /* caution: particular case  */
   if (nLastBlockLen !=0)
   {
      nOutputBlockLen = nLastBlockLen;
      nError = C_DecryptUpdate(hCryptoSession, 
         pEncryptedData, 
         nLastBlockLen, 
         pData, 
         &nOutputBlockLen);
      if (nError != CKR_OK)
      {
         _SLogError("C_DecryptUpdate returned (%08x).", nError);
         return nError;
      }
   }

   /* Remark:
   * For explanations about the parameters of C_DecryptFinal, see comments about 
   * the parameters of C_EncryptFinal in the function encryptMultiStages().
   */
   nError = C_DecryptFinal(hCryptoSession, pDummy, &nDummyLen);
   if (nError != CKR_OK)
   {
      _SLogError("C_DecryptFinal returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/* ----------------------------------------------------------------------------
 *   Signature/Verification generic functions
 * ---------------------------------------------------------------------------- */
/**
 *  Function signOneStage:
 *  Description:
 *           Perform a single stage signature.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_MECHANISM* pMechanism      = points to the signature mechanism
 *           CK_OBJECT_HANDLE  hPrivateKey = private key handle
 *           CK_BYTE*      pData           = data
 *           CK_ULONG      nDataLen        = data length
 *  Output:  CK_BYTE*      pSignature      = signature
 *  In/Out:  CK_ULONG*     pnSignatureLen  = points to the signature length
 **/
static CK_RV signOneStage(IN S_HANDLE           hCryptoSession,
                             IN CK_MECHANISM*      pMechanism,
                             IN CK_OBJECT_HANDLE   hPrivateKey,
                             IN CK_BYTE*           pData,
                             IN CK_ULONG           nDataLen,
                             OUT CK_BYTE*          pSignature,
                             IN OUT CK_ULONG*      pnSignatureLen)
{
   CK_RV       nError;

   nError = C_SignInit(hCryptoSession, pMechanism, hPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_SignInit returned (%08x).", nError);
      return nError;
   }
   
   nError = C_Sign(hCryptoSession, pData, nDataLen, pSignature, pnSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("C_Sign returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function signMultiStages:
 *  Description:
 *           Perform a multi stages signature.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_MECHANISM* pMechanism      = points to the signature mechanism
 *           CK_OBJECT_HANDLE  hPrivateKey = private key handle
 *           CK_BYTE*      pData           = data
 *           CK_ULONG      nDataLen        = data length
 *           CK_ULONG      nBlockLen       = data block length
 *  Output:  CK_BYTE*      pSignature      = signature
 *  In/Out:  CK_ULONG*     pnSignatureLen  = points to the signature length
 **/
static CK_RV signMultiStages(IN S_HANDLE           hCryptoSession,
                                IN CK_MECHANISM*      pMechanism,
                                IN CK_OBJECT_HANDLE   hPrivateKey,
                                IN CK_BYTE*           pData,
                                IN CK_ULONG           nDataLen,
                                IN CK_ULONG           nBlockLen,
                                OUT CK_BYTE*          pSignature,
                                IN OUT CK_ULONG*      pnSignatureLen)
{
   CK_RV          nError;
   CK_ULONG       nBlockCount;
   CK_ULONG       nLastBlockLen;

   nError = C_SignInit(hCryptoSession, pMechanism, hPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_SignInit returned (%08x).", nError);
      return nError;
   }
   
   for(nBlockCount = 0; nBlockCount < (nDataLen/nBlockLen); nBlockCount++)
   {
      nError = C_SignUpdate(hCryptoSession, pData, nBlockLen);
      if (nError != CKR_OK)
      {
         _SLogError("C_SignUpdate returned (%08x).", nError);
         return nError;
      }
      pData += nBlockLen;
   }
   nLastBlockLen = (nDataLen % nBlockLen);
   if (nLastBlockLen !=0)
   {
      nError = C_SignUpdate(hCryptoSession, pData, nLastBlockLen);
      if (nError != CKR_OK)
      {
         _SLogError("C_SignUpdate returned (%08x).", nError);
         return nError;
      }
   }


   nError = C_SignFinal(hCryptoSession, pSignature, pnSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("C_SignFinal returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function verifyOneStage:
 *  Description:
 *           Perform a single stage verification.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_MECHANISM* pMechanism      = points to the verification mechanism
 *           CK_OBJECT_HANDLE  hPublicKey  = public key handle
 *           CK_BYTE*      pData           = data
 *           CK_ULONG      nDataLen        = data length
 *           CK_BYTE*      pSignature      = signature
 *           CK_ULONG      nSignatureLen   = signature length
 **/
static CK_RV verifyOneStage(IN S_HANDLE         hCryptoSession,
                               IN CK_MECHANISM*    pMechanism,
                               IN CK_OBJECT_HANDLE hPublicKey,
                               IN CK_BYTE*         pData,
                               IN CK_ULONG         nDataLen,
                               IN CK_BYTE*         pSignature,
                               IN CK_ULONG         nSignatureLen)
{
   CK_RV       nError;

   nError = C_VerifyInit(hCryptoSession, pMechanism, hPublicKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_VerifyInit returned (%08x).", nError);
      return nError;
   }
   
   nError = C_Verify(hCryptoSession, pData, nDataLen, pSignature, nSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("C_Verify returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function verifyMultiStages:
 *  Description:
 *           Perform a multi stages verification.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_MECHANISM* pMechanism      = points to the verification mechanism
 *           CK_OBJECT_HANDLE  hPublicKey  = public key handle
 *           CK_BYTE*      pData           = data
 *           CK_ULONG      nDataLen        = data length
 *           CK_ULONG      nBlockLen       = data block length
 *           CK_BYTE*      pSignature      = signature
 *           CK_ULONG      nSignatureLen   = signature length
 **/
static CK_RV verifyMultiStages(IN S_HANDLE         hCryptoSession,
                                  IN CK_MECHANISM*    pMechanism,
                                  IN CK_OBJECT_HANDLE hPublicKey,
                                  IN CK_BYTE*         pData,
                                  IN CK_ULONG         nDataLen,
                                  IN CK_ULONG         nBlockLen,
                                  IN CK_BYTE*         pSignature,
                                  IN CK_ULONG         nSignatureLen)
{
   CK_RV          nError;
   CK_ULONG       nBlockCount;
   CK_ULONG       nLastBlockLen;

   nError = C_VerifyInit(hCryptoSession, pMechanism, hPublicKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_VerifInit returned (%08x).", nError);
      return nError;
   }
   
   for(nBlockCount = 0; nBlockCount < (nDataLen/nBlockLen); nBlockCount++)
   {
      nError = C_VerifyUpdate(hCryptoSession, pData, nBlockLen);
      if (nError != CKR_OK)
      {
         _SLogError("C_VerifyUpdate returned (%08x).", nError);
         return nError;
      }
      pData += nBlockLen;
   }
   nLastBlockLen = (nDataLen % nBlockLen);
   if (nLastBlockLen !=0)
   {
      nError = C_VerifyUpdate(hCryptoSession, pData, nLastBlockLen);
      if (nError != CKR_OK)
      {
         _SLogError("C_VerifyUpdate returned (%08x).", nError);
         return nError;
      }
   }

   nError = C_VerifyFinal(hCryptoSession, pSignature, nSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("C_VerifyFinal returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/* ----------------------------------------------------------------------------
 *   CKO_DATA Objects
 * ---------------------------------------------------------------------------- */
/**
 *  Function createAndRetrieveCkoDataWithCheck:
 *  Description:
 *           Create a CKO_DATA object from data, retrieve the object value,
 *           then compare whith data value.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_BYTE*      pData           = data
 *           CK_ULONG      nDataLen        = data length
 **/
static CK_RV createAndRetrieveCkoDataWithCheck(IN S_HANDLE      hCryptoSession,
                                        IN const CK_BYTE*      pData,
                                        IN const CK_ULONG      nDataLen)
{
   CK_RV          nError;
   static const CK_OBJECT_CLASS   nDataObjectClass  = CKO_DATA;
   static const CK_BYTE           pObjectID[2]      = {0x00, 0x01};
   CK_OBJECT_HANDLE  hObject = CK_INVALID_HANDLE;

   CK_ATTRIBUTE objTemplate[] =
   {
       {CKA_CLASS,             (CK_VOID_PTR)&nDataObjectClass,		sizeof(nDataObjectClass) },
       {CKA_ID,                (CK_VOID_PTR)pObjectID,				2}, 
       {CKA_TOKEN,             (CK_VOID_PTR)&test_true,             sizeof(test_true) }, 
       {CKA_VALUE,             NULL,                  0},
   };

   CK_ATTRIBUTE objSearchTemplate[1] =
   {
       {CKA_VALUE,     NULL_PTR,  0}
   };
   uint8_t* pDataCheck; 
   uint32_t nDataLenCheck;


   /* Set objTemplate data to parameter value */
   objTemplate[3].pValue = (CK_VOID_PTR)pData;
   objTemplate[3].ulValueLen = nDataLen;

   /* Create a cko_data object */
   _SLogTrace(" -  Create a CKO_DATA object from data (%d bytes)", nDataLen);
   nError = C_CreateObject(
      hCryptoSession,
      objTemplate,
      ATTRIBUTES_SIZE(objTemplate),
      &hObject);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      return nError;
   }


   /* Get the size of the object stored. */
   _SLogTrace(" -  Retrieve the object value");
   nError = C_GetAttributeValue(hCryptoSession, hObject, objSearchTemplate, 1);
   if (nError != CKR_OK)
   {
       _SLogError("C_GetAttributeValue returned (%08x).", nError);
       C_DestroyObject(hCryptoSession, hObject);
       return nError;
   }

   nDataLenCheck = objSearchTemplate[0].ulValueLen;
   pDataCheck = SMemAlloc(objSearchTemplate[0].ulValueLen);
   if (pDataCheck == NULL)
   {
       _SLogError("SMemAlloc failed (%08x).\n", nError);
       C_DestroyObject(hCryptoSession, hObject);
       return CKR_DEVICE_MEMORY;
   }

   /* Get the object stored. */
   objSearchTemplate[0].pValue = pDataCheck;
   nError = C_GetAttributeValue(hCryptoSession, hObject, objSearchTemplate, 1);
   if (nError != CKR_OK)
   {
      _SLogError("C_GetAttributeValue returned (%08x).", nError);
      SMemFree(pDataCheck);
      nDataLenCheck = 0;
      pDataCheck = NULL;
      C_DestroyObject(hCryptoSession, hObject);
      return nError;
   }

   /* Check data match */
   _SLogTrace(" -  Compare with the data value");
   if ((SMemCompare(pDataCheck, pData, nDataLenCheck)!=0)
       || (nDataLenCheck!= nDataLen))
   {
       _SLogError("createAndRetrieveCkoDataWithCheck(): data does not match.");
       SMemFree(pDataCheck);
       nDataLenCheck = 0;
       pDataCheck = NULL;
       C_DestroyObject(hCryptoSession, hObject);
       return CKR_VENDOR_DEFINED;
   }

   /* cleanup */
   SMemFree(pDataCheck);
   nDataLenCheck = 0;
   pDataCheck = NULL;

    _SLogTrace(" -  Destroy the object");
   nError = C_DestroyObject(hCryptoSession, hObject);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject returned (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function exampleCkoDataObject:
 *  Description:
 *           Call createAndRetrieveCkoDataWithCheck on a 128 bytes data.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 **/
static CK_RV exampleCkoDataObject(IN S_HANDLE		hCryptoSession)
{
   CK_RV       nError = CKR_OK;

   nError = createAndRetrieveCkoDataWithCheck(
       hCryptoSession, 
       (CK_BYTE*)test_message_128_bytes /* pData */, 
       sizeof(test_message_128_bytes)  /* nDataLen */);
   if (nError != CKR_OK)
   {
      _SLogError("createAndRetrieveCkoDataWithCheck() failed (%08x).", nError);
   }

   return nError;
}

/* ----------------------------------------------------------------------------
 *   Digest functions specific to some mechanisms and data
 * ---------------------------------------------------------------------------- */
/**
 *  Function digestSha1WithCheck:
 *  Description:
 *           Perform a digest SHA1 on data and check the result.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV digestSha1WithCheck(IN S_HANDLE           hCryptoSession,
                                    IN bool               bIsMultiStage)
{
   CK_RV       nError;

   /*
   *  Here the digest uses the SHA1 algorithm.
   *  If you need to use MD5, SHA224, SHA256, SHA384, or SHA512 algorithm, 
   *  just replace the mechanism {CKM_SHA_1, NULL_PTR, 0} 
   *  and the digest pDigest[20] respectively by:
   *     {CKM_MD5,    NULL_PTR, 0}  and   pDigest[16]
   *     {CKM_SHA224, NULL_PTR, 0}  and   pDigest[28]
   *     {CKM_SHA256, NULL_PTR, 0}  and   pDigest[32]
   *     {CKM_SHA384, NULL_PTR, 0}  and   pDigest[48]
   *     {CKM_SHA512, NULL_PTR, 0}  and   pDigest[64]
   */
   CK_MECHANISM mechanism = {CKM_SHA_1, NULL_PTR, 0}; 

   CK_BYTE  pDigest[20]; 
   CK_ULONG nDigestLen = sizeof(pDigest);

   CK_ULONG      nBlockLen = 8; /* any value is possible */

   /* perform a digest of a message */
   if(bIsMultiStage == false)
   {
      nError = digestOneStage(
         hCryptoSession, 
         &mechanism, 
         (CK_BYTE*)test_sha1_msg, /* in this example the input is a string */
         test_sha1_msg_len, /* sizeof(test_sha1_msg)-1, we remove the ending character '\0' */
         pDigest,
         &nDigestLen);
      if (nError != CKR_OK)
      {
         _SLogError("digestOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = digestMultiStages(
         hCryptoSession,
         &mechanism, 
         (CK_BYTE*)test_sha1_msg, /* .. */
         test_sha1_msg_len, 
         nBlockLen,
         pDigest,
         &nDigestLen);
      if (nError != CKR_OK)
      {
         _SLogError("digestMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check digest match */
   if ((SMemCompare(pDigest, test_sha1_msg_digest, nDigestLen)!=0)
      || (nDigestLen!= sizeof(test_sha1_msg_digest)))
   {
      _SLogError("digestSha1WithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *
 *  Function exampleDigestMechanism:
 *  Description:
 *           Perform some digests (and check the results).
 *           See comments inside the present function for more details.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *
 **/
static CK_RV exampleDigestMechanism(IN S_HANDLE           hCryptoSession)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   _SLogTrace(" -  SHA1 digest, one stage (then comparison)");
   nError = digestSha1WithCheck(hCryptoSession, false);
   if (nError != CKR_OK)
   {
      _SLogError("digestSha1WithCheck() failed (%08x).", nError);
      nFirstError = nError;
   }

   _SLogTrace(" -  SHA1 digest, multi stages (then comparison)");
   nError = digestSha1WithCheck(hCryptoSession, true);
   if (nError != CKR_OK)
   {
      _SLogError("digestSha1WithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}
/* ----------------------------------------------------------------------------
*   Signature/Verification functions specific to some mechanisms and data
* ---------------------------------------------------------------------------- */
/** 
*  RSA PKCS mechanism: 
*  ------------------
*  The RSA PKCS supports single-part/multi-parts signature and verification.
**/

/**
 *  Function signRsaPkcsSha1WithCheck:
 *  Description:
 *           Perform a RSA PKCS SHA1 signature on data and check the result.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPrivateKey  = private key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV signRsaPkcsSha1WithCheck(IN S_HANDLE           hCryptoSession,
                                         IN CK_OBJECT_HANDLE   hPrivateKey,
                                         IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_SHA1_RSA_PKCS, NULL_PTR, 0}; 

   CK_BYTE  pSignature[128]; /* 128 bytes corresponds to a 1024 bits RSA key */
   CK_ULONG nSignatureLen = sizeof(pSignature);

   CK_ULONG      nBlockLen = 16; /* any value is possible */

   /* sign a content with RSA PKCS sha1 */
   if(bIsMultiStage == false)
   {
      nError = signOneStage(
         hCryptoSession, 
         &mechanism, 
         hPrivateKey, 
         (CK_BYTE*)test_rsa_pkcs_msg,
         sizeof(test_rsa_pkcs_msg),
         pSignature,
         &nSignatureLen);
      if (nError != CKR_OK)
      {
         _SLogError("signOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = signMultiStages(
         hCryptoSession,
         &mechanism, 
         hPrivateKey, 
         (CK_BYTE*)test_rsa_pkcs_msg,
         sizeof(test_rsa_pkcs_msg),
         nBlockLen,
         pSignature,
         &nSignatureLen);
      if (nError != CKR_OK)
      {
         _SLogError("signMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check signature match */
   if ((SMemCompare(pSignature, test_rsa_pkcs_sha1_sig, nSignatureLen)!=0)
      || (nSignatureLen!= sizeof(test_rsa_pkcs_sha1_sig)))
   {
      _SLogError("signRsaPkcsWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}


/**
 *  Function verifyRsaPkcsSha1:
 *  Description:
 *           Perform a RSA PKCS SHA1 verification on data.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPublicKey   = public key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV verifyRsaPkcsSha1(IN S_HANDLE           hCryptoSession,
                                  IN CK_OBJECT_HANDLE   hPublicKey,
                                  IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_SHA1_RSA_PKCS, NULL_PTR, 0}; 

   CK_ULONG      nBlockLen = 16; /* any value is possible */

   /* verify a RSA PKCS sha1 signature */
   if(bIsMultiStage == false)
   {
      nError = verifyOneStage(
         hCryptoSession, 
         &mechanism, 
         hPublicKey, 
         (CK_BYTE*)test_rsa_pkcs_msg,
         sizeof(test_rsa_pkcs_msg),
         (CK_BYTE*)test_rsa_pkcs_sha1_sig, 
         sizeof(test_rsa_pkcs_sha1_sig));
      if (nError != CKR_OK)
      {
         _SLogError("verifyOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = verifyMultiStages(
         hCryptoSession, 
         &mechanism, 
         hPublicKey, 
         (CK_BYTE*)test_rsa_pkcs_msg,
         sizeof(test_rsa_pkcs_msg),
         nBlockLen,
         (CK_BYTE*)test_rsa_pkcs_sha1_sig,
         sizeof(test_rsa_pkcs_sha1_sig));
      if (nError != CKR_OK)
      {
         _SLogError("verifyMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   return CKR_OK;
}

/**
 *  Function exampleRsaPkcsMechanismSignVerify:
 *  Descryption:
 *           Create a RSA key pair, then perform some RSA PKCS signatures and verifications
 *           on data (and check the results), then destroy the key pair.
 *           See comments inside the present function for more details.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleRsaPkcsMechanismSignVerify(IN  S_HANDLE  hCryptoSession,
                                                  IN CK_BBOOL   bIsToken)
{
   CK_RV       nError = CKR_OK;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE       privateKeyRsaId[2]    ={0x00, 0x01}; 
   CK_BYTE       publicKeyRsaId[2]     ={0x00, 0x02};
   CK_OBJECT_HANDLE hPrivateKeyRsa     = CK_INVALID_HANDLE; 
   CK_OBJECT_HANDLE hPublicKeyRsa      = CK_INVALID_HANDLE;

   /* ------------   RSA Keys   -------------------- */
   _SLogTrace(" -  Import a RSA key pair");
   nError = createRsaKeyPair(hCryptoSession,
                             privateKeyRsaId,
                             publicKeyRsaId,
                             bIsToken,
                             &hPrivateKeyRsa,
                             &hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("createRsaKeyPair() failed (%08x).", nError);
      nFirstError = nError;
   }

   /* ----------  RSA PKCS  sign/verify ----------- */
   _SLogTrace(" -  RSA PKCS SHA1 signature, one stage (then comparison)");
   nError = signRsaPkcsSha1WithCheck(hCryptoSession, hPrivateKeyRsa, false);
   if (nError != CKR_OK)
   {
      _SLogError("signRsaPkcsWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  RSA PKCS SHA1 signature, multi stages (then comparison)");
   nError = signRsaPkcsSha1WithCheck(hCryptoSession, hPrivateKeyRsa, true);
   if (nError != CKR_OK)
   {
      _SLogError("signRsaPkcsWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  RSA PKCS SHA1 verification, one stage");
   nError = verifyRsaPkcsSha1(hCryptoSession, hPublicKeyRsa, false);
   if (nError != CKR_OK)
   {
      _SLogError("verifyRsaPkcsSha1() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  RSA PKCS SHA1 verification, multi stages");
   nError = verifyRsaPkcsSha1(hCryptoSession, hPublicKeyRsa, true);
   if (nError != CKR_OK)
   {
      _SLogError("verifyRsaPkcsSha1() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ------------  Destroy Keys   ----------------- */
   _SLogTrace(" -  Destroy RSA key pair");
   nError = C_DestroyObject(hCryptoSession, hPrivateKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/**
 *  RSA PSS mechanism:  
 *  ------------------
 *  The RSA PSS with digest mechanism (for instance CKM_SHA1_RSA_PKCS_PSS) is a mechanism 
 *  for single- and multiple-part signatures and verification.
 *
 *  The RSA PSS without hashing mechanism, denoted CKM_RSA_PKCS_PSS, is a mechanism 
 *  for single-part signatures and verification.
 *
 *  Remark: The RSA PSS signature is computed using some random, 
 *  so we cannot compare the signature with an expected one.
 **/

/**
 *  Function signRsaPssSha1ThenVerify:
 *  Description:
 *           Perform a RSA PSS SHA1 signature on data, then verify the signature.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPrivateKey  = private key handle
 *           CK_OBJECT_HANDLE hPublicKey   = public key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV signRsaPssSha1ThenVerify(IN S_HANDLE           hCryptoSession,
                                         IN CK_OBJECT_HANDLE   hPrivateKey,  
                                         IN CK_OBJECT_HANDLE   hPublicKey,  
                                         IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_RSA_PKCS_PSS_PARAMS params = {CKM_SHA_1, CKG_MGF1_SHA1, 20};
   CK_MECHANISM mechanism = {CKM_SHA1_RSA_PKCS_PSS, NULL, sizeof(params)};

   CK_BYTE  pSignature[128]; /* 128 bytes corresponds to a 1024 bits RSA key */
   CK_ULONG nSignatureLen = sizeof(pSignature);

   CK_ULONG      nBlockLen = 16; /* any value is possible */

   mechanism.pParameter = &params;
   
   /* sign a content with RSA PKCS sha1 */
   if(bIsMultiStage == false)
   {
      nError = signOneStage(
         hCryptoSession, 
         &mechanism, 
         hPrivateKey, 
         (CK_BYTE*)test_rsa_pss_msg,
         sizeof(test_rsa_pss_msg),
         pSignature,
         &nSignatureLen);
      if (nError != CKR_OK)
      {
         _SLogError("signOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = signMultiStages(
         hCryptoSession,
         &mechanism, 
         hPrivateKey, 
         (CK_BYTE*)test_rsa_pss_msg,
         sizeof(test_rsa_pss_msg),
         nBlockLen,
         pSignature,
         &nSignatureLen);
      if (nError != CKR_OK)
      {
         _SLogError("signMultiStages() failed (%08x).", nError);
         return nError;
      }
   }

   /* verify the signature */
   nError = verifyOneStage(
      hCryptoSession, 
      &mechanism,
      hPublicKey, 
      (CK_BYTE*)test_rsa_pss_msg,
      sizeof(test_rsa_pss_msg),
      pSignature,
      nSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("verifyOneStage() failed (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function verifyRsaPssSha1:
 *  Description:
 *           Perform a RSA PSS SHA1 verification on data.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPublicKey   = public key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV verifyRsaPssSha1(IN S_HANDLE           hCryptoSession,
                                 IN CK_OBJECT_HANDLE   hPublicKey, 
                                 IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_RSA_PKCS_PSS_PARAMS params = {CKM_SHA_1, CKG_MGF1_SHA1, 20};
   CK_MECHANISM mechanism = {CKM_SHA1_RSA_PKCS_PSS, NULL, sizeof(params)};

   CK_ULONG      nBlockLen = 16; /* any value is possible */
   
   mechanism.pParameter = &params;

   /* verify a RSA PSS SHA1 signature*/
   if(bIsMultiStage == false)
   {
      nError = verifyOneStage(
         hCryptoSession, 
         &mechanism, 
         hPublicKey, 
         (CK_BYTE*)test_rsa_pss_msg,
         sizeof(test_rsa_pss_msg),
         (CK_BYTE*)test_rsa_pss_sha1_sig, /* one of the possible signatures */
         sizeof(test_rsa_pss_sha1_sig));  
      if (nError != CKR_OK)
      {
         _SLogError("verifyOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = verifyMultiStages(
         hCryptoSession, 
         &mechanism, 
         hPublicKey, 
         (CK_BYTE*)test_rsa_pss_msg,
         sizeof(test_rsa_pss_msg),
         nBlockLen,
         (CK_BYTE*)test_rsa_pss_sha1_sig, /* one of the possible signatures */
         sizeof(test_rsa_pss_sha1_sig));
      if (nError != CKR_OK)
      {
         _SLogError("verifyMultiStages() failed (%08x).", nError);
         return nError;
      }
   }

   return CKR_OK;
}

/**
 *  Function digestSha1SignRsaPssThenVerify:
 *  Description:
 *           Perform a SHA1 digest on data, then a RSA PSS signature.
 *           Two verifications are performed :
 *           - a RSA PSS SHA1 verification from the initial message;
 *           - a RSA PSS verification directly from the digest.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPrivateKey  = private key handle
 *           CK_OBJECT_HANDLE hPublicKey   = public key handle
 **/
static CK_RV digestSha1SignRsaPssThenVerify(IN S_HANDLE           hCryptoSession,
                                               IN CK_OBJECT_HANDLE   hPrivateKey, 
                                               IN CK_OBJECT_HANDLE   hPublicKey) 
{
   CK_RV       nError;

   CK_MECHANISM digestMechanism = {CKM_SHA_1, NULL_PTR, 0}; 
   CK_RSA_PKCS_PSS_PARAMS params = {CKM_SHA_1, CKG_MGF1_SHA1, 20};
   CK_MECHANISM mechanism = {CKM_RSA_PKCS_PSS, NULL, sizeof(params)};

   CK_MECHANISM verifyMechanism  = {CKM_SHA1_RSA_PKCS_PSS, NULL, sizeof(params)};

   CK_BYTE  pDigest[20]; /* 20 bytes corresponds to a SHA1 digest  */
   CK_ULONG nDigestLen = sizeof(pDigest);

   CK_BYTE  pSignature[128]; /* 128 bytes corresponds to a 1024 bits RSA key */
   CK_ULONG nSignatureLen = sizeof(pSignature);
   
   mechanism.pParameter = &params;
   verifyMechanism.pParameter = &params;  

   /* digest: could be single part or multi part */
   nError = digestOneStage(
      hCryptoSession, 
      &digestMechanism, 
      (CK_BYTE*)test_rsa_pss_msg,
      sizeof(test_rsa_pss_msg),
      pDigest,
      &nDigestLen);
   if (nError != CKR_OK)
   {
      _SLogError("digestOneStage() failed (%08x).", nError);
      return nError;
   }

   /* sign: single part only */
   nError = signOneStage(
      hCryptoSession, 
      &mechanism, 
      hPrivateKey, 
      (CK_BYTE*)pDigest,
      nDigestLen,
      pSignature,
      &nSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("signOneStage() failed (%08x).", nError);
      return nError;
   }
   /* Remark: the PSS signature is computed using some random, 
    * so we cannot compare the signature with an expected one */

   /* Check(1): verify the signature from the message */
   /* could be single part or multi part */
   nError = verifyOneStage(
      hCryptoSession, 
      &verifyMechanism,
      hPublicKey, 
      (CK_BYTE*)test_rsa_pss_msg,
      sizeof(test_rsa_pss_msg),
      pSignature,
      nSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("verifyOneStage() failed (%08x).", nError);
      return nError;
   }

   /* Check(2): verify the signature directly from the digest */
   /* single part only */
   nError = verifyOneStage(
      hCryptoSession, 
      &mechanism,
      hPublicKey, 
      (CK_BYTE*)pDigest,
      nDigestLen,
      pSignature,
      nSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("verifyOneStage() failed (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function exampleRsaPssMechanism:
 *  Descryption:
 *           Create a RSA key pair, then perform some RSA PSS signatures and verifications
 *           on data (and check the results), then destroy the key pair.
 *           See comments inside the present function for more details.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleRsaPssMechanism(IN  S_HANDLE  hCryptoSession, IN CK_BBOOL   bIsToken)
{
   CK_RV      nError;
   CK_RV      nFirstError = CKR_OK;

   CK_BYTE       privateKeyRsaId[2]    = {0x00, 0x01}; 
   CK_BYTE       publicKeyRsaId[2]     = {0x00, 0x02};
   CK_OBJECT_HANDLE hPrivateKeyRsa     = CK_INVALID_HANDLE; 
   CK_OBJECT_HANDLE hPublicKeyRsa      = CK_INVALID_HANDLE;

   /* ------------   RSA Keys   -------------------- */
   _SLogTrace(" -  Import a RSA key pair");
   nError = createRsaPssKeyPair(hCryptoSession,
                             privateKeyRsaId,
                             publicKeyRsaId,
                             bIsToken,
                             &hPrivateKeyRsa,
                             &hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("createRsaKeyPair() failed (%08x).", nError);
      nFirstError = nError;
   }

   /* ------------   RSA PSS   -------------------- */
   _SLogTrace(" -  RSA PSS SHA1 signature, one stage (then verification)");
   nError = signRsaPssSha1ThenVerify(hCryptoSession, hPrivateKeyRsa, hPublicKeyRsa, false);
   if (nError != CKR_OK)
   {
      _SLogError("signRsaPssSha1ThenVerify() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  RSA PSS SHA1 signature, multi stages (then verification)");
   nError = signRsaPssSha1ThenVerify(hCryptoSession, hPrivateKeyRsa, hPublicKeyRsa, true);
   if (nError != CKR_OK)
   {
      _SLogError("signRsaPssSha1ThenVerify() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }


   _SLogTrace(" -  RSA PSS SHA1 verification, one stage");
   nError = verifyRsaPssSha1(hCryptoSession, hPublicKeyRsa, false);
   if (nError != CKR_OK)
   {
      _SLogError("verifyRsaPssSha1() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  RSA PSS SHA1 verification, multi stages");
   nError = verifyRsaPssSha1(hCryptoSession, hPublicKeyRsa, true);
   if (nError != CKR_OK)
   {
      _SLogError("verifyRsaPssSha1() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }


   /* perform a SHA1 digest followed by a RSA PSS signature, one stage, then verify the signature */
   _SLogTrace(" -  SHA1 digest + RSA PSS signature, one stage (then verification, two different ways)");
   nError = digestSha1SignRsaPssThenVerify(hCryptoSession, hPrivateKeyRsa, hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("digestSha1SignRsaPssThenVerify() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ------------  Destroy Keys   ----------------- */
   _SLogTrace(" -  Destroy RSA key pair");
   nError = C_DestroyObject(hCryptoSession, hPrivateKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/**
 *  DSA mechanism:
 *  --------------
 *  The DSA with SHA-1 mechanism, denoted CKM_DSA_SHA1, is a mechanism 
 *  for single- and multiple-part signatures and verification.
 *
 *  The DSA without hashing mechanism, denoted CKM_DSA, is a mechanism 
 *  for single-part signatures and verification.
 *
 *  Remark: The DSA signature is computed using some random, 
 *  so we cannot compare the signature with an expected one.
 **/

/**
 *  Function signDsaSha1ThenVerify:
 *  Description:
 *           Perform a DSA SHA1 signature on data, then verify the signature.
 *           The key size is 512 bits in this example.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPrivateKey  = private key handle
 *           CK_OBJECT_HANDLE hPublicKey   = public key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV signDsaSha1ThenVerify(IN S_HANDLE           hCryptoSession,
                                         IN CK_OBJECT_HANDLE   hPrivateKey,  
                                         IN CK_OBJECT_HANDLE   hPublicKey,  
                                         IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DSA_SHA1, NULL_PTR, 0};

   CK_BYTE  pSignature[40]; /* 40 bytes  for DSA signature */
   CK_ULONG nSignatureLen = sizeof(pSignature);

   CK_ULONG      nBlockLen = 16; /* any value is possible */

   /* sign a content with DSA SHA1 */
   if(bIsMultiStage == false)
   {
      nError = signOneStage(
         hCryptoSession, 
         &mechanism, 
         hPrivateKey, 
         (CK_BYTE*)test_dsa_512_msg,
         sizeof(test_dsa_512_msg),
         pSignature,
         &nSignatureLen);
      if (nError != CKR_OK)
      {
         _SLogError("signOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = signMultiStages(
         hCryptoSession,
         &mechanism, 
         hPrivateKey, 
         (CK_BYTE*)test_dsa_512_msg,
         sizeof(test_dsa_512_msg),
         nBlockLen,
         pSignature,
         &nSignatureLen);
      if (nError != CKR_OK)
      {
         _SLogError("signMultiStages() failed (%08x).", nError);
         return nError;
      }
   }

   /* verify the signature */
   nError = verifyOneStage(
      hCryptoSession, 
      &mechanism,
      hPublicKey, 
      (CK_BYTE*)test_dsa_512_msg,
      sizeof(test_dsa_512_msg),
      pSignature,
      nSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("verifyOneStage() failed (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function verifyDsaSha1:
 *  Description:
 *           Perform a DSA SHA1 verification on data.
 *           The key size is 512 bits in this example.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPublicKey   = public key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV verifyDsaSha1(IN S_HANDLE           hCryptoSession,
                                 IN CK_OBJECT_HANDLE   hPublicKey, 
                                 IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DSA_SHA1, NULL_PTR, 0};

   CK_ULONG      nBlockLen = 16; /* any value is possible */

   /* verify a DSA SHA1 signature*/
   if(bIsMultiStage == false)
   {
      nError = verifyOneStage(
         hCryptoSession, 
         &mechanism, 
         hPublicKey, 
         (CK_BYTE*)test_dsa_512_msg,
         sizeof(test_dsa_512_msg),
         (CK_BYTE*)test_dsa_512_sig, /* one of the possible signatures */
         sizeof(test_dsa_512_sig));  
      if (nError != CKR_OK)
      {
         _SLogError("verifyOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = verifyMultiStages(
         hCryptoSession, 
         &mechanism, 
         hPublicKey, 
         (CK_BYTE*)test_dsa_512_msg,
         sizeof(test_dsa_512_msg),
         nBlockLen,
         (CK_BYTE*)test_dsa_512_sig, /* one of the possible signatures */
         sizeof(test_dsa_512_sig));
      if (nError != CKR_OK)
      {
         _SLogError("verifyMultiStages() failed (%08x).", nError);
         return nError;
      }
   }

   return CKR_OK;
}

/**
 *  Function digestSha1SignDsaThenVerify:
 *  Description:
 *           Perform a SHA1 digest on data, then a DSA signature.
 *           The key size is 512 bits in this example.
 *           Two verifications are performed :
 *           - a DSA SHA1 verification from the initial message;
 *           - a DSA verification directly from the digest.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPrivateKey  = private key handle
 *           CK_OBJECT_HANDLE hPublicKey   = public key handle
 **/
static CK_RV digestSha1SignDsaThenVerify(IN S_HANDLE           hCryptoSession,
                                               IN CK_OBJECT_HANDLE   hPrivateKey, 
                                               IN CK_OBJECT_HANDLE   hPublicKey) 
{
   CK_RV       nError;

   CK_MECHANISM digestMechanism = {CKM_SHA_1, NULL_PTR, 0}; 
   CK_MECHANISM mechanism = {CKM_DSA, NULL_PTR, 0};

   CK_MECHANISM verifyMechanism = {CKM_DSA_SHA1, NULL_PTR, 0};

   CK_BYTE  pDigest[20]; /* 20 bytes corresponds to a SHA1 digest  */
   CK_ULONG nDigestLen = sizeof(pDigest);

   CK_BYTE  pSignature[40]; /* 40 bytes  for DSA signature */
   CK_ULONG nSignatureLen = sizeof(pSignature);

   /* digest: could be single part or multi part */
   nError = digestOneStage(
      hCryptoSession, 
      &digestMechanism, 
      (CK_BYTE*)test_dsa_512_msg,
      sizeof(test_dsa_512_msg),
      pDigest,
      &nDigestLen);
   if (nError != CKR_OK)
   {
      _SLogError("digestOneStage() failed (%08x).", nError);
      return nError;
   }

   /* sign: single part only */
   nError = signOneStage(
      hCryptoSession, 
      &mechanism, 
      hPrivateKey, 
      (CK_BYTE*)pDigest,
      nDigestLen,
      pSignature,
      &nSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("signOneStage() failed (%08x).", nError);
      return nError;
   }

   /* Check(1): verify the signature from the message */
   /* could be single part or multi part */
   nError = verifyOneStage(
      hCryptoSession, 
      &verifyMechanism,
      hPublicKey, 
      (CK_BYTE*)test_dsa_512_msg,
      sizeof(test_dsa_512_msg),
      pSignature,
      nSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("verifyOneStage() failed (%08x).", nError);
      return nError;
   }

   /* Check(2): verify the signature directly from the digest */
   /* single part only */
   nError = verifyOneStage(
      hCryptoSession, 
      &mechanism,
      hPublicKey, 
      (CK_BYTE*)pDigest,
      nDigestLen,
      pSignature,
      nSignatureLen);
   if (nError != CKR_OK)
   {
      _SLogError("verifyOneStage() failed (%08x).", nError);
      return nError;
   }

   return CKR_OK;
}

/**
 *  Function exampleDsaMechanism:
 *  Descryption:
 *           Create a DSA key pair, then perform some DSA signatures and verifications
 *           on data (and check the results), then destroy the key pair.
 *           See comments inside the present function for more details.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleDsaMechanism(IN  S_HANDLE  hCryptoSession, IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE       privateKeyDsaId[2]    ={0x00, 0x01}; 
   CK_BYTE       publicKeyDsaId[2]     ={0x00, 0x02};
   CK_OBJECT_HANDLE hPrivateKeyDsa     = CK_INVALID_HANDLE; 
   CK_OBJECT_HANDLE hPublicKeyDsa      = CK_INVALID_HANDLE;

  /* ------------   DSA Keys   -------------------- */
   _SLogTrace(" -  Import a DSA key pair");
   nError = createDsaKeyPair(hCryptoSession,
                             privateKeyDsaId,
                             publicKeyDsaId,
                             bIsToken,
                             &hPrivateKeyDsa,
                             &hPublicKeyDsa);
   if (nError != CKR_OK)
   {
      _SLogError("createDsaKeyPair() failed (%08x).", nError);
      nFirstError = nError;
   }

   /* -------------  DSA  ------------------------- */
   _SLogTrace(" -  DSA SHA1 signature, one stage (then verification)");
   nError = signDsaSha1ThenVerify(hCryptoSession, hPrivateKeyDsa, hPublicKeyDsa, false);
   if (nError != CKR_OK)
   {
      _SLogError("signDsaSha1ThenVerify() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  DSA SHA1 signature, multi stages (then verification)");
   nError = signDsaSha1ThenVerify(hCryptoSession, hPrivateKeyDsa, hPublicKeyDsa, true);
   if (nError != CKR_OK)
   {
      _SLogError("signDsaSha1ThenVerify() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  DSA SHA1 verification, one stage");
   nError = verifyDsaSha1(hCryptoSession, hPublicKeyDsa, false);
   if (nError != CKR_OK)
   {
      _SLogError("verifyDsaSha1() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  DSA SHA1 verification, multi stages");
   nError = verifyDsaSha1(hCryptoSession, hPublicKeyDsa, true);
   if (nError != CKR_OK)
   {
      _SLogError("verifyDsaSha1() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* perform a SHA1 digest followed by a Dsa signature, one stage, then verify the signature */
   _SLogTrace(" -  SHA1 digest + DSA signature, one stage (then verification, two different ways)");
   nError = digestSha1SignDsaThenVerify(hCryptoSession, hPrivateKeyDsa, hPublicKeyDsa);
   if (nError != CKR_OK)
   {
      _SLogError("digestSha1SignDsaThenVerify() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ------------  Destroy Keys   ----------------- */
   _SLogTrace(" -  Destroy DSA key pair");
   nError = C_DestroyObject(hCryptoSession, hPrivateKeyDsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hPublicKeyDsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/**
 *  Function exampleDsaKeyGeneration:
 *  Description:
 *           Generate a DSA key pair, then perform a DSA signature and verification
 *           on data (and check the results), then destroy the key pair.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleDsaKeyGeneration(IN  S_HANDLE  hCryptoSession,IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE       privateKeyDsaId[2]    ={0x00, 0x01}; 
   CK_BYTE       publicKeyDsaId[2]     ={0x00, 0x02};
   CK_OBJECT_HANDLE hPrivateKeyDsa     = CK_INVALID_HANDLE; 
   CK_OBJECT_HANDLE hPublicKeyDsa      = CK_INVALID_HANDLE;

   /* generate a DSA key pair */
   _SLogTrace(" -  Generate a DSA key pair");
   nError = generateDsaKeyPair(hCryptoSession,
      privateKeyDsaId,
      publicKeyDsaId,
      bIsToken,
      &hPrivateKeyDsa,
      &hPublicKeyDsa);
   if (nError != CKR_OK)
   {
      _SLogError("generateRsaKeyPair() failed (%08x).", nError);
      nFirstError = nError;
   }

   /* sign and verify */
   _SLogTrace(" -  DSA SHA1 signature, multi stages (then verification)");
   nError = signDsaSha1ThenVerify(hCryptoSession, hPrivateKeyDsa, hPublicKeyDsa, true);
   if (nError != CKR_OK)
   {
      _SLogError("signDsaSha1ThenVerify() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* destroy the DSA key pair */
   _SLogTrace(" -  Destroy DSA key pair");
   nError = C_DestroyObject(hCryptoSession, hPrivateKeyDsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hPublicKeyDsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/* ----------------------------------------------------------------------------
 *   Encryption/Decryption functions specific to some mechanisms and data
 * ---------------------------------------------------------------------------- */
/**
 *  RSA PKCS mechanism:
 *  -------------------
 *  The RSA PKCS supports single-part encryption and decryption.
 *
 *  Let k denote the modulus length. 
 *  Then, we must have:
 *  Function  Input length   Output length 
 *  C_Encrypt  <= k-11       =k
 *  C_Decrypt  =k            <=k-11
 *
 *  Remark: the RSA PKCS cipher text is computed" using some random, 
 *  so we cannot compare the cipher text with an expected one 
 *
 **/

/**
 *  Function encryptRsaPkcsThenDecrypt:
 *  Description:
 *           Perform a RSA PKCS encryption on data, then decrypt the encrypted data
 *           and compare with the initial data.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPublicKey   = public key handle
 *           CK_OBJECT_HANDLE hPrivateKey  = private key handle
 **/
static CK_RV encryptRsaPkcsThenDecrypt(IN S_HANDLE           hCryptoSession,
                                          IN CK_OBJECT_HANDLE   hPublicKey,
                                          IN CK_OBJECT_HANDLE   hPrivateKey)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_RSA_PKCS, NULL_PTR, 0}; 

   CK_BYTE  pCipher[128]; /* 128 bytes corresponds to a 1024 bits RSA key */
   CK_ULONG nCipherLen = sizeof(pCipher);

   CK_BYTE  pData[128-11]; /* length <= k-11 (here the modulus length k is 128 bytes) */
   CK_ULONG nDataLen = sizeof(pData);

   /* encrypt a content with RSA PKCS */
   nError = encryptOneStage(
      hCryptoSession, 
      &mechanism, 
      hPublicKey, 
      (CK_BYTE*)test_rsa_pkcs_msg,
      sizeof(test_rsa_pkcs_msg), /* length <= k-11 (i.e.128-11) */
      pCipher,
      &nCipherLen);
   if (nError != CKR_OK)
   {
      _SLogError("encryptOneStage() failed (%08x).", nError);
      return nError;
   }
   
   /* decrypt a RSA PKCS cipher text */
   nError = decryptOneStage(
      hCryptoSession, 
      &mechanism, 
      hPrivateKey, 
      (CK_BYTE*)pCipher,
      nCipherLen,
      (CK_BYTE*)pData,
      &nDataLen);
   if (nError != CKR_OK)
   {
      _SLogError("decryptOneStage() failed (%08x).", nError);
      return nError;
   }

   /* Check plain text match */
   if ((SMemCompare(pData, test_rsa_pkcs_msg, nDataLen)!=0)
      || (nDataLen!= sizeof(test_rsa_pkcs_msg)))
   {
      _SLogError("decryptRsaPkcsWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function decryptRsaPkcsWithCheck:
 *  Description:
 *           Perform a RSA PKCS decryption on encrypted data, 
 *           and compare the result with the expected plaintext.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPrivateKey  = private key handle
 **/
static CK_RV decryptRsaPkcsWithCheck(IN S_HANDLE           hCryptoSession,
                                        IN CK_OBJECT_HANDLE   hPrivateKey)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_RSA_PKCS, NULL_PTR, 0}; 

   CK_BYTE  pData[128-11]; /* length <= k-11 (here the modulus length k is 128 bytes) */
   CK_ULONG nDataLen = sizeof(pData);

   /* decrypt a RSA PKCS cipher text */
   nError = decryptOneStage(
      hCryptoSession, 
      &mechanism, 
      hPrivateKey, 
      (CK_BYTE*)test_rsa_pkcs_cipher,
      sizeof(test_rsa_pkcs_cipher),
      (CK_BYTE*)pData,
      &nDataLen);
   if (nError != CKR_OK)
   {
      _SLogError("decryptOneStage() failed (%08x).", nError);
      return nError;
   }

   /* Check plain text match */
   if ((SMemCompare(pData, test_rsa_pkcs_msg, nDataLen)!=0)
      || (nDataLen!= sizeof(test_rsa_pkcs_msg)))
   {
      _SLogError("decryptRsaPkcsWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function exampleRsaPkcsMechanismEncryptDecrypt:
 *  Description:
 *           Create a RSA key pair, then perform some RSA PKCS encryptions and decryptions
 *           on data (and check the results), then destroy the key pair.
 *           See comments inside the present function for more details.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleRsaPkcsMechanismEncryptDecrypt(IN  S_HANDLE  hCryptoSession, 
                                                      IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE       privateKeyRsaId[2]    ={0x00, 0x01}; 
   CK_BYTE       publicKeyRsaId[2]     ={0x00, 0x02};
   CK_OBJECT_HANDLE hPrivateKeyRsa     = CK_INVALID_HANDLE; 
   CK_OBJECT_HANDLE hPublicKeyRsa      = CK_INVALID_HANDLE;

   /* ------------   RSA Keys   -------------------- */

   /* create a 1024 RSA key pair */
   _SLogTrace(" -  Import a RSA key pair");
   nError = createRsaKeyPair(hCryptoSession,
                             privateKeyRsaId,
                             publicKeyRsaId,
                             bIsToken,
                             &hPrivateKeyRsa,
                             &hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("createRsaKeyPair() failed (%08x).", nError);
      nFirstError = nError;
   }

   /* ---------- RSA PKCS encrypt/decrypt  -------- */

   _SLogTrace(" -  RSA PKCS encryption, one stage (then decryption)");
   nError = encryptRsaPkcsThenDecrypt(hCryptoSession, hPublicKeyRsa, hPrivateKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("encryptRsaPkcsThenDecrypt() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  RSA PKCS decryption, one stage (then comparison)");
   nError = decryptRsaPkcsWithCheck(hCryptoSession, hPrivateKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("decryptRsaPkcsWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ------------  Destroy Keys   ----------------- */

   /* destroy the RSA key pair */
   _SLogTrace(" -  Destroy RSA key pair");
   nError = C_DestroyObject(hCryptoSession, hPrivateKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/**
 *  Function exampleRsaKeyGeneration:
 *  Description:
 *           Generate a RSA key pair, then perform a RSA PKCS encryption and decryption
 *           on data (and check the results), then destroy the key pair.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleRsaKeyGeneration(IN  S_HANDLE  hCryptoSession, 
                                        IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE       privateKeyRsaId[2]    ={0x00, 0x01}; 
   CK_BYTE       publicKeyRsaId[2]     ={0x00, 0x02};
   CK_OBJECT_HANDLE hPrivateKeyRsa     = CK_INVALID_HANDLE; 
   CK_OBJECT_HANDLE hPublicKeyRsa      = CK_INVALID_HANDLE;

   /* generate a 1024 RSA key pair */
   _SLogTrace(" -  Generate a RSA key pair");
   nError = generateRsaKeyPair(hCryptoSession,
      privateKeyRsaId,
      publicKeyRsaId,
      bIsToken,
      &hPrivateKeyRsa,
      &hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("generateRsaKeyPair() failed (%08x).", nError);
      nFirstError = nError;
   }

   /* encrypt, decrypt and compare */
   _SLogTrace(" -  RSA PKCS encryption, one stage (then decryption)");
   nError = encryptRsaPkcsThenDecrypt(hCryptoSession, hPublicKeyRsa, hPrivateKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("encryptRsaPkcsThenDecrypt() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* destroy the RSA key pair */
   _SLogTrace(" -  Destroy RSA key pair");
   nError = C_DestroyObject(hCryptoSession, hPrivateKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/**
 *  RSA OAEP mechanism:
 *  -------------------
 *  RSA OAEP supports single-part encryption and decryption.
 *
 *  Let k denote the modulus length, and hLen denote the hash length. 
 *  Then, we must have:
 *  Function  Input length   Output length 
 *  C_Encrypt  <= k-2-2hLen  =k
 *  C_Decrypt  =k            <=k-2-2hLen
 *
 *  Remark: The RSA OAEP cipher text is computed using some random, 
 *  so we cannot compare the cipher text with an expected one.
 *
 **/

/**
 *  Function encryptRsaOaepSha1ThenDecrypt:
 *  Description:
 *           Perform a RSA OAEP SHA1 encryption on data, then decrypt the encrypted data
 *           and compare with the initial data.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPublicKey   = public key handle
 *           CK_OBJECT_HANDLE hPrivateKey  = private key handle
 **/
static CK_RV encryptRsaOaepSha1ThenDecrypt(IN S_HANDLE           hCryptoSession,
                                              IN CK_OBJECT_HANDLE   hPublicKey,
                                              IN CK_OBJECT_HANDLE   hPrivateKey)
{
   CK_RV       nError;

   /* 
   *  Here RSA OAEP uses the SHA1 algorithm.
   *  If you need to use SHA224, SHA256, SHA384, or SHA512 algorithm, 
   *  just replace CKM_SHA_1 (params.hashAlg) and CKG_MGF1_SHA1 (params.mgf) 
   *  respectively by:
   *     CKM_SHA224  and   CKG_MGF1_SHA224
   *     CKM_SHA256  and   CKG_MGF1_SHA256
   *     CKM_SHA384  and   CKG_MGF1_SHA384
   *     CKM_SHA512  and   CKG_MGF1_SHA512
   */
   CK_RSA_PKCS_OAEP_PARAMS params =
   {
      CKM_SHA_1,           /* param.hashAlg */
      CKG_MGF1_SHA1,       /* param.mgf */
      CKZ_DATA_SPECIFIED,  /* param.source */
      NULL_PTR,            /* param.pSourceData */
      0                    /* param.ulSourceDataLen */
   };
   CK_MECHANISM mechanism = {CKM_RSA_PKCS_OAEP, NULL, sizeof(params)};

   CK_BYTE  pCipher[128]; /* 128 bytes corresponds to a 1024 bits RSA key */
   CK_ULONG nCipherLen = sizeof(pCipher);
   CK_BYTE  pData[128-2-2*20]; /* length <= k-2-2hLen (i.e.128-2-2*20) */
   CK_ULONG nDataLen = sizeof(pData);
   
   mechanism.pParameter = &params;

   /* encrypt a content with RSA OAEP sha1 */
   nError = encryptOneStage(
      hCryptoSession, 
      &mechanism, 
      hPublicKey, 
      (CK_BYTE*)test_rsa_oaep_msg,
      sizeof(test_rsa_oaep_msg), /* length <= k-2-2hLen */
      pCipher,
      &nCipherLen);
   if (nError != CKR_OK)
   {
      _SLogError("encryptOneStage() failed (%08x).", nError);
      return nError;
   }
   
   /* decrypt a RSA OAEP sha1 cipher text */
   nError = decryptOneStage(
      hCryptoSession, 
      &mechanism, 
      hPrivateKey, 
      (CK_BYTE*)pCipher,
      nCipherLen,
      (CK_BYTE*)pData,
      &nDataLen);
   if (nError != CKR_OK)
   {
      _SLogError("decryptOneStage() failed (%08x).", nError);
      return nError;
   }

   /* Check plain text match */
   if ((SMemCompare(pData, test_rsa_oaep_msg, nDataLen)!=0)
      || (nDataLen!= sizeof(test_rsa_oaep_msg)))
   {
      _SLogError("encryptRsaOaepSha1ThenDecrypt(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function decryptRsaOaepSha1WithCheck:
 *  Description:
 *           Perform a RSA OAEP SHA1 decryption on encrypted data, 
 *           and compare the result with the expected plaintext.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hPrivateKey  = private key handle
 **/
static CK_RV decryptRsaOaepSha1WithCheck(IN S_HANDLE           hCryptoSession,
                                            IN CK_OBJECT_HANDLE   hPrivateKey)
{
   CK_RV       nError;

   CK_RSA_PKCS_OAEP_PARAMS params =
   {
      CKM_SHA_1,           /* param.hashAlg */
      CKG_MGF1_SHA1,       /* param.mgf */
      CKZ_DATA_SPECIFIED,  /* param.source */
      NULL_PTR,            /* param.pSourceData */
      0                    /* param.ulSourceDataLen */
   };
   CK_MECHANISM mechanism = {CKM_RSA_PKCS_OAEP, NULL, sizeof(params)};

   CK_BYTE  pData[128-2-2*20]; /* length <= k-2-2hLen (i.e.128-2-2*20) */
   CK_ULONG nDataLen = sizeof(pData);
   
   mechanism.pParameter = &params;

   /* decrypt a RSA OAEP cipher text */
   nError = decryptOneStage(
      hCryptoSession, 
      &mechanism, 
      hPrivateKey, 
      (CK_BYTE*)test_rsa_oaep_sha1_cipher,
      sizeof(test_rsa_oaep_sha1_cipher),
      (CK_BYTE*)pData,
      &nDataLen);
   if (nError != CKR_OK)
   {
      _SLogError("decryptOneStage() failed (%08x).", nError);
      return nError;
   }

   /* Check plain text match */
   if ((SMemCompare(pData, test_rsa_oaep_msg, nDataLen)!=0)
      || (nDataLen!= sizeof(test_rsa_oaep_msg)))
   {
      _SLogError("decryptRsaOaepSha1WithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function exampleRsaOaepMechanism:
 *  Description:
 *           Create a RSA key pair, then perform some RSA OAEP encryptions and decryptions
 *           on data (and check the results), then destroy the key pair.
 *           See comments inside the present function for more details.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleRsaOaepMechanism(IN  S_HANDLE  hCryptoSession, 
                                        IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE       privateKeyRsaId[2]    ={0x00, 0x01}; 
   CK_BYTE       publicKeyRsaId[2]     ={0x00, 0x02};
   CK_OBJECT_HANDLE hPrivateKeyRsa     = CK_INVALID_HANDLE; 
   CK_OBJECT_HANDLE hPublicKeyRsa      = CK_INVALID_HANDLE;

   /* ------------   RSA Keys   -------------------- */

   /* create a 1024 RSA key pair */
   _SLogTrace(" -  Import a RSA key pair");
   nError = createRsaOaepKeyPair(hCryptoSession,
                             privateKeyRsaId,
                             publicKeyRsaId,
                             bIsToken,
                             &hPrivateKeyRsa,
                             &hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("createRsaKeyPair() failed (%08x).", nError);
      nFirstError = nError;
   }

   /* ---------- RSA OAEP encrypt/decrypt  -------- */

   _SLogTrace(" -  RSA OAEP SHA1 encryption, one stage (then decryption)");
   nError = encryptRsaOaepSha1ThenDecrypt(hCryptoSession, hPublicKeyRsa, hPrivateKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("encryptRsaOaepSha1ThenDecrypt() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  RSA OAEP SHA1 decryption, one stage (then comparison)");
   nError = decryptRsaOaepSha1WithCheck(hCryptoSession, hPrivateKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("decryptRsaOaepSha1WithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ------------  Destroy Keys   ----------------- */

   /* destroy the RSA key pair */
   _SLogTrace(" -  Destroy RSA key pair");
   nError = C_DestroyObject(hCryptoSession, hPrivateKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hPublicKeyRsa);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/**
 *  AES mechanism:
 *  --------------
 *  AES supports single-part/multi-parts encryption and decryption.
 *
 *  Encryption: The plaintext lenght MUST be a multiple of AES block size.
 *  Decryption: The ciphertext lenght MUST be a multiple of AES block size.
 *  These conditions are prerequired in the following functions,
 *  unless in the function encryptAesCbcWithCheck() were they are checked.
 *
 **/

/**
 *  Function encryptAesCtrWithCheck:
 *  Description:
 *           Perform a AES CTR encryption on data, then check the result.
 *           The key size is 128 bits in this example.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV encryptAesCtrWithCheck(IN S_HANDLE           hCryptoSession,
                                       IN CK_OBJECT_HANDLE   hSecretKey,
                                       IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_AES_CTR_PARAMS params;
   CK_MECHANISM mechanism = {CKM_AES_CTR, NULL, sizeof(params)}; 

   CK_BYTE  pCipher[16*TEST_MAX_DATA_BLOCK_COUNT]; 
   CK_ULONG nCipherLen;

   CK_ULONG      nBlockLen = AES_BLOCK_SIZE; /* MUST be a multiple of AES block size */

   mechanism.pParameter = &params;   

   /* setup AES CTR params */
   params.ulCounterBits = test_aes_128_ctr_counter_len;
   SMemMove(params.cb, test_aes_128_ctr_cb, 16); /* 'cb' length must be 16 bytes */

   /* output size = input size */
   nCipherLen = sizeof(test_aes_128_ctr_plaintext); 

   /* encrypt the data with AES CTR */
   if(bIsMultiStage == false)
   {
      nError = encryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_128_ctr_plaintext,
         sizeof(test_aes_128_ctr_plaintext), 
         pCipher,
         &nCipherLen);
      if (nError != CKR_OK)
      {
         _SLogError("encryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = encryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_128_ctr_plaintext,
         sizeof(test_aes_128_ctr_plaintext), 
         nBlockLen,
         pCipher);
      if (nError != CKR_OK)
      {
         _SLogError("encryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pCipher, test_aes_128_ctr_ciphertext, nCipherLen)!=0)
      || (nCipherLen!= sizeof(test_aes_128_ctr_ciphertext)))
   {
      _SLogError("encryptAesCtrWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function decryptAesCtrWithCheck:
 *  Description:
 *           Perform a AES CTR decryption on data, then check the result.
 *           The key size is 128 bits in this example.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV decryptAesCtrWithCheck(IN S_HANDLE           hCryptoSession,
                                       IN CK_OBJECT_HANDLE   hSecretKey,
                                       IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_AES_CTR_PARAMS params;
   CK_MECHANISM mechanism = {CKM_AES_CTR, NULL, sizeof(params)}; 

   CK_BYTE  pData[16*TEST_MAX_DATA_BLOCK_COUNT]; 
   CK_ULONG nDataLen;

   CK_ULONG      nBlockLen = AES_BLOCK_SIZE; /* MUST be a multiple of AES block size */

   mechanism.pParameter = &params;  
   
   /* setup AES CTR params */
   params.ulCounterBits = test_aes_128_ctr_counter_len;
   SMemMove(params.cb, test_aes_128_ctr_cb, 16); /* 'cb' length must be 16 bytes */

   /* output size = input size */
   nDataLen = sizeof(test_aes_128_ctr_ciphertext); 

   /* encrypt the data with AES CTR */
   if(bIsMultiStage == false)
   {
      nError = decryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_128_ctr_ciphertext,
         sizeof(test_aes_128_ctr_ciphertext), 
         pData,
         &nDataLen);
      if (nError != CKR_OK)
      {
         _SLogError("decryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = decryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_128_ctr_ciphertext,
         sizeof(test_aes_128_ctr_ciphertext), 
         nBlockLen,
         pData);
      if (nError != CKR_OK)
      {
         _SLogError("decryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check data match */
   if ((SMemCompare(pData, test_aes_128_ctr_plaintext, nDataLen)!=0)
      || (nDataLen!= sizeof(test_aes_128_ctr_plaintext)))
   {
      _SLogError("decryptAesCtrWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}


/*
 *  Function encryptAesEcbWithCheck:
 *  Description:
 *           Perform a AES ECB encryption on data, then check the result.
 *           The key size is 192 bits in this example.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 */
static CK_RV encryptAesEcbWithCheck(IN S_HANDLE           hCryptoSession,
                                       IN CK_OBJECT_HANDLE   hSecretKey,
                                       IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_AES_ECB, NULL_PTR, 0}; 

   CK_BYTE  pCipher[16*TEST_MAX_DATA_BLOCK_COUNT];
   CK_ULONG nCipherLen;

   CK_ULONG      nBlockLen = AES_BLOCK_SIZE; /* MUST be a multiple of AES block size */

   /* output size = input size */
   nCipherLen = sizeof(test_aes_192_ecb_plaintext);

   /* encrypt the data with AES ECB */
   if(bIsMultiStage == false)
   {
      nError = encryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_192_ecb_plaintext,
         sizeof(test_aes_192_ecb_plaintext), 
         pCipher,
         &nCipherLen);
      if (nError != CKR_OK)
      {
         _SLogError("encryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = encryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_192_ecb_plaintext,
         sizeof(test_aes_192_ecb_plaintext), 
         nBlockLen,
         pCipher);
      if (nError != CKR_OK)
      {
         _SLogError("encryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pCipher, test_aes_192_ecb_ciphertext, nCipherLen)!=0)
      || (nCipherLen!= sizeof(test_aes_192_ecb_ciphertext)))
   {
      _SLogError("encryptAesEcbWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function decryptAesEcbWithCheck:
 *  Description:
 *           Perform a AES ECB decryption on data, then check the result.
 *           The key size is 192 bits in this example.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV decryptAesEcbWithCheck(IN S_HANDLE           hCryptoSession,
                                       IN CK_OBJECT_HANDLE   hSecretKey,
                                       IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_AES_ECB, NULL_PTR, 0}; 

   CK_BYTE  pData[16*TEST_MAX_DATA_BLOCK_COUNT]; 
   CK_ULONG nDataLen;

   CK_ULONG      nBlockLen = AES_BLOCK_SIZE; /* MUST be a multiple of AES block size */

   /* output size = input size */
   nDataLen = sizeof(test_aes_192_ecb_ciphertext); 

   /* encrypt the data with AES ECB */
   if(bIsMultiStage == false)
   {
      nError = decryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_192_ecb_ciphertext,
         sizeof(test_aes_192_ecb_ciphertext), 
         pData,
         &nDataLen);
      if (nError != CKR_OK)
      {
         _SLogError("decryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = decryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_192_ecb_ciphertext,
         sizeof(test_aes_192_ecb_ciphertext), 
         nBlockLen,
         pData);
      if (nError != CKR_OK)
      {
         _SLogError("decryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check data match */
   if ((SMemCompare(pData, test_aes_192_ecb_plaintext, nDataLen)!=0)
      || (nDataLen!= sizeof(test_aes_192_ecb_plaintext)))
   {
      _SLogError("decryptAesEcbWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}


/**
 *  Function encryptAesCbcWithCheck:
 *  Description:
 *           Perform a AES CBC encryption on data, then check the result.
 *           The key size is 256 bits in this example.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 * Remark:
 *  nBlockLen and nDataLen MUST be multiple of AES block size.
 *  For instance in encryptAesCbcWithCheck(), this is checked.
 *  Here nBlockLen=16*2 bytes and nDataLen=16*3 bytes. 
 */
static CK_RV encryptAesCbcWithCheck(IN S_HANDLE           hCryptoSession,
                                       IN CK_OBJECT_HANDLE   hSecretKey,
                                       IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_AES_CBC, (CK_VOID_PTR)test_aes_256_cbc_iv, sizeof(test_aes_256_cbc_iv)}; 

   CK_BYTE  pCipher[16*TEST_MAX_DATA_BLOCK_COUNT];
   CK_ULONG nCipherLen;

   CK_ULONG      nBlockLen = AES_BLOCK_SIZE*2; /* MUST be a multiple of AES block size */

   /* output size = input size */
   nCipherLen = sizeof(test_aes_256_cbc_plaintext);  

   /* encrypt the data with AES CBC */
   if(bIsMultiStage == false)
   {

      /* check nDataLen is a multiple of AES block size */
      if((sizeof(test_aes_256_cbc_plaintext)%16)!=0)
      {
         nError = CKR_DATA_LEN_RANGE;
         _SLogError("encryptAesCbcWithCheck failed: nDataLen should be a multiple of AES block size.");
         return nError;
      }

      nError = encryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_256_cbc_plaintext,
         sizeof(test_aes_256_cbc_plaintext), 
         pCipher,
         &nCipherLen);
      if (nError != CKR_OK)
      {
         _SLogError("encryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      /* check nBlockLen is a multiple of AES block size */
      if((nBlockLen%16)!=0)
      {
         nError = CKR_DATA_LEN_RANGE;
         _SLogError("encryptAesCbcWithCheck failed: nBlockLen should be a multiple of AES block size.");
         return nError;
      }

      nError = encryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_256_cbc_plaintext,
         sizeof(test_aes_256_cbc_plaintext), 
         nBlockLen,
         pCipher);
      if (nError != CKR_OK)
      {
         _SLogError("encryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pCipher, test_aes_256_cbc_ciphertext, nCipherLen)!=0)
      || (nCipherLen!= sizeof(test_aes_256_cbc_ciphertext)))
   {
      _SLogError("encryptAesCbcWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}


/**
 *  Function decryptAesCbcWithCheck:
 *  Description:
 *           Perform a AES CBC decryption on data, then check the result.
 *           The key size is 256 bits in this example.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV decryptAesCbcWithCheck(IN S_HANDLE           hCryptoSession,
                                       IN CK_OBJECT_HANDLE   hSecretKey,
                                       IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_AES_CBC, (CK_VOID_PTR)test_aes_256_cbc_iv, sizeof(test_aes_256_cbc_iv)}; 

   CK_BYTE  pData[16*TEST_MAX_DATA_BLOCK_COUNT]; 
   CK_ULONG nDataLen;

   CK_ULONG      nBlockLen = AES_BLOCK_SIZE; /* AES block size = 16 bytes (fixed value) */

   /* output size = input size */
   nDataLen = sizeof(test_aes_256_cbc_ciphertext); 

   /* encrypt the data with AES CBC */
   if(bIsMultiStage == false)
   {
      nError = decryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_256_cbc_ciphertext,
         sizeof(test_aes_256_cbc_ciphertext), 
         pData,
         &nDataLen);
      if (nError != CKR_OK)
      {
         _SLogError("decryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = decryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_aes_256_cbc_ciphertext,
         sizeof(test_aes_256_cbc_ciphertext), 
         nBlockLen,
         pData);
      if (nError != CKR_OK)
      {
         _SLogError("decryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check data match */
   if ((SMemCompare(pData, test_aes_256_cbc_plaintext, nDataLen)!=0)
      || (nDataLen!= sizeof(test_aes_256_cbc_plaintext)))
   {
      _SLogError("decryptAesCbcWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}


/**
 *  Function encryptAesEcbThenDecrypt:
 *  Description:
 *           Perform a AES ECB encryption on data, then decrypt and compare the
 *           decrypted data with the initial data.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV encryptAesEcbThenDecrypt(IN S_HANDLE           hCryptoSession,
                                         IN CK_OBJECT_HANDLE   hSecretKey,
                                         IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_AES_ECB, NULL, 0};

   CK_BYTE  pData [] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   };/* 32 bytes */ 
   CK_ULONG nDataLen = sizeof(pData);  /* MUST be a multiple of AES block size */

   CK_BYTE  pEncryptedData[32]; /* same size as pData size */
   CK_ULONG nEncryptedDataLen;

   CK_BYTE  pDataCheck[32]; /* same size as pData size */
   CK_ULONG nDataCheckLen;

   CK_ULONG      nBlockLen = AES_BLOCK_SIZE; /* MUST be a multiple of AES block size */

   /* output size = input size */
   nEncryptedDataLen = nDataLen;

   /* encrypt the data with AES ECB */
   if(bIsMultiStage == false)
   {
      nError = encryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)pData,
         sizeof(pData), 
         pEncryptedData,
         &nEncryptedDataLen);
      if (nError != CKR_OK)
      {
         _SLogError("encryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = encryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)pData,
         sizeof(pData), 
         nBlockLen,
         pEncryptedData);
      if (nError != CKR_OK)
      {
         _SLogError("encryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   

   /* decrypt and compare the data with the initial data */
   nDataCheckLen = nEncryptedDataLen;

   nError = decryptOneStage(
      hCryptoSession, 
      &mechanism, 
      hSecretKey, 
      (CK_BYTE*)pEncryptedData,
      nEncryptedDataLen, 
      pDataCheck,
      &nDataCheckLen);
   if (nError != CKR_OK)
   {
      _SLogError("decryptOneStage() failed (%08x).", nError);
      return nError;
   }

   if ((SMemCompare(pDataCheck, pData, nDataCheckLen)!=0)
      || (nDataCheckLen!= sizeof(pData)))
   {
      _SLogError("encryptAesEcbThenDecrypt(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}


/**
 *  Function exampleAesMechanism:
 *  Description:
 *           Create AES keys, then perform some AES encryptions and decryptions
 *           on data (and check the results), then destroy the keys.
 *           See comments inside the present function for more details.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleAesMechanism(IN  S_HANDLE  hCryptoSession,
                                    IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE       secretKeyAes128Id[2]  ={0x00, 0x01};
   CK_BYTE       secretKeyAes192Id[2]  ={0x00, 0x02};
   CK_BYTE       secretKeyAes256Id[2]  ={0x00, 0x03};
   CK_OBJECT_HANDLE hSecretKeyAes128   = CK_INVALID_HANDLE;
   CK_OBJECT_HANDLE hSecretKeyAes192   = CK_INVALID_HANDLE;
   CK_OBJECT_HANDLE hSecretKeyAes256   = CK_INVALID_HANDLE;

   /* ------------   AES Keys   -------------------- */
   _SLogTrace(" -  Import a AES key 128 bits");
   nError = createAesKey128(
      hCryptoSession, 
      secretKeyAes128Id, 
      bIsToken,
      &hSecretKeyAes128);
   if (nError != CKR_OK)
   {
      _SLogError("createAesKey128() failed (%08x).", nError);
      nFirstError = nError;
   }

   _SLogTrace(" -  Import a AES key 192 bits");
   nError = createAesKey192(
      hCryptoSession, 
      secretKeyAes192Id, 
      bIsToken,
      &hSecretKeyAes192);
   if (nError != CKR_OK)
   {
      _SLogError("createAesKey192() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  Import a AES key 256 bits");
   nError = createAesKey256(
      hCryptoSession, 
      secretKeyAes256Id, 
      bIsToken,
      &hSecretKeyAes256);
   if (nError != CKR_OK)
   {
      _SLogError("createAesKey256() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ------------   AES CTR   -------------------- */
   _SLogTrace(" -  AES CTR 128 encryption, one stage (then comparison)");
   nError = encryptAesCtrWithCheck(hCryptoSession, hSecretKeyAes128, false);
   if (nError != CKR_OK)
   {
      _SLogError("encryptAesCtrWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  AES CTR 128 encryption, multi stages (then comparison)");
   nError = encryptAesCtrWithCheck(hCryptoSession, hSecretKeyAes128, true);
   if (nError != CKR_OK)
   {
      _SLogError("encryptAesCtrWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  AES CTR 128 decryption, one stage (then comparison)");
   nError = decryptAesCtrWithCheck(hCryptoSession, hSecretKeyAes128, false);
   if (nError != CKR_OK)
   {
      _SLogError("decryptAesCtrWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  AES CTR 128 decryption, multi stages (then comparison)");
   nError = decryptAesCtrWithCheck(hCryptoSession, hSecretKeyAes128, true);
   if (nError != CKR_OK)
   {
      _SLogError("decryptAesCtrWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ------------   AES ECB   -------------------- */
   _SLogTrace(" -  AES ECB 192 encryption, one stage (then comparison)");
   nError = encryptAesEcbWithCheck(hCryptoSession, hSecretKeyAes192, false);
   if (nError != CKR_OK)
   {
      _SLogError("encryptAesEcbWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  AES ECB 192 encryption, multi stages (then comparison)");
   nError = encryptAesEcbWithCheck(hCryptoSession, hSecretKeyAes192, true);
   if (nError != CKR_OK)
   {
      _SLogError("encryptAesEcbWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  AES ECB 192 decryption, one stage (then comparison)");
   nError = decryptAesEcbWithCheck(hCryptoSession, hSecretKeyAes192, false);
   if (nError != CKR_OK)
   {
      _SLogError("decryptAesEcbWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  AES ECB 192 decryption, multi stages (then comparison)");
   nError = decryptAesEcbWithCheck(hCryptoSession, hSecretKeyAes192, true);
   if (nError != CKR_OK)
   {
      _SLogError("decryptAesEcbWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ------------   AES CBC   -------------------- */
   _SLogTrace(" -  AES CBC 256 encryption, one stage (then comparison)");
   nError = encryptAesCbcWithCheck(hCryptoSession, hSecretKeyAes256, false);
   if (nError != CKR_OK)
   {
      _SLogError("encryptAesCbcWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  AES CBC 256 encryption, multi stages (then comparison)");
   nError = encryptAesCbcWithCheck(hCryptoSession, hSecretKeyAes256, true);
   if (nError != CKR_OK)
   {
      _SLogError("encryptAesCbcWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  AES CBC 256 decryption, one stage (then comparison)");
   nError = decryptAesCbcWithCheck(hCryptoSession, hSecretKeyAes256, false);
   if (nError != CKR_OK)
   {
      _SLogError("decryptAesCbcWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  AES CBC 256 decryption, multi stages (then comparison)");
   nError = decryptAesCbcWithCheck(hCryptoSession, hSecretKeyAes256, true);
   if (nError != CKR_OK)
   {
      _SLogError("decryptAesCbcWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* destroy AES keys */
   _SLogTrace(" -  Destroy AES keys");
   nError = C_DestroyObject(hCryptoSession, hSecretKeyAes128);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hSecretKeyAes192);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hSecretKeyAes256);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/**
 *  Function exampleAesKeyGeneration:
 *  Description:
 *           Generate a AES key, then perform an AES ECB encryption and decryption
 *           on data (and check the results), then destroy the key.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleAesKeyGeneration(IN  S_HANDLE  hCryptoSession,
                                        IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE       secretKeyAes128Id[2]  ={0x00, 0x01};
   CK_OBJECT_HANDLE hSecretKeyAes128   = CK_INVALID_HANDLE;

   /* generate a AES key */
   _SLogTrace(" -  Generate a AES key 128");
   nError = generateAesKey128(hCryptoSession,
      secretKeyAes128Id,
      bIsToken,
      &hSecretKeyAes128);
   if (nError != CKR_OK)
   {
      _SLogError("generateAesKey128() failed (%08x).", nError);
      nFirstError = nError;
   }

   /* encrypt, decrypt, and compare */
   _SLogTrace(" -  AES 128 ECB encryption, one stage (then decryption)");
   nError = encryptAesEcbThenDecrypt(hCryptoSession, hSecretKeyAes128, false);
   if (nError != CKR_OK)
   {
      _SLogError("encryptEcbCtrThenDecrypt() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
      
   /* destroy AES keys */
   _SLogTrace(" -  Destroy AES key");
   nError = C_DestroyObject(hCryptoSession, hSecretKeyAes128);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/**
 *  DES/3DES Mechanism:
 *  -------------------
 *  DES/3DES supports single-part/multi-parts encryption and decryption.
 *
 *  Encryption: The plaintext lenght MUST be a multiple of DES block size.
 *  Decryption: The ciphertext lenght MUST be a multiple of DES block size.
 *  These conditions are prerequired in the following functions.
 *
 **/

/**
 *  Function encryptDesEcbWithCheck:
 *  Description:
 *           Perform a DES ECB encryption on data, then check the result.
 *           The key size is 64 bits.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 **/
static CK_RV encryptDesEcbWithCheck(IN S_HANDLE           hCryptoSession,
                                       IN CK_OBJECT_HANDLE   hSecretKey,
                                       IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DES_ECB, NULL_PTR, 0}; 

   CK_BYTE  pCipher[8*TEST_MAX_DATA_BLOCK_COUNT];
   CK_ULONG nCipherLen;

   CK_ULONG      nBlockLen = DES_BLOCK_SIZE; /* MUST be a multiple of DES block size (=8 bytes) */

   /* output size = input size */
   nCipherLen = sizeof(test_des_ecb_plaintext); 

   /* encrypt the data with DES ECB */
   if(bIsMultiStage == false)
   {
      nError = encryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des_ecb_plaintext,
         sizeof(test_des_ecb_plaintext), 
         pCipher,
         &nCipherLen);
      if (nError != CKR_OK)
      {
         _SLogError("encryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = encryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des_ecb_plaintext,
         sizeof(test_des_ecb_plaintext), 
         nBlockLen,
         pCipher);
      if (nError != CKR_OK)
      {
         _SLogError("encryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pCipher, test_des_ecb_ciphertext, nCipherLen)!=0)
      || (nCipherLen!= sizeof(test_des_ecb_ciphertext)))
   {
      _SLogError("encryptDesEcbWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function decryptDesEcbWithCheck:
 *  Description:
 *           Perform a DES ECB decryption on data, then check the result.
 *           The key size is 64 bits.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 **/
static CK_RV decryptDesEcbWithCheck(IN S_HANDLE           hCryptoSession,
                                       IN CK_OBJECT_HANDLE   hSecretKey,
                                       IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DES_ECB, NULL_PTR, 0}; 

   CK_BYTE  pData[8*TEST_MAX_DATA_BLOCK_COUNT];
   CK_ULONG nDataLen;

   CK_ULONG      nBlockLen = DES_BLOCK_SIZE; /* MUST be a multiple of DES block size (=8 bytes) */

   /* output size = input size */
   nDataLen = sizeof(test_des_ecb_ciphertext); 

   /* encrypt the data with AES CBC */
   if(bIsMultiStage == false)
   {
      nError = decryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des_ecb_ciphertext,
         sizeof(test_des_ecb_ciphertext), 
         pData,
         &nDataLen);
      if (nError != CKR_OK)
      {
         _SLogError("decryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = decryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des_ecb_ciphertext,
         sizeof(test_des_ecb_ciphertext), 
         nBlockLen,
         pData);
      if (nError != CKR_OK)
      {
         _SLogError("decryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pData, test_des_ecb_plaintext, nDataLen)!=0)
      || (nDataLen!= sizeof(test_des_ecb_plaintext)))
   {
      _SLogError("decryptDesEcbWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function encryptDesCbcWithCheck:
 *  Description:
 *           Perform a DES ECB encryption on data, then check the result.
 *           The key size is 64 bits.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 **/
static CK_RV encryptDesCbcWithCheck(IN S_HANDLE           hCryptoSession,
                                       IN CK_OBJECT_HANDLE   hSecretKey,
                                       IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DES_CBC, (CK_VOID_PTR)test_des_cbc_iv, sizeof(test_des_cbc_iv)}; 

   CK_BYTE  pCipher[8*TEST_MAX_DATA_BLOCK_COUNT];
   CK_ULONG nCipherLen;

   CK_ULONG      nBlockLen = DES_BLOCK_SIZE; /* MUST be a multiple of DES block size (=8 bytes) */

   /* output size = input size */
   nCipherLen = sizeof(test_des_cbc_plaintext); 

   /* encrypt the data with DES CBC */
   if(bIsMultiStage == false)
   {
      nError = encryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des_cbc_plaintext,
         sizeof(test_des_cbc_plaintext), 
         pCipher,
         &nCipherLen);
      if (nError != CKR_OK)
      {
         _SLogError("encryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = encryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des_cbc_plaintext,
         sizeof(test_des_cbc_plaintext), 
         nBlockLen,
         pCipher);
      if (nError != CKR_OK)
      {
         _SLogError("encryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pCipher, test_des_cbc_ciphertext, nCipherLen)!=0)
      || (nCipherLen!= sizeof(test_des_cbc_ciphertext)))
   {
      _SLogError("encryptDesEcbWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function decryptDesCbcWithCheck:
 *  Description:
 *           Perform a DES ECB decryption on data, then check the result.
 *           The key size is 64 bits.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 **/
static CK_RV decryptDesCbcWithCheck(IN S_HANDLE           hCryptoSession,
                                       IN CK_OBJECT_HANDLE   hSecretKey,
                                       IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DES_CBC, (CK_VOID_PTR)test_des_cbc_iv, sizeof(test_des_cbc_iv)}; 

   CK_BYTE  pData[8*TEST_MAX_DATA_BLOCK_COUNT];
   CK_ULONG nDataLen;

   CK_ULONG      nBlockLen = DES_BLOCK_SIZE; /* MUST be a multiple of DES block size (=8 bytes) */

   /* output size = input size */
   nDataLen = sizeof(test_des_cbc_ciphertext); 

   /* encrypt the data with AES CBC */
   if(bIsMultiStage == false)
   {
      nError = decryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des_cbc_ciphertext,
         sizeof(test_des_cbc_ciphertext), 
         pData,
         &nDataLen);
      if (nError != CKR_OK)
      {
         _SLogError("decryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = decryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des_cbc_ciphertext,
         sizeof(test_des_cbc_ciphertext), 
         nBlockLen,
         pData);
      if (nError != CKR_OK)
      {
         _SLogError("decryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pData, test_des_cbc_plaintext, nDataLen)!=0)
      || (nDataLen!= sizeof(test_des_cbc_plaintext)))
   {
      _SLogError("decryptDesCbcWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function encryptDes3EcbWithDes3KeyWithCheck:
 *  Description:
 *           Perform a DES3 ECB encryption on data, with a DES3 key (192 bits),
 *           then check the result.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 **/
static CK_RV encryptDes3EcbWithDes3KeyWithCheck(IN S_HANDLE           hCryptoSession,
                                                   IN CK_OBJECT_HANDLE   hSecretKey,
                                                   IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DES3_ECB, NULL_PTR, 0}; 

   CK_BYTE  pCipher[8*TEST_MAX_DATA_BLOCK_COUNT];
   CK_ULONG nCipherLen;

   CK_ULONG      nBlockLen = DES_BLOCK_SIZE; /* MUST be a multiple of DES block size (=8 bytes) */

   /* output size = input size */
   nCipherLen = sizeof(test_des3_ecb_plaintext); 

   /* encrypt the data with AES CBC */
   if(bIsMultiStage == false)
   {
      nError = encryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des3_ecb_plaintext,
         sizeof(test_des3_ecb_plaintext), 
         pCipher,
         &nCipherLen);
      if (nError != CKR_OK)
      {
         _SLogError("encryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = encryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des3_ecb_plaintext,
         sizeof(test_des3_ecb_plaintext), 
         nBlockLen,
         pCipher);
      if (nError != CKR_OK)
      {
         _SLogError("encryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pCipher, test_des3_ecb_ciphertext, nCipherLen)!=0)
      || (nCipherLen!= sizeof(test_des3_ecb_ciphertext)))
   {
      _SLogError("encryptDes3EcbWithDes3KeyWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}


/**
 *  Function decryptDes3EcbWithDes3KeyWithCheck:
 *  Description:
 *           Perform a DES3 ECB decryption on data, with a DES3 key (192 bits),
 *           then check the result.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 **/
static CK_RV decryptDes3EcbWithDes3KeyWithCheck(IN S_HANDLE           hCryptoSession,
                                                   IN CK_OBJECT_HANDLE   hSecretKey,
                                                   IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DES3_ECB, NULL_PTR, 0}; 

   CK_BYTE  pData[8*TEST_MAX_DATA_BLOCK_COUNT];
   CK_ULONG nDataLen;

   CK_ULONG      nBlockLen = DES_BLOCK_SIZE; /* MUST be a multiple of DES block size (=8 bytes) */

   /* output size = input size */
   nDataLen = sizeof(test_des3_ecb_ciphertext); 

   /* encrypt the data with AES CBC */
   if(bIsMultiStage == false)
   {
      nError = decryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des3_ecb_ciphertext,
         sizeof(test_des3_ecb_ciphertext), 
         pData,
         &nDataLen);
      if (nError != CKR_OK)
      {
         _SLogError("decryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = decryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des3_ecb_ciphertext,
         sizeof(test_des3_ecb_ciphertext), 
         nBlockLen,
         pData);
      if (nError != CKR_OK)
      {
         _SLogError("decryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pData, test_des3_ecb_plaintext, nDataLen)!=0)
      || (nDataLen!= sizeof(test_des3_ecb_plaintext)))
   {
      _SLogError("decryptDes3EcbWithDes3KeyWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function encryptDes3CbcWithDes2KeyWithCheck:
 *  Description:
 *           Perform a DES3 CBC encryption on data, with a DES2 key (128 bits),
 *           then check the result.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 **/
static CK_RV encryptDes3CbcWithDes2KeyWithCheck(IN S_HANDLE           hCryptoSession,
                                                   IN CK_OBJECT_HANDLE   hSecretKey,
                                                   IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DES3_CBC, (CK_VOID_PTR)test_des3_cbc_iv_2, sizeof(test_des3_cbc_iv_2)}; 

   CK_BYTE  pCipher[8*TEST_MAX_DATA_BLOCK_COUNT];
   CK_ULONG nCipherLen;

   CK_ULONG      nBlockLen = DES_BLOCK_SIZE; /* MUST be a multiple of DES block size (=8 bytes) */

   /* output size = input size */
   nCipherLen = sizeof(test_des3_cbc_plaintext_2); 

   /* encrypt the data with AES CBC */
   if(bIsMultiStage == false)
   {
      nError = encryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des3_cbc_plaintext_2,
         sizeof(test_des3_cbc_plaintext_2), 
         pCipher,
         &nCipherLen);
      if (nError != CKR_OK)
      {
         _SLogError("encryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = encryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des3_cbc_plaintext_2,
         sizeof(test_des3_cbc_plaintext_2), 
         nBlockLen,
         pCipher);
      if (nError != CKR_OK)
      {
         _SLogError("encryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pCipher, test_des3_cbc_ciphertext_2, nCipherLen)!=0)
      || (nCipherLen!= sizeof(test_des3_cbc_ciphertext_2)))
   {
      _SLogError("encryptDes3CbcWithDes2KeyWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}





/**
 *  Function decryptDes3CbcWithDes2KeyWithCheck:
 *  Description:
 *           Perform a DES3 CBC decryption on data, with a DES2 key (128 bits),
 *           then check the result.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 **/
static CK_RV decryptDes3CbcWithDes2KeyWithCheck(IN S_HANDLE           hCryptoSession,
                                                   IN CK_OBJECT_HANDLE   hSecretKey,
                                                   IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DES3_CBC, (CK_VOID_PTR)test_des3_cbc_iv_2, sizeof(test_des3_cbc_iv_2)}; 

   CK_BYTE  pData[8*TEST_MAX_DATA_BLOCK_COUNT];
   CK_ULONG nDataLen;

   CK_ULONG      nBlockLen = DES_BLOCK_SIZE; /* MUST be a multiple of DES block size (=8 bytes) */

   /* output size = input size */
   nDataLen = sizeof(test_des3_cbc_ciphertext_2); 

   /* encrypt the data with AES CBC */
   if(bIsMultiStage == false)
   {
      nError = decryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des3_cbc_ciphertext_2,
         sizeof(test_des3_cbc_ciphertext_2), 
         pData,
         &nDataLen);
      if (nError != CKR_OK)
      {
         _SLogError("decryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = decryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)test_des3_cbc_ciphertext_2,
         sizeof(test_des3_cbc_ciphertext_2), 
         nBlockLen,
         pData);
      if (nError != CKR_OK)
      {
         _SLogError("decryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   
   /* Check cipher text match */
   if ((SMemCompare(pData, test_des3_cbc_plaintext_2, nDataLen)!=0)
      || (nDataLen!= sizeof(test_des3_cbc_plaintext_2)))
   {
      _SLogError("decryptDes3CbcWithDes2KeyWithCheck(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function encryptDes3EcbThenDecrypt:
 *  Description:
 *           Perform a DES3 ECB encryption on data, then decrypt and compare the
 *           decrypted data with the initial data.
 *  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
 *           CK_OBJECT_HANDLE hSecretKey   = secret key handle
 *           bool          bIsMultiStage   = if true then multi-stages operation
 *                                           else single-stage operation.
 **/
static CK_RV encryptDes3EcbThenDecrypt(IN S_HANDLE           hCryptoSession,
                                          IN CK_OBJECT_HANDLE   hSecretKey,
                                          IN bool               bIsMultiStage)
{
   CK_RV       nError;

   CK_MECHANISM mechanism = {CKM_DES3_ECB, NULL, 0};

   CK_BYTE  pData [] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   };/* 32 bytes */ 
   CK_ULONG nDataLen = sizeof(pData);  /* MUST be a multiple of DES block size */

   CK_BYTE  pEncryptedData[32]; /* same size as pData size */
   CK_ULONG nEncryptedDataLen;

   CK_BYTE  pDataCheck[32]; /* same size as pData size */
   CK_ULONG nDataCheckLen;

   CK_ULONG      nBlockLen = DES_BLOCK_SIZE*2; /* MUST be a multiple of DES block size */

   /* output size = input size */
   nEncryptedDataLen = nDataLen;

   /* encrypt the data with DES3 ECB */
   if(bIsMultiStage == false)
   {
      nError = encryptOneStage(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)pData,
         sizeof(pData), 
         pEncryptedData,
         &nEncryptedDataLen);
      if (nError != CKR_OK)
      {
         _SLogError("encryptOneStage() failed (%08x).", nError);
         return nError;
      }
   }
   else
   {
      nError = encryptMultiStages(
         hCryptoSession, 
         &mechanism, 
         hSecretKey, 
         (CK_BYTE*)pData,
         sizeof(pData), 
         nBlockLen,
         pEncryptedData);
      if (nError != CKR_OK)
      {
         _SLogError("encryptMultiStages() failed (%08x).", nError);
         return nError;
      }
   }
   

   /* decrypt and compare the data with the initial data */
   nDataCheckLen = nEncryptedDataLen;

   nError = decryptOneStage(
      hCryptoSession, 
      &mechanism, 
      hSecretKey, 
      (CK_BYTE*)pEncryptedData,
      nEncryptedDataLen, 
      pDataCheck,
      &nDataCheckLen);
   if (nError != CKR_OK)
   {
      _SLogError("decryptOneStage() failed (%08x).", nError);
      return nError;
   }

   if ((SMemCompare(pDataCheck, pData, nDataCheckLen)!=0)
      || (nDataCheckLen!= sizeof(pData)))
   {
      _SLogError("encryptAesEcbThenDecrypt(): data does not match.");
      return CKR_VENDOR_DEFINED;
   }

   return CKR_OK;
}

/**
 *  Function exampleDesMechanism:
 *  Description:
 *           Create DES/DES2/DES3 keys, then perform some DES/DES3 encryptions and decryptions
 *           on data (and check the results), then destroy the keys.
 *           See comments inside the present function for more details.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleDesMechanism(IN  S_HANDLE  hCryptoSession, 
                                    IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE       secretKeyDesId[2]     ={0x00, 0x01};
   CK_BYTE       secretKeyDes2Id[2]    ={0x00, 0x02};
   CK_BYTE       secretKeyDes3Id[2]    ={0x00, 0x03};
   CK_OBJECT_HANDLE hSecretKeyDes      = CK_INVALID_HANDLE;
   CK_OBJECT_HANDLE hSecretKeyDes2     = CK_INVALID_HANDLE;
   CK_OBJECT_HANDLE hSecretKeyDes3     = CK_INVALID_HANDLE;

   /* ----------  DES/DES2/DES3 Keys -------------- */

   _SLogTrace(" -  Import a DES key (64 bits)");
   nError = createDesKey(
      hCryptoSession, 
      secretKeyDesId, 
      bIsToken, 
      &hSecretKeyDes);
   if (nError != CKR_OK)
   {
      _SLogError("createDesKey() failed (%08x).", nError);
      nFirstError = nError;
   }

    _SLogTrace(" -  Import a DES2 key (128 bits)");
   nError = createDes2Key(
      hCryptoSession, 
      secretKeyDes2Id, 
      bIsToken, 
      &hSecretKeyDes2);
   if (nError != CKR_OK)
   {
      _SLogError("createDesKey2() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  Import a DES3 key (192 bits)");
   nError = createDes3Key(
      hCryptoSession, 
      secretKeyDes3Id, 
      bIsToken, 
      &hSecretKeyDes3);
   if (nError != CKR_OK)
   {
      _SLogError("createDesKey3() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* -----------------   DES       ---------------- */
   /* DES algorithm uses DES keys (64 bits) */

   _SLogTrace(" -  DES ECB encryption, one stage (then comparison)");
   nError = encryptDesEcbWithCheck(hCryptoSession, hSecretKeyDes, false);
   if (nError != CKR_OK)
   {
      _SLogError("encryptDesEcbWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  DES ECB encryption, multi stages (then comparison)");
   nError = encryptDesEcbWithCheck(hCryptoSession, hSecretKeyDes, true);
   if (nError != CKR_OK)
   {
      _SLogError("encryptDesEcbWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  DES ECB decryption, one stage (then comparison)");
   nError = decryptDesEcbWithCheck(hCryptoSession, hSecretKeyDes, false);
   if (nError != CKR_OK)
   {
      _SLogError("decryptDesEcbWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  DES ECB decryption, multi stages (then comparison)");
   nError = decryptDesEcbWithCheck(hCryptoSession, hSecretKeyDes, true);
   if (nError != CKR_OK)
   {
      _SLogError("decryptDesEcbWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

  /* ------------ */
   _SLogTrace(" -  DES CBC encryption, one stage (then comparison)");
   nError = encryptDesCbcWithCheck(hCryptoSession, hSecretKeyDes, false);
   if (nError != CKR_OK)
   {
      _SLogError("encryptDesCbcWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  DES CBC encryption, multi stages (then comparison)");
   nError = encryptDesCbcWithCheck(hCryptoSession, hSecretKeyDes, true);
   if (nError != CKR_OK)
   {
      _SLogError("encryptDesCbcWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  DES CBC decryption, one stage (then comparison)");
   nError = decryptDesCbcWithCheck(hCryptoSession, hSecretKeyDes, false);
   if (nError != CKR_OK)
   {
      _SLogError("decryptDesCbcWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  DES CBC decryption, multi stages (then comparison)");
   nError = decryptDesCbcWithCheck(hCryptoSession, hSecretKeyDes, true);
   if (nError != CKR_OK)
   {
      _SLogError("decryptDesCbcWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ---------------  Triple DES  ----------------- */
   /* Triple DES algorithm uses DES2 keys (128 bits) or DES3 keys (192 bits) */

   _SLogTrace(" -  DES3 ECB encryption, DES3 key, one stage (then comparison)");
   nError =  encryptDes3EcbWithDes3KeyWithCheck(hCryptoSession, hSecretKeyDes3, false);
   if (nError != CKR_OK)
   {
      _SLogError("encryptDes3EcbWithDes3KeyWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  DES3 ECB encryption, DES3 key, multi stages (then comparison)");
   nError =  encryptDes3EcbWithDes3KeyWithCheck(hCryptoSession, hSecretKeyDes3, true);
   if (nError != CKR_OK)
   {
      _SLogError("encryptDes3EcbWithDes3KeyWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  DES3 ECB decryption, DES3 key, one stage (then comparison)");
   nError =  decryptDes3EcbWithDes3KeyWithCheck(hCryptoSession, hSecretKeyDes3, false);
   if (nError != CKR_OK)
   {
      _SLogError("decryptDes3EcbWithDes3KeyWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  DES3 ECB decryption, DES3 key, multi stages (then comparison)");
   nError =  decryptDes3EcbWithDes3KeyWithCheck(hCryptoSession, hSecretKeyDes3, true);
   if (nError != CKR_OK)
   {
      _SLogError("decryptDes3EcbWithDes3KeyWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

  /* ------------ */
   _SLogTrace(" -  DES3 CBC encryption, DES2 key, one stage (then comparison)");
   nError =  encryptDes3CbcWithDes2KeyWithCheck(hCryptoSession, hSecretKeyDes2, false);
   if (nError != CKR_OK)
   {
      _SLogError("encryptDes3CbcWithDes2KeyWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  DES3 CBC encryption, DES2 key, multi stages (then comparison)");
   nError =  encryptDes3CbcWithDes2KeyWithCheck(hCryptoSession, hSecretKeyDes2, true);
   if (nError != CKR_OK)
   {
      _SLogError("encryptDes3CbcWithDes2KeyWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  DES3 CBC decryption, DES2 key, one stage (then comparison)");
   nError =  decryptDes3CbcWithDes2KeyWithCheck(hCryptoSession, hSecretKeyDes2, false);
   if (nError != CKR_OK)
   {
      _SLogError("decryptDes3CbcWithDes2KeyWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   _SLogTrace(" -  DES3 CBC decryption, DES2 key, multi stages (then comparison)");
   nError =  decryptDes3CbcWithDes2KeyWithCheck(hCryptoSession, hSecretKeyDes2, true);
   if (nError != CKR_OK)
   {
      _SLogError("decryptDes3CbcWithDes2KeyWithCheck() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }


   /* destroy DES keys */
   _SLogTrace(" -  Destroy DES/DES2/DES3 keys");
   nError = C_DestroyObject(hCryptoSession, hSecretKeyDes);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hSecretKeyDes2);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hSecretKeyDes3);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}


/**
 *  Function exampleDesKeyGeneration:
 *  Description:
 *           Generate a DES2 key, then perform a DES3 ECB encryption and decryption
 *           on data (and check the results), then destroy the key.
 *  Input:   S_HANDLE       hCryptoSession = cryptoki session handle
 *           CK_BBOOL       bIsToken       = true if and only if the keys are token objects
 **/
static CK_RV exampleDesKeyGeneration(IN  S_HANDLE  hCryptoSession,
                                        IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE       secretKeyDes2Id[2]    ={0x00, 0x01};
   CK_OBJECT_HANDLE hSecretKeyDes2     = CK_INVALID_HANDLE;

   /* generate a DES2 key */
   _SLogTrace(" -  Generate a DES2 key");
   nError = generateDes2Key(hCryptoSession,
      secretKeyDes2Id,
      bIsToken,
      &hSecretKeyDes2);
   if (nError != CKR_OK)
   {
      _SLogError("generateDes2Key() failed (%08x).", nError);
      nFirstError = nError;
   }

   _SLogTrace(" -  DES3 ECB encryption, DES2 key, multi stages (then decryption)");
   nError = encryptDes3EcbThenDecrypt(hCryptoSession, hSecretKeyDes2, true);
   if (nError != CKR_OK)
   {
      _SLogError("encryptDes3EcbThenDecrypt() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  Destroy DES2 key");
   nError = C_DestroyObject(hCryptoSession, hSecretKeyDes2);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/* ----------------------------------------------------------------------------
*   Random generation functions 
* ---------------------------------------------------------------------------- */
/**
*  Function exampleRandomMechanism:
*  Description:
*        Generate a random number of 128 bits, then used this value to create 
*        a AES 128 key.
*  Input:   S_HANDLE      hCryptoSession  = cryptoki session handle
**/
static CK_RV exampleRandomMechanism(IN S_HANDLE   hCryptoSession)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE  pRandomData[16]; /* 128 bits */
   CK_BYTE  pSeed[16]; 
   /* CK_ULONG nIndex; */

   CK_ATTRIBUTE keyTemplate[] =
   {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_aes_key_type,       sizeof(test_aes_key_type) },
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) }, 
      {CKA_VALUE,             NULL,              sizeof(pRandomData) },
   };
   CK_OBJECT_HANDLE hKey;
   
   keyTemplate[3].pValue = pRandomData;

   _SLogTrace(" -  Generate a random");
   nError = C_SeedRandom(hCryptoSession, (CK_BYTE*)pSeed, sizeof(pSeed));
   if (nError != CKR_OK)
   {
      _SLogError("C_SeedRandom() failed (%08x).", nError);
      nFirstError = nError;
   }

   nError = C_GenerateRandom(hCryptoSession, (CK_BYTE*)pRandomData, sizeof(pRandomData));
   if (nError != CKR_OK)
   {
      _SLogError("C_GenerateRandom() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /*
   for(nIndex = 0; nIndex < sizeof(pRandomData); nIndex++)
   {
      _SLogTrace(" 0x%02x,", pRandomData[nIndex]);
   }
   */

   /* Use this random */
   _SLogTrace(" -  Use it to create a AES 128 bits key");
   nError = C_CreateObject(
      hCryptoSession,
      keyTemplate,
      ATTRIBUTES_SIZE(keyTemplate),
      &hKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/* ----------------------------------------------------------------------------
*   Key agrement functions 
* ---------------------------------------------------------------------------- */
/**
 *  Diffie Hellman mechanism (DH):
 *  -----------------------------
 *  IMPORTANT REMARK (about C_DeriveKey):
 *  The derived key can be retrieved in clear-text, using C_GetAttributeValue, IF AND ONLY IF 
 *  the following conditions are BOTH satisfied:
 *     - condition(1):   the attribute CKAV_ALLOW_NON_SENSITIVE_DERIVED_KEY of the private  
 *                       key template is present and set to CK_TRUE;
 *     - condition(2):   the attribute CKA_SENSITIVE of the derived key template is present 
 *                       and set to CK_FALSE.
 **/

/**
 *  Function exampleDhMechanism1:
 *  Description:
 *           Import DH private key, derive with other's public key, 
 *           retrieve the derived key in clear-text, check the result,
 *           then destroy keys.
 *           Conditions (1) and (2) are both satisfied.
 *  Inputs:  S_HANDLE  hCryptoSession = the crypto session
 *           CK_BBOOL   bIsToken      = if CK_TRUE then keys are token objects, 
 *                                      else (CK_FALSE) they are session objects. 
 **/
static CK_RV exampleDhMechanism1(IN  S_HANDLE  hCryptoSession, 
                                    IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   CK_BYTE        privateKeyId[2]       = {0x00, 0x01}; 
   CK_BYTE        genericSecretKeyId[2] = {0x00, 0x03};
   CK_OBJECT_HANDLE hPrivateKey = CK_INVALID_HANDLE;
   CK_OBJECT_HANDLE hGenericSecretKey = CK_INVALID_HANDLE;

   /* 
   The attribute CKA_DERIVE must be present and set to CK_TRUE if you want to use the 
   private key for key derivation. 
   */
   CK_ATTRIBUTE privateKeyTemplateA[] = {
      {CKA_CLASS,             (CK_VOID_PTR)&test_private_key_class,  sizeof(test_private_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) },       
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_dh_key_type,        sizeof(test_dh_key_type) }, 
      {CKA_DERIVE,            (CK_VOID_PTR)&test_true,               sizeof(test_true) },
      {CKA_PRIME,             (CK_VOID_PTR)test_dh_1024_P,           sizeof(test_dh_1024_P) },/* mandatory for C_DeriveKey */
      {CKA_BASE,              (CK_VOID_PTR)test_dh_1024_G,           sizeof(test_dh_1024_G) },
      {CKA_VALUE,             (CK_VOID_PTR)test_dh_1024_private_A,   sizeof(test_dh_1024_private_A) },
      {CKAV_ALLOW_NON_SENSITIVE_DERIVED_KEY, (CK_VOID_PTR)&test_true, sizeof(test_true) }, /* condition(1) satisfied */
   };

   /* 
   The attribute CKA_KEY_TYPE of the derived key must be set to CKK_GENERIC_SECRET. 
   The attribute CKA_VALUE_LEN must be either omitted or set the size of the prime p. 
   */
   static const CK_ULONG genericSecretKeyLen = sizeof(test_dh_1024_P); /* if present, MUST be size of prime p */
   CK_ATTRIBUTE genericSecretKeyTemplate[] = {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_ID,                NULL,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) },       
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_generic_key_type,   sizeof(test_generic_key_type) },
      {CKA_VALUE_LEN,         (CK_VOID_PTR)&genericSecretKeyLen,     sizeof(genericSecretKeyLen) }, /* optional */
      {CKA_SENSITIVE,         (CK_VOID_PTR)&test_false,              sizeof(test_false) }, /* condition(2) satisfied */
   };

   /* the derivation mechanism takes the other's public key as parameter */
   CK_MECHANISM mechanismDeriveA  = {CKM_DH_PKCS_DERIVE, (CK_VOID_PTR)test_dh_1024_public_B, sizeof(test_dh_1024_public_B)};
   
   /* the following template is used to retrieve in clear-text the derived key value */
   CK_BYTE key[128]; /* size of prime p */
   CK_ATTRIBUTE genericSecretValueTemplate[] =
   {
      {CKA_VALUE, NULL, sizeof(key)}
   };
   genericSecretValueTemplate[0].pValue = key;
   
   
   privateKeyTemplateA[1].pValue = privateKeyId;
   privateKeyTemplateA[2].pValue = &bIsToken;
   genericSecretKeyTemplate[1].pValue = genericSecretKeyId;
   genericSecretKeyTemplate[2].pValue = &bIsToken;   

   /* import private key */
   _SLogTrace(" -  Import DH private key");
   nError = C_CreateObject(
      hCryptoSession,
      privateKeyTemplateA,
      ATTRIBUTES_SIZE(privateKeyTemplateA),
      &hPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      nFirstError = nError;
   }

   /* derive with other's public key */
   _SLogTrace(" -  Derive secret key");
   nError = C_DeriveKey(
      hCryptoSession,
      &mechanismDeriveA,
      hPrivateKey,
      genericSecretKeyTemplate,
      ATTRIBUTES_SIZE(genericSecretKeyTemplate),
      &hGenericSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DeriveKey returned (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* retrieve the derived key in clear-text */
   _SLogTrace(" -  Retrieve secret key value (then comparison)");
   nError = C_GetAttributeValue(
      hCryptoSession,
      hGenericSecretKey,
      genericSecretValueTemplate,
      ATTRIBUTES_SIZE(genericSecretValueTemplate));
   if (nError != CKR_OK)
   {
      _SLogError("C_GetAttributeValue returned (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* check the result */
   if ((SMemCompare(genericSecretValueTemplate[0].pValue, test_dh_1024_shared_secret, genericSecretValueTemplate[0].ulValueLen)!=0)
      || (genericSecretValueTemplate[0].ulValueLen!= sizeof(test_dh_1024_shared_secret)))
   {
      _SLogError("exampleDhMechanism1(): data does not match.");
      /* nError = S_ERROR_GENERIC; */
      nError = CKR_VENDOR_DEFINED;
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* destroy keys */
   _SLogTrace(" -  Destroy keys");
   nError = C_DestroyObject(hCryptoSession, hPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hGenericSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}


/**
 *  Function exampleDhMechanism2:
 *  Description:
 *           Generate DH key pair, derive with other's public key, 
 *           try to retrieve the derived key in clear-text (it must fail),
 *           then destroy keys.
 *           Conditions (1) and (2) are NOT both satisfied.
 *  Inputs:  S_HANDLE  hCryptoSession = the crypto session
 *           CK_BBOOL   bIsToken      = if CK_TRUE then keys are token objects, 
 *                                      else (CK_FALSE) they are session objects. 
 **/
static CK_RV exampleDhMechanism2(IN  S_HANDLE  hCryptoSession, 
                                    IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   static const CK_BYTE        privateKeyId[2]       ={0x00, 0x01}; 
   static const CK_BYTE        publicKeyId[2]        ={0x00, 0x02};
   static const CK_BYTE        genericSecretKeyId[2] ={0x00, 0x03};
   CK_OBJECT_HANDLE hPrivateKey = CK_INVALID_HANDLE;
   CK_OBJECT_HANDLE hPublicKey = CK_INVALID_HANDLE;                                 
   CK_OBJECT_HANDLE hGenericSecretKey = CK_INVALID_HANDLE;


   CK_ATTRIBUTE publicKeyTemplate[] = {
      {CKA_CLASS,             (CK_VOID_PTR)&test_public_key_class,   sizeof(test_public_key_class) },/* optional */
      {CKA_ID,                (CK_VOID_PTR)publicKeyId,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_PRIME,             (CK_VOID_PTR)test_dh_1024_P,           sizeof(test_dh_1024_P) },
      {CKA_BASE,              (CK_VOID_PTR)test_dh_1024_G,           sizeof(test_dh_1024_G) },
   };

   /* 
   The attribute CKA_VALUE_BITS of the private key is optional for C_GenerateKeyPair. 
   If present, it specifies the desired private key length in bits.
   The attribute CKA_DERIVE must be present and set to CK_TRUE if you want to use the 
   private key for key derivation. 
   */
   static const CK_ULONG privateKeyBits = 160;
   CK_ATTRIBUTE privateKeyTemplate[] = {
      {CKA_CLASS,             (CK_VOID_PTR)&test_private_key_class,  sizeof(test_private_key_class) },/* optional */
      {CKA_ID,                (CK_VOID_PTR)privateKeyId,             2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_DERIVE,            (CK_VOID_PTR)&test_true,               sizeof(test_true) }, /* mandatory for C_DeriveKey */
      {CKA_VALUE_BITS,        (CK_VOID_PTR)&privateKeyBits,          sizeof(privateKeyBits) }, /* optional */
      {CKAV_ALLOW_NON_SENSITIVE_DERIVED_KEY, (CK_VOID_PTR)&test_true, sizeof(test_true) }, /* condition(1) satisfied */
   };

   CK_MECHANISM mechanismDhGenerate = { CKM_DH_PKCS_KEY_PAIR_GEN, NULL, 0 };


   /* 
   The attribute CKA_KEY_TYPE of the derived key must be set to CKK_GENERIC_SECRET. 
   The attribute CKA_VALUE_LEN must be either omitted or set the size of the prime p. 
   */
   static const CK_ULONG genericSecretKeyLen = sizeof(test_dh_1024_P); /* if present, MUST be size of prime p */
   CK_ATTRIBUTE genericSecretKeyTemplate[] = {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_ID,                (CK_VOID_PTR)genericSecretKeyId,       2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_generic_key_type,   sizeof(test_generic_key_type) },
      {CKA_VALUE_LEN,         (CK_VOID_PTR)&genericSecretKeyLen,     sizeof(genericSecretKeyLen) }, /* optional */
   }; /* condition(2) not satisfied */


   /* Remark: 
   *  In this example condition(1) is satisfied but useless since condition (2) is not satisfied. 
   *  The attribute CKAV_ALLOW_NON_SENSITIVE_DERIVED_KEY could be removed. 
   *  A similar example would be when condition(2) is satisfied but not condition(1), 
   *  or when the two conditions are not satisfied.
   */

   /* the derivation mechanism takes the other's public key as parameter */
   CK_MECHANISM mechanismDerive  = {CKM_DH_PKCS_DERIVE, (CK_VOID_PTR)test_dh_1024_public_B, sizeof(test_dh_1024_public_B)};
   
   /* the following template is used to retrieve in clear-text the derived key value */
   CK_BYTE key[128]; /* size of prime p */
   CK_ATTRIBUTE genericSecretValueTemplate[] =
   {
      {CKA_VALUE, NULL, sizeof(key)}
   };
   genericSecretValueTemplate[0].pValue = key;

   publicKeyTemplate[2].pValue = &bIsToken;
   privateKeyTemplate[2].pValue = &bIsToken;
   genericSecretKeyTemplate[2].pValue = &bIsToken;   
   
   /* generate key pair */
   _SLogTrace(" -  Generate DH key pair");
   nError = C_GenerateKeyPair(
      hCryptoSession,
      &mechanismDhGenerate,
      publicKeyTemplate,
      ATTRIBUTES_SIZE(publicKeyTemplate),
      privateKeyTemplate,
      ATTRIBUTES_SIZE(privateKeyTemplate),
      &hPublicKey,
      &hPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_GenerateKeyPair returned (%08x).", nError);
      nFirstError = nError;
   }

   /* derive with other's public key */
   _SLogTrace(" -  Derive secret key");
   nError = C_DeriveKey(
      hCryptoSession,
      &mechanismDerive,
      hPrivateKey,
      genericSecretKeyTemplate,
      ATTRIBUTES_SIZE(genericSecretKeyTemplate),
      &hGenericSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DeriveKey returned (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* try to retrieve the derived key in clear-text: 
   it must fail since condition(1) and condition(2) are not BOTH satisfied. */
   _SLogTrace(" -  Try to retrieve secret key value (must fail)");
   nError = C_GetAttributeValue(
      hCryptoSession,
      hGenericSecretKey,
      genericSecretValueTemplate,
      ATTRIBUTES_SIZE(genericSecretValueTemplate));
   if (nError != CKR_ATTRIBUTE_SENSITIVE)
   {
      _SLogError("C_GetAttributeValue should return CKR_ATTRIBUTE_SENSITIVE().", nError);
      if (nError == CKR_OK)
      {
         if (nFirstError == CKR_OK) nFirstError = CKR_GENERAL_ERROR;
      }
      else
      {
         if (nFirstError == CKR_OK) nFirstError = nError;
      }
   }

   /* destroy keys */
   _SLogTrace(" -  Destroy keys");
   nError = C_DestroyObject(hCryptoSession, hPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hPublicKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hGenericSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/**
 *  Function exampleDhMechanism3:
 *  Description:
 *           Generate DH key pair, derive with other's public key, 
 *           retrieve the derived key in clear-text, 
 *           then create an AES key from the derived key, 
 *           perform a AES encryption on data then decrypt, 
 *           and compare the decrypted data with the initial data.
 *           Then destroy keys.
 *           Conditions (1) and (2) are both satisfied.
 *  Inputs:  S_HANDLE  hCryptoSession = the crypto session
 *           CK_BBOOL   bIsToken      = if CK_TRUE then keys are token objects, 
 *                                      else (CK_FALSE) they are session objects. 
 *
 *  Remark: 
 *  The exampleDhMechanism3 function illustrates a typical usecase of the DH mechanism.
 *  DH key pair generation could be replaced by DH key import. 
 *  If you want to create a DES/DES2/DES3 key (instead of a AES key) from the derived key, 
 *  do not forget that those keys contain parity bits.
 *  For each DES/DES2/DES3 key bytes:
 *  - choose the bits 1 to 7 from the derived key;
 *  - set the 8th bit to '0' or '1' such that the parity of the byte is odd.
 **/
static CK_RV exampleDhMechanism3(IN  S_HANDLE  hCryptoSession, 
                                    IN CK_BBOOL   bIsToken)
{
   CK_RV       nError;
   CK_RV       nFirstError = CKR_OK;

   static const CK_BYTE        privateKeyId[2]       ={0x00, 0x01}; 
   static const CK_BYTE        publicKeyId[2]        ={0x00, 0x02};
   static const CK_BYTE        genericSecretKeyId[2] ={0x00, 0x03};
   static const CK_BYTE        secretKeyId[2]        ={0x00, 0x04};
   CK_OBJECT_HANDLE hPrivateKey = CK_INVALID_HANDLE;
   CK_OBJECT_HANDLE hPublicKey = CK_INVALID_HANDLE;                                 
   CK_OBJECT_HANDLE hGenericSecretKey = CK_INVALID_HANDLE;
   CK_OBJECT_HANDLE hSecretKey = CK_INVALID_HANDLE;

   CK_ATTRIBUTE publicKeyTemplate[] = {
      {CKA_CLASS,             (CK_VOID_PTR)&test_public_key_class,   sizeof(test_public_key_class) },/* optional */
      {CKA_ID,                (CK_VOID_PTR)publicKeyId,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_PRIME,             (CK_VOID_PTR)test_dh_1024_P,           sizeof(test_dh_1024_P) },
      {CKA_BASE,              (CK_VOID_PTR)test_dh_1024_G,           sizeof(test_dh_1024_G) },
   };

   /* 
   The attribute CKA_VALUE_BITS of the private key is optional for C_GenerateKeyPair. 
   If present, it specifies the desired private key length in bits.
   The attribute CKA_DERIVE must be present and set to CK_TRUE if you want to use the 
   private key for key derivation. 
   */
   static const CK_ULONG privateKeyBits = 160;
   CK_ATTRIBUTE privateKeyTemplate[] = {
      {CKA_CLASS,             (CK_VOID_PTR)&test_private_key_class,  sizeof(test_private_key_class) },/* optional */
      {CKA_ID,                (CK_VOID_PTR)privateKeyId,             2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_DERIVE,            (CK_VOID_PTR)&test_true,               sizeof(test_true) }, /* mandatory for C_DeriveKey */
      {CKA_VALUE_BITS,        (CK_VOID_PTR)&privateKeyBits,          sizeof(privateKeyBits) }, /* optional */
      {CKAV_ALLOW_NON_SENSITIVE_DERIVED_KEY, (CK_VOID_PTR)&test_true, sizeof(test_true) }, /* condition(1) satisfied */
   };


   CK_MECHANISM mechanismDhGenerate = { CKM_DH_PKCS_KEY_PAIR_GEN, NULL, 0 };

   /* 
   The attribute CKA_KEY_TYPE of the derived key must be set to CKK_GENERIC_SECRET. 
   The attribute CKA_VALUE_LEN must be either omitted or set the size of the prime p. 
   */
   static const CK_ULONG genericSecretKeyLen = sizeof(test_dh_1024_P); /* if present, MUST be size of prime p */
   CK_ATTRIBUTE genericSecretKeyTemplate[] = {
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_ID,                (CK_VOID_PTR)genericSecretKeyId,       2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_generic_key_type,   sizeof(test_generic_key_type) },
      {CKA_VALUE_LEN,         (CK_VOID_PTR)&genericSecretKeyLen,     sizeof(genericSecretKeyLen) }, /* optional */
      {CKA_SENSITIVE,         (CK_VOID_PTR)&test_false,              sizeof(test_false) }, /* condition(2) satisfied */
   };

   /* the derivation mechanism takes the other's public key as parameter */
   CK_MECHANISM mechanismDerive  = {CKM_DH_PKCS_DERIVE, (CK_VOID_PTR)test_dh_1024_public_B, sizeof(test_dh_1024_public_B)};
   
   /* the following template is used to retrieve in clear-text the derived key value */
   CK_BYTE key[128]; /* size of prime p */
   CK_ATTRIBUTE genericSecretValueTemplate[] =
   {
      {CKA_VALUE, NULL, sizeof(key)}
   };


   static const CK_ULONG secretKeyLength = 32; /* AES key 256 bits = 32 bytes */
   CK_ATTRIBUTE secretKeyTemplate[] = { 
      {CKA_CLASS,             (CK_VOID_PTR)&test_secret_key_class,   sizeof(test_secret_key_class) },
      {CKA_ID,                (CK_VOID_PTR)secretKeyId,              2}, 
      {CKA_TOKEN,             NULL,                sizeof(bIsToken) }, 
      {CKA_KEY_TYPE,          (CK_VOID_PTR)&test_aes_key_type,       sizeof(test_aes_key_type) },
      {CKA_ENCRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) },
      {CKA_DECRYPT,           (CK_VOID_PTR)&test_true,               sizeof(test_true) },
      {CKA_VALUE,             NULL,                      secretKeyLength},
   };

   CK_MECHANISM mechanismEncryptDecrypt = {CKM_AES_ECB, NULL, 0};

   CK_BYTE  pData [] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   };/* 32 bytes */
   CK_ULONG nDataLen = sizeof(pData); 
   CK_BYTE  pEncryptedData[32];
   CK_ULONG nEncryptedDataLen = sizeof(pEncryptedData);
   CK_BYTE  pDataCheck[32];
   CK_ULONG nDataCheckLen = sizeof(pDataCheck);
   
   
   genericSecretValueTemplate[0].pValue = key;

   publicKeyTemplate[2].pValue = &bIsToken;
   privateKeyTemplate[2].pValue = &bIsToken;
   genericSecretKeyTemplate[2].pValue = &bIsToken;
   secretKeyTemplate[2].pValue = &bIsToken;   
   secretKeyTemplate[6].pValue = key;   

   /* generate key pair */
   _SLogTrace(" -  Generate DH key pair");
   nError = C_GenerateKeyPair(
      hCryptoSession,
      &mechanismDhGenerate,
      publicKeyTemplate,
      ATTRIBUTES_SIZE(publicKeyTemplate),
      privateKeyTemplate,
      ATTRIBUTES_SIZE(privateKeyTemplate),
      &hPublicKey,
      &hPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_GenerateKeyPair returned (%08x).", nError);
      nFirstError = nError;
   }

   /* derive with other's public key */
   _SLogTrace(" -  Derive secret key");
   nError = C_DeriveKey(
      hCryptoSession,
      &mechanismDerive,
      hPrivateKey,
      genericSecretKeyTemplate,
      ATTRIBUTES_SIZE(genericSecretKeyTemplate),
      &hGenericSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DeriveKey returned (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* retrieve the derived key in clear-text */
   _SLogTrace(" -  Retrieve secret key value");
   nError = C_GetAttributeValue(
      hCryptoSession,
      hGenericSecretKey,
      genericSecretValueTemplate,
      ATTRIBUTES_SIZE(genericSecretValueTemplate));
   if (nError != CKR_OK)
   {
      _SLogError("C_GetAttributeValue returned (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace(" -  Use it for AES encryption (then decryption)");
   /* create an AES key from the generic secret key */
   nError = C_CreateObject(
      hCryptoSession,
      secretKeyTemplate,
      ATTRIBUTES_SIZE(secretKeyTemplate),
      &hSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_CreateObject returned (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* encrypt data (could be one stage or multi stages) */
   nError = encryptOneStage(
      hCryptoSession,
      &mechanismEncryptDecrypt,
      hSecretKey,
      pData,
      nDataLen,
      pEncryptedData,
      &nEncryptedDataLen);
   if (nError != CKR_OK)
   {
      _SLogError("encryptOneStage returned (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

    /* check: decrypt, then compare the result with the initial data */
   nError = decryptOneStage(
      hCryptoSession,
      &mechanismEncryptDecrypt,
      hSecretKey,
      pEncryptedData,
      nEncryptedDataLen,
      pDataCheck,
      &nDataCheckLen);
   if (nError != CKR_OK)
   {
      _SLogError("encryptOneStage returned (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   if ((SMemCompare(pDataCheck, pData, nDataCheckLen)!=0)
      || (nDataCheckLen!= sizeof(pData)))
   {
      _SLogError("exampleDhMechanism3(): data does not match.");
      /* nError = S_ERROR_GENERIC; */
      nError = CKR_VENDOR_DEFINED;
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* destroy keys */
   _SLogTrace(" -  Destroy keys");
   nError = C_DestroyObject(hCryptoSession, hPrivateKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hPublicKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hGenericSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   nError = C_DestroyObject(hCryptoSession, hSecretKey);
   if (nError != CKR_OK)
   {
      _SLogError("C_DestroyObject() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   return nFirstError;
}

/* ----------------------------------------------------------------------------
*   Main functions
* ---------------------------------------------------------------------------- */
/**
*  Function cryptoCatalogueService:
*  Description:
*        Perform a list of cryptographic operations on data, and check all the results.
*        The operations are: 
*        - key creations/ key generations,  
*        - digests, 
*        - signatures/verifications,
*        - encryptions/decryptions, 
*        - ramdom numbers generation, 
*        - key agreement.                  
*        See comments inside the present function for more details.
*
*  Remark:
*  In this example, if bIsToken is set to CK_TRUE then all the keys are token
*  objects otherwise they are session objects.
**/
static CK_RV cryptoCatalogueService(void)
{
   CK_RV          nError;
   S_HANDLE       hCryptoSession;
   CK_RV          nFirstError = CKR_OK;
   CK_BBOOL       bIsToken = CK_TRUE;

   _SLogTrace("Start cryptoCatalogueService()");

   /* open a crypto session */
   nError = C_OpenSession (
      1,
      CKF_SERIAL_SESSION | CKF_RW_SESSION,
      NULL_PTR,
      NULL_PTR,
      &hCryptoSession); 
   if (nError != CKR_OK)
   {
      _SLogError("Could not open session (%08x).", nError);
      return nError;
   }

   /* ------------   Digests  --------------------- */

   _SLogTrace("exampleDigestMechanism:");
   nError = exampleDigestMechanism(hCryptoSession);
   if (nError != CKR_OK)
   {
      _SLogError("exampleDigestMechanism() failed (%08x).", nError);
      nFirstError = nError;
   }

   /* ---------  Signature / Verification  -------- */

   _SLogTrace("exampleRsaPkcsMechanismSignVerify:");
   nError = exampleRsaPkcsMechanismSignVerify(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleRsaPkcsMechanismSignVerify() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace("exampleRsaPssMechanism:");
   nError = exampleRsaPssMechanism(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleRsaPssMechanism() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace("exampleDsaMechanism:");
   nError = exampleDsaMechanism(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleDsaMechanism() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace("exampleDsaKeyGeneration:");
   nError = exampleDsaKeyGeneration(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleDsaKeyGeneration() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ---------  Encryption / Decryption  --------- */

   _SLogTrace("exampleRsaPkcsMechanismEncryptDecrypt:");
   nError = exampleRsaPkcsMechanismEncryptDecrypt(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleRsaPkcsMechanismEncryptDecrypt() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace("exampleRsaKeyGeneration:");
   nError = exampleRsaKeyGeneration(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleRsaKeyGeneration() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace("exampleRsaOaepMechanism:");
   nError = exampleRsaOaepMechanism(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleRsaOaepMechanism() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace("exampleAesMechanism:");
   nError = exampleAesMechanism(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleAesMechanism() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }
   
   _SLogTrace("exampleAesKeyGeneration:");
   nError = exampleAesKeyGeneration(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleAesKeyGeneration() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace("exampleDesMechanism:");
   nError = exampleDesMechanism(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleDesMechanism() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace("exampleDesKeyGeneration:");
   nError = exampleDesKeyGeneration(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleDesKeyGeneration() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* -------------- Random ----------------------- */

   _SLogTrace("exampleRandomMechanism:");
   nError = exampleRandomMechanism(hCryptoSession);
   if (nError != CKR_OK)
   {
      _SLogError("exampleRandomMechanism() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ------------  Key agreement   --------------- */

   _SLogTrace("exampleDhMechanism1:");
   nError = exampleDhMechanism1(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleDhMechanism1() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace("exampleDhMechanism2:");
   nError = exampleDhMechanism2(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleDhMechanism2() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   _SLogTrace("exampleDhMechanism3:");
   nError = exampleDhMechanism3(hCryptoSession, bIsToken);
   if (nError != CKR_OK)
   {
      _SLogError("exampleDhMechanism3() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /* ------------   CKO_DATA  --------------------- */

   _SLogTrace("exampleCkoDataObject:");
   nError = exampleCkoDataObject(hCryptoSession);
   if (nError != CKR_OK)
   {
       _SLogError("exampleCkoDataObject() failed (%08x).", nError);
       if (nFirstError == CKR_OK) nFirstError = nError;
   }

   /*----------------------------------------------------------------*/
   /* close the crypto session */
   nError = C_CloseSession(hCryptoSession);
   if (nError != CKR_OK)
   {
      _SLogError("C_CloseSession() failed (%08x).", nError);
      if (nFirstError == CKR_OK) nFirstError = nError;
   }

   if (nFirstError == CKR_OK)
   {
      _SLogTrace("cryptoCatalogueService() was successfully performed.");
   }
   else
   {
      _SLogTrace("cryptoCatalogueService() failed (at least one of the tests failed).");
   }

   return nFirstError;
}
/* ----------------------------------------------------------------------------
 *   Service Entry Points
 * ---------------------------------------------------------------------------- */

/**
 *  Function SRVXCreate:
 *  Description:
 *        The function SRVXCreate is the service constructor, which the system
 *        calls when it creates a new instance of the service.
 *        Here this function implements nothing.
 **/
S_RESULT SRVX_EXPORT SRVXCreate(void)
{
   _SLogTrace("SRVXCreate");

   return S_SUCCESS;
}

/**
 *  Function SRVXDestroy:
 *  Description:
 *        The function SRVXDestroy is the service destructor, which the system
 *        calls when the instance is being destroyed.
 *        Here this function implements nothing.
 **/
void SRVX_EXPORT SRVXDestroy(void)
{
   _SLogTrace("SRVXDestroy");
}

/**
 *  Function SRVXOpenClientSession:
 *  Description:
 *        The system calls this function when a new client
 *        connects to the service instance.
 **/
S_RESULT SRVX_EXPORT SRVXOpenClientSession(
   uint32_t nParamTypes,
   IN OUT S_PARAM pParams[4],
   OUT void** ppSessionContext )
{
    S_VAR_NOT_USED(nParamTypes);
    S_VAR_NOT_USED(pParams);
    S_VAR_NOT_USED(ppSessionContext);

    _SLogTrace("SRVXOpenClientSession");

    return S_SUCCESS;
}

/**
 *  Function SRVXCloseClientSession:
 *  Description:
 *        The system calls this function to indicate that a session is being closed.
 **/
void SRVX_EXPORT SRVXCloseClientSession(IN OUT void* pSessionContext)
{
    S_VAR_NOT_USED(pSessionContext);

    _SLogTrace("SRVXCloseClientSession");
}

/**
 *  Function SRVXInvokeCommand:
 *  Description:
 *        The system calls this function when the client invokes a command within
 *        a session of the instance.
 *        Here this function perfoms a switch on the command Id received as input,
 *        and returns the main function matching to the command id.
 **/
S_RESULT SRVX_EXPORT SRVXInvokeCommand(IN OUT void* pSessionContext,
                                       uint32_t nCommandID,
                                       uint32_t nParamTypes,
                                       IN OUT S_PARAM pParams[4])
{
    S_VAR_NOT_USED(pSessionContext);
    
   _SLogTrace("SRVXInvokeCommand");

   switch(nCommandID)
   {
   case CMD_CRYPTOCATALOGUE:
      return (S_RESULT)cryptoCatalogueService();
   default:
      _SLogError("invalid command ID: 0x%X", nCommandID);
      return (S_RESULT)CKR_VENDOR_DEFINED;
   }
}
