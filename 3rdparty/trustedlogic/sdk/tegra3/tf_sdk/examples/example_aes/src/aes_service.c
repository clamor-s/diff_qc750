/**
 * Copyright (c) 2009 Trusted Logic S.A.
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
 * Implementation notes:
 * This implementation assumes it runs over 32-bit processors.
 * It uses 4 tables of 1K.
 * It'd be possible to get down to
 * 2 tables of 256 bytes, but this is a time/memory compromise.
*/

/* ----------------------------------------------------------------------------
*   Includes
* ---------------------------------------------------------------------------- */
#include "ssdi.h"
#include "aes_protocol.h"
#include "aes_service.h"

/* ----------------------------------------------------------------------------
*   Defines
* ---------------------------------------------------------------------------- */

#define SSRV_CRYPTO_MACRO_LOAD32B(x, y) \
     { x = ((uint32_t)((y)[0] & 255)<<24) | \
           ((uint32_t)((y)[1] & 255)<<16) | \
           ((uint32_t)((y)[2] & 255)<<8)  | \
           ((uint32_t)((y)[3] & 255));}

#define SSRV_CRYPTO_MACRO_STORE32B(x, y) \
     { (y)[0] = (uint8_t)(((x)>>24)&255); \
       (y)[1] = (uint8_t)(((x)>>16)&255); \
       (y)[2] = (uint8_t)(((x)>>8)&255);  \
       (y)[3] = (uint8_t)((x)&255); }

/**
 * Rotates 32-bit x, y bits to the right
 * For example, x=0xD0 00 01 18 and y = 4 => 0x18 D0 00 01
 * This macro is independant of platform's endianness and
 * word size. It does not assume x is a word, simply
 * requires x is 32-bits long.
 */
#define SSRV_CRYPTO_MACRO_ROR(x, y) ( ((((uint32_t)(x)&0xFFFFFFFFUL)>>(uint32_t)((y)&31)) | \
        ((uint32_t)(x)<<(uint32_t)(32-((y)&31)))) & 0xFFFFFFFFUL)

/* maximum block's size for block ciphers AES, DES & 3DES */
#define SSRV_CRYPTO_IMPL_MAX_BLOCK_CIPHER_SIZE 16

/* ----------------------------------------------------------------------------
*   Statics
* ---------------------------------------------------------------------------- */

/*
 * Lookup table for decryption
 * TD[x] = SboxInv[x].[0e, 09, 0d, 0b];
 */
static const uint32_t TD[256] =
{
   0x51f4a750UL /* (52).[0e, 09, 0d, 0b] */, 0x7e416553UL, 0x1a17a4c3UL, 0x3a275e96UL,
   0x3bab6bcbUL, 0x1f9d45f1UL, 0xacfa58abUL, 0x4be30393UL,
   0x2030fa55UL, 0xad766df6UL, 0x88cc7691UL, 0xf5024c25UL,
   0x4fe5d7fcUL, 0xc52acbd7UL, 0x26354480UL, 0xb562a38fUL,
   0xdeb15a49UL, 0x25ba1b67UL, 0x45ea0e98UL, 0x5dfec0e1UL,
   0xc32f7502UL, 0x814cf012UL, 0x8d4697a3UL, 0x6bd3f9c6UL,
   0x038f5fe7UL, 0x15929c95UL, 0xbf6d7aebUL, 0x955259daUL,
   0xd4be832dUL, 0x587421d3UL, 0x49e06929UL, 0x8ec9c844UL,
   0x75c2896aUL, 0xf48e7978UL, 0x99583e6bUL, 0x27b971ddUL,
   0xbee14fb6UL, 0xf088ad17UL, 0xc920ac66UL, 0x7dce3ab4UL,
   0x63df4a18UL, 0xe51a3182UL, 0x97513360UL, 0x62537f45UL,
   0xb16477e0UL, 0xbb6bae84UL, 0xfe81a01cUL, 0xf9082b94UL,
   0x70486858UL, 0x8f45fd19UL, 0x94de6c87UL, 0x527bf8b7UL,
   0xab73d323UL, 0x724b02e2UL, 0xe31f8f57UL, 0x6655ab2aUL,
   0xb2eb2807UL, 0x2fb5c203UL, 0x86c57b9aUL, 0xd33708a5UL,
   0x302887f2UL, 0x23bfa5b2UL, 0x02036abaUL, 0xed16825cUL,
   0x8acf1c2bUL, 0xa779b492UL, 0xf307f2f0UL, 0x4e69e2a1UL,
   0x65daf4cdUL, 0x0605bed5UL, 0xd134621fUL, 0xc4a6fe8aUL,
   0x342e539dUL, 0xa2f355a0UL, 0x058ae132UL, 0xa4f6eb75UL,
   0x0b83ec39UL, 0x4060efaaUL, 0x5e719f06UL, 0xbd6e1051UL,
   0x3e218af9UL, 0x96dd063dUL, 0xdd3e05aeUL, 0x4de6bd46UL,
   0x91548db5UL, 0x71c45d05UL, 0x0406d46fUL, 0x605015ffUL,
   0x1998fb24UL, 0xd6bde997UL, 0x894043ccUL, 0x67d99e77UL,
   0xb0e842bdUL, 0x07898b88UL, 0xe7195b38UL, 0x79c8eedbUL,
   0xa17c0a47UL, 0x7c420fe9UL, 0xf8841ec9UL, 0x00000000UL,
   0x09808683UL, 0x322bed48UL, 0x1e1170acUL, 0x6c5a724eUL,
   0xfd0efffbUL, 0x0f853856UL, 0x3daed51eUL, 0x362d3927UL,
   0x0a0fd964UL, 0x685ca621UL, 0x9b5b54d1UL, 0x24362e3aUL,
   0x0c0a67b1UL, 0x9357e70fUL, 0xb4ee96d2UL, 0x1b9b919eUL,
   0x80c0c54fUL, 0x61dc20a2UL, 0x5a774b69UL, 0x1c121a16UL,
   0xe293ba0aUL, 0xc0a02ae5UL, 0x3c22e043UL, 0x121b171dUL,
   0x0e090d0bUL, 0xf28bc7adUL, 0x2db6a8b9UL, 0x141ea9c8UL,
   0x57f11985UL, 0xaf75074cUL, 0xee99ddbbUL, 0xa37f60fdUL,
   0xf701269fUL, 0x5c72f5bcUL, 0x44663bc5UL, 0x5bfb7e34UL,
   0x8b432976UL, 0xcb23c6dcUL, 0xb6edfc68UL, 0xb8e4f163UL,
   0xd731dccaUL, 0x42638510UL, 0x13972240UL, 0x84c61120UL,
   0x854a247dUL, 0xd2bb3df8UL, 0xaef93211UL, 0xc729a16dUL,
   0x1d9e2f4bUL, 0xdcb230f3UL, 0x0d8652ecUL, 0x77c1e3d0UL,
   0x2bb3166cUL, 0xa970b999UL, 0x119448faUL, 0x47e96422UL,
   0xa8fc8cc4UL, 0xa0f03f1aUL, 0x567d2cd8UL, 0x223390efUL,
   0x87494ec7UL, 0xd938d1c1UL, 0x8ccaa2feUL, 0x98d40b36UL,
   0xa6f581cfUL, 0xa57ade28UL, 0xdab78e26UL, 0x3fadbfa4UL,
   0x2c3a9de4UL, 0x5078920dUL, 0x6a5fcc9bUL, 0x547e4662UL,
   0xf68d13c2UL, 0x90d8b8e8UL, 0x2e39f75eUL, 0x82c3aff5UL,
   0x9f5d80beUL, 0x69d0937cUL, 0x6fd52da9UL, 0xcf2512b3UL,
   0xc8ac993bUL, 0x10187da7UL, 0xe89c636eUL, 0xdb3bbb7bUL,
   0xcd267809UL, 0x6e5918f4UL, 0xec9ab701UL, 0x834f9aa8UL,
   0xe6956e65UL, 0xaaffe67eUL, 0x21bccf08UL, 0xef15e8e6UL,
   0xbae79bd9UL, 0x4a6f36ceUL, 0xea9f09d4UL, 0x29b07cd6UL,
   0x31a4b2afUL, 0x2a3f2331UL, 0xc6a59430UL, 0x35a266c0UL,
   0x744ebc37UL, 0xfc82caa6UL, 0xe090d0b0UL, 0x33a7d815UL,
   0xf104984aUL, 0x41ecdaf7UL, 0x7fcd500eUL, 0x1791f62fUL,
   0x764dd68dUL, 0x43efb04dUL, 0xccaa4d54UL, 0xe49604dfUL,
   0x9ed1b5e3UL, 0x4c6a881bUL, 0xc12c1fb8UL, 0x4665517fUL,
   0x9d5eea04UL, 0x018c355dUL, 0xfa877473UL, 0xfb0b412eUL,
   0xb3671d5aUL, 0x92dbd252UL, 0xe9105633UL, 0x6dd64713UL,
   0x9ad7618cUL, 0x37a10c7aUL, 0x59f8148eUL, 0xeb133c89UL,
   0xcea927eeUL, 0xb761c935UL, 0xe11ce5edUL, 0x7a47b13cUL,
   0x9cd2df59UL, 0x55f2733fUL, 0x1814ce79UL, 0x73c737bfUL,
   0x53f7cdeaUL, 0x5ffdaa5bUL, 0xdf3d6f14UL, 0x7844db86UL,
   0xcaaff381UL, 0xb968c43eUL, 0x3824342cUL, 0xc2a3405fUL,
   0x161dc372UL, 0xbce2250cUL, 0x283c498bUL, 0xff0d9541UL,
   0x39a80171UL, 0x080cb3deUL, 0xd8b4e49cUL, 0x6456c190UL,
   0x7bcb8461UL, 0xd532b670UL, 0x486c5c74UL, 0xd0b85742UL,
};

/* table to use for the last round of encryption
*/
static const uint8_t SBOX[256] =
{
   0x63, 0x7c, 0x77, 0x7b,
   0xf2, 0x6b, 0x6f, 0xc5,
   0x30, 0x01, 0x67, 0x2b,
   0xfe, 0xd7, 0xab, 0x76,
   0xca, 0x82, 0xc9, 0x7d,
   0xfa, 0x59, 0x47, 0xf0,
   0xad, 0xd4, 0xa2, 0xaf,
   0x9c, 0xa4, 0x72, 0xc0,
   0xb7, 0xfd, 0x93, 0x26,
   0x36, 0x3f, 0xf7, 0xcc,
   0x34, 0xa5, 0xe5, 0xf1,
   0x71, 0xd8, 0x31, 0x15,
   0x04, 0xc7, 0x23, 0xc3,
   0x18, 0x96, 0x05, 0x9a,
   0x07, 0x12, 0x80, 0xe2,
   0xeb, 0x27, 0xb2, 0x75,
   0x09, 0x83, 0x2c, 0x1a,
   0x1b, 0x6e, 0x5a, 0xa0,
   0x52, 0x3b, 0xd6, 0xb3,
   0x29, 0xe3, 0x2f, 0x84,
   0x53, 0xd1, 0x00, 0xed,
   0x20, 0xfc, 0xb1, 0x5b,
   0x6a, 0xcb, 0xbe, 0x39,
   0x4a, 0x4c, 0x58, 0xcf,
   0xd0, 0xef, 0xaa, 0xfb,
   0x43, 0x4d, 0x33, 0x85,
   0x45, 0xf9, 0x02, 0x7f,
   0x50, 0x3c, 0x9f, 0xa8,
   0x51, 0xa3, 0x40, 0x8f,
   0x92, 0x9d, 0x38, 0xf5,
   0xbc, 0xb6, 0xda, 0x21,
   0x10, 0xff, 0xf3, 0xd2,
   0xcd, 0x0c, 0x13, 0xec,
   0x5f, 0x97, 0x44, 0x17,
   0xc4, 0xa7, 0x7e, 0x3d,
   0x64, 0x5d, 0x19, 0x73,
   0x60, 0x81, 0x4f, 0xdc,
   0x22, 0x2a, 0x90, 0x88,
   0x46, 0xee, 0xb8, 0x14,
   0xde, 0x5e, 0x0b, 0xdb,
   0xe0, 0x32, 0x3a, 0x0a,
   0x49, 0x06, 0x24, 0x5c,
   0xc2, 0xd3, 0xac, 0x62,
   0x91, 0x95, 0xe4, 0x79,
   0xe7, 0xc8, 0x37, 0x6d,
   0x8d, 0xd5, 0x4e, 0xa9,
   0x6c, 0x56, 0xf4, 0xea,
   0x65, 0x7a, 0xae, 0x08,
   0xba, 0x78, 0x25, 0x2e,
   0x1c, 0xa6, 0xb4, 0xc6,
   0xe8, 0xdd, 0x74, 0x1f,
   0x4b, 0xbd, 0x8b, 0x8a,
   0x70, 0x3e, 0xb5, 0x66,
   0x48, 0x03, 0xf6, 0x0e,
   0x61, 0x35, 0x57, 0xb9,
   0x86, 0xc1, 0x1d, 0x9e,
   0xe1, 0xf8, 0x98, 0x11,
   0x69, 0xd9, 0x8e, 0x94,
   0x9b, 0x1e, 0x87, 0xe9,
   0xce, 0x55, 0x28, 0xdf,
   0x8c, 0xa1, 0x89, 0x0d,
   0xbf, 0xe6, 0x42, 0x68,
   0x41, 0x99, 0x2d, 0x0f,
   0xb0, 0x54, 0xbb, 0x16,
};

/* table to use for the last round of decryption */
static const uint8_t SINVBOX[256] =
{
   0x52, 0x09, 0x6a, 0xd5,
   0x30, 0x36, 0xa5, 0x38,
   0xbf, 0x40, 0xa3, 0x9e,
   0x81, 0xf3, 0xd7, 0xfb,
   0x7c, 0xe3, 0x39, 0x82,
   0x9b, 0x2f, 0xff, 0x87,
   0x34, 0x8e, 0x43, 0x44,
   0xc4, 0xde, 0xe9, 0xcb,
   0x54, 0x7b, 0x94, 0x32,
   0xa6, 0xc2, 0x23, 0x3d,
   0xee, 0x4c, 0x95, 0x0b,
   0x42, 0xfa, 0xc3, 0x4e,
   0x08, 0x2e, 0xa1, 0x66,
   0x28, 0xd9, 0x24, 0xb2,
   0x76, 0x5b, 0xa2, 0x49,
   0x6d, 0x8b, 0xd1, 0x25,
   0x72, 0xf8, 0xf6, 0x64,
   0x86, 0x68, 0x98, 0x16,
   0xd4, 0xa4, 0x5c, 0xcc,
   0x5d, 0x65, 0xb6, 0x92,
   0x6c, 0x70, 0x48, 0x50,
   0xfd, 0xed, 0xb9, 0xda,
   0x5e, 0x15, 0x46, 0x57,
   0xa7, 0x8d, 0x9d, 0x84,
   0x90, 0xd8, 0xab, 0x00,
   0x8c, 0xbc, 0xd3, 0x0a,
   0xf7, 0xe4, 0x58, 0x05,
   0xb8, 0xb3, 0x45, 0x06,
   0xd0, 0x2c, 0x1e, 0x8f,
   0xca, 0x3f, 0x0f, 0x02,
   0xc1, 0xaf, 0xbd, 0x03,
   0x01, 0x13, 0x8a, 0x6b,
   0x3a, 0x91, 0x11, 0x41,
   0x4f, 0x67, 0xdc, 0xea,
   0x97, 0xf2, 0xcf, 0xce,
   0xf0, 0xb4, 0xe6, 0x73,
   0x96, 0xac, 0x74, 0x22,
   0xe7, 0xad, 0x35, 0x85,
   0xe2, 0xf9, 0x37, 0xe8,
   0x1c, 0x75, 0xdf, 0x6e,
   0x47, 0xf1, 0x1a, 0x71,
   0x1d, 0x29, 0xc5, 0x89,
   0x6f, 0xb7, 0x62, 0x0e,
   0xaa, 0x18, 0xbe, 0x1b,
   0xfc, 0x56, 0x3e, 0x4b,
   0xc6, 0xd2, 0x79, 0x20,
   0x9a, 0xdb, 0xc0, 0xfe,
   0x78, 0xcd, 0x5a, 0xf4,
   0x1f, 0xdd, 0xa8, 0x33,
   0x88, 0x07, 0xc7, 0x31,
   0xb1, 0x12, 0x10, 0x59,
   0x27, 0x80, 0xec, 0x5f,
   0x60, 0x51, 0x7f, 0xa9,
   0x19, 0xb5, 0x4a, 0x0d,
   0x2d, 0xe5, 0x7a, 0x9f,
   0x93, 0xc9, 0x9c, 0xef,
   0xa0, 0xe0, 0x3b, 0x4d,
   0xae, 0x2a, 0xf5, 0xb0,
   0xc8, 0xeb, 0xbb, 0x3c,
   0x83, 0x53, 0x99, 0x61,
   0x17, 0x2b, 0x04, 0x7e,
   0xba, 0x77, 0xd6, 0x26,
   0xe1, 0x69, 0x14, 0x63,
   0x55, 0x21, 0x0c, 0x7d,
};


/* round key constants */
static const uint32_t RCON[] =
{
   0x01000000 /* x^0, 0, 0, 0 */,
   0x02000000 /* x^1, 0, 0, 0 */,
   0x04000000,
   0x08000000,
   0x10000000,
   0x20000000,
   0x40000000,
   0x80000000,
   0x1B000000, /* x^8, 0, 0, 0 mod x^8 + x^4 + x^3 + x + 1*/
   0x36000000,
};

static const uint8_t sCipherKey[16] =
{
   0x6c,0x3e,0xa0,0x47,
   0x76,0x30,0xce,0x21,
   0xa2,0xce,0x33,0x4a,
   0xa7,0x46,0xc2,0xcd,
};

/* ---------------------------------------------------- */
/*
 * Sets up the key schedule for AES.
 *
 * @param cipherKey the key to expand. Only supports 128, 192 or
 * 256 bit keys.
 *
 * @param Nr the number of rounds to use in the AES algorithms.
 * Call SSrvCryptoImplAesGetRounds() to retrieve the correct value.
 *
 * @param keySchedule the computed key schedule used internally by
 * AES. This table must be allocated with at least 4 * (Nr + 1)
 * 32-bit words
 */
static void SSrvCryptoImplAesKeyExpansion(
                                   IN const uint8_t cipherKey[/* 128, 192 or 256 bits */],
                                   IN uint8_t Nr,
                                   OUT uint32_t keySchedule[/*4*(Nr + 1)*/])
{
   uint32_t temp;
   uint8_t Nk, i;

   /* Nk = key length in words */
   Nk = 4;
   switch(Nr)
   {
   case 10:
     Nk = 4; break;
   case 12:
     Nk = 6; break;
   case 14:
     Nk = 8; break;
   default:
     /* should not occur */
     SAssert(false);
   }

   /* the first Nk words are filled with the cipher key */
   for (i=0; i<Nk; i++)
   {
      SSRV_CRYPTO_MACRO_LOAD32B(keySchedule[i], cipherKey + (4*i)  );
   }

   i = Nk;
   while (i < (4 * (Nr + 1)))
   {
      temp = keySchedule[i-1];

      if ((i % Nk) == 0)
      {
         /* SubWord(RotWord(temp)) XOR Rcon[i/Nk]
          * where SubWord applies the Sbox
          * and RotWord performs a cyclic permutation to the left
          */
         temp = (SBOX[ (temp >> 16 /* get second byte */) & 0xff ] << 24)
              ^ (SBOX[ (temp >> 8 /* get third byte */) & 0xff ] << 16)
              ^ (SBOX[ temp & 0xff /* get fourth byte */] << 8)
              ^ (SBOX[ (temp >> 24 /* get first byte */) & 0xff ])
              ^ RCON[(i/Nk)-1];

      }
      else if ((Nk > 6) && ((i % Nk) == 4))
      {
         /* SubWord(temp) */
         temp = (SBOX[ (temp >> 24) & 0xff ] << 24)
              ^ (SBOX[ (temp >> 16) & 0xff ] << 16)
              ^ (SBOX[ (temp >> 8)  & 0xff ] << 8)
              ^ (SBOX[ temp         & 0xff ]);
      }
      keySchedule[i] = keySchedule[i-Nk] ^ temp;
      i++;
   }
}

/* ---------------------------------------------------- */
/**
 * This function sets up the key schedule for
 * use with the equivalent inverse cipher (see
 * FIPS 197 section 5.3.5)
 *
 * @param cipherKey the key to expand. Only supports 128, 192 or
 * 256 bit keys.
 *
 * @param Nr the number of rounds to use in the AES algorithms.
 * Call SSrvCryptoImplAesGetRounds() to retrieve the correct value.
 *
 * @param keySchedule the computed key schedule used internally by
 * AES. This table must be allocated with at least 4 * (Nr + 1)
 * 32-bit words
 *
 */
static void SSrvCryptoImplAesInvKeyExpansion(
                                      IN const uint8_t cipherKey[/* 128, 192 or 256 bits */],
                                      IN uint8_t Nr,
                                      OUT uint32_t keySchedule[/*4*(Nr + 1)*/])
{
   uint8_t i;

   SSrvCryptoImplAesKeyExpansion(cipherKey, Nr, keySchedule);


   for (i=4 ; i < 4 * Nr; i++)
   {
      /* InvMixColumns all keys except first and last one
       * Instead of coding .[0e, 09, 0d, 0b]
       * we rather use existing tables:
       * SboxInv(Sbox(a)).[0e, 09, 0d, 0b]
       */
      keySchedule[i] =           TD[ SBOX[ (keySchedule[i] >> 24)        ]]
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ SBOX[ (keySchedule[i] >> 16) & 0xff ]], 8)
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ SBOX[ (keySchedule[i] >> 8 ) & 0xff ]], 16)
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ SBOX[ (keySchedule[i]      ) & 0xff ]], 24);
   }
}

/* ---------------------------------------------------- */
/**
 * This is the implementation of the Equivalent Inverse
 * Cipher (FIPS 197, section 5.3.5). It uses
 * the same sequence of operations as for the Cipher
 * but must use a modified key schedule.
 *
 * @param keySchedule special key schedule setup for
 * use with equivalent inverse cipher.
 * Round Key 0 - Round Key 1 - Round Key 2 ... - Round Key Nr
 *
 * @param Nr number of rounds
 *
 * @param in the encrypted text. Its size must be a block
 * size.
 *
 * @param out the cleartext (result). A block's size.
 */
static void SSrvCryptoImplAesInvCipher(
                                IN uint32_t keySchedule[/* 4*(Nr + 1) */],
                                IN uint8_t Nr, /* number of rounds */
                                IN const uint8_t in[16 /* 4 * Nb */],   /* encrypted text */
                                OUT uint8_t out[16 /* 4 * Nb */])
{
   uint32_t state0, state1, state2, state3;
   uint32_t temp0, temp1, temp2, temp3;
   uint32_t *roundKey;
   uint8_t round;

   /* state = in */
   SSRV_CRYPTO_MACRO_LOAD32B(state0, in);
   SSRV_CRYPTO_MACRO_LOAD32B(state1, in+4);
   SSRV_CRYPTO_MACRO_LOAD32B(state2, in+8);
   SSRV_CRYPTO_MACRO_LOAD32B(state3, in+12);

   /* add last round key */
   state0 ^=  keySchedule[Nr * 4];
   state1 ^=  keySchedule[(Nr * 4) + 1];
   state2 ^=  keySchedule[(Nr * 4) + 2];
   state3 ^=  keySchedule[(Nr * 4) + 3];

   /* all rounds are similar, except the last one */
   for (round = (Nr - 1); round > 0; round--)
   {
      roundKey = &keySchedule[round * 4];

      /* ej = kj XOR T'0[a0,j]
       *         XOR rotation of one byte(T'0[a1,j-1 mod 4])
       *         XOR rotation of two bytes(T'0[a2,j-2 mod 4])
       *         XOR rotation of three bytes(T'0[a3,j-3 mod 4])
       */
      temp0 = roundKey[0]      ^ TD[ (state0 >> 24) & 0xff ]
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state3 >> 16) & 0xff ], 8)
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state2 >> 8) & 0xff ], 16)
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state1) & 0xff ], 24);

      temp1 = roundKey[1]      ^ TD[ (state1 >> 24) & 0xff ]
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state0 >> 16) & 0xff ], 8)
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state3 >> 8) & 0xff ], 16)
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state2) & 0xff ], 24);

      temp2 = roundKey[2]      ^ TD[ (state2 >> 24) & 0xff ]
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state1 >> 16) & 0xff ], 8)
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state0 >> 8) & 0xff ], 16)
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state3) & 0xff ], 24);

      temp3 = roundKey[3]      ^ TD[ (state3 >> 24) & 0xff ]
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state2 >> 16) & 0xff ], 8)
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state1 >> 8) & 0xff ], 16)
         ^ SSRV_CRYPTO_MACRO_ROR(TD[ (state0) & 0xff ], 24);

      state0 = temp0;
      state1 = temp1;
      state2 = temp2;
      state3 = temp3;
   }

   /* last round - uses round key 0
    * ej = kj XOR InvSbox[a0,j].[1, 1, 1, 1]
    *         XOR InvSbox[a1,j-1 mod 4]   .[1, 1, 1, 1]
    *         XOR InvSbox[a2,j-2 mod 4]   .[1, 1, 1, 1]
    *         XOR InvSbox[a3,j-3 mod 4]   .[1, 1, 1, 1]
    */
   temp0 = keySchedule[0] ^ (SINVBOX[ (state0 >> 24) & 0xff ] << 24)
                          ^ (SINVBOX[ (state3 >> 16) & 0xff ] << 16)
                          ^ (SINVBOX[ (state2 >> 8) & 0xff ] << 8)
                          ^ (SINVBOX[ (state1) & 0xff ] );

   temp1 = keySchedule[1] ^ (SINVBOX[ (state1 >> 24) & 0xff ] << 24)
                          ^ (SINVBOX[ (state0 >> 16) & 0xff ] << 16)
                          ^ (SINVBOX[ (state3 >> 8) & 0xff ] << 8)
                          ^ (SINVBOX[ (state2) & 0xff ]);

   temp2 = keySchedule[2] ^ (SINVBOX[ (state2 >> 24) & 0xff ] << 24)
                          ^ (SINVBOX[ (state1 >> 16) & 0xff ] << 16)
                          ^ (SINVBOX[ (state0 >> 8) & 0xff ] << 8)
                          ^ (SINVBOX[ (state3) & 0xff ]);

   temp3 = keySchedule[3] ^ (SINVBOX[ (state3 >> 24) & 0xff ] << 24)
                          ^ (SINVBOX[ (state2 >> 16) & 0xff ] << 16)
                          ^ (SINVBOX[ (state1 >> 8) & 0xff ] << 8)
                          ^ (SINVBOX[ (state0) & 0xff ]) ;

   SSRV_CRYPTO_MACRO_STORE32B(temp0, out);
   SSRV_CRYPTO_MACRO_STORE32B(temp1, out+4);
   SSRV_CRYPTO_MACRO_STORE32B(temp2, out+8);
   SSRV_CRYPTO_MACRO_STORE32B(temp3, out+12);
}

static void doCBC(const uint8_t* pInput, uint8_t* pOutput,uint8_t* pPreviousC,uint32_t* sKeySchedule,uint32_t nSize)
{
   uint8_t block[SSRV_CRYPTO_IMPL_MAX_BLOCK_CIPHER_SIZE];

   uint32_t i;
   uint32_t j;
   uint32_t numBlocks=nSize >> 4;

   SLogTrace("aes Decrypt(): numBlocks=%d", numBlocks);

   for (i = 0; i<numBlocks; i++)
   {
      /* computes D(Ci) and store it to the temporary block */
      SSrvCryptoImplAesInvCipher(sKeySchedule,0x0a,pInput,block);

      /* computes D(Ci) XOR Ci-1 in place */
      for (j = 0; j<SSRV_CRYPTO_IMPL_MAX_BLOCK_CIPHER_SIZE; j++)
      {
         block[j] ^= pPreviousC[j];
      }

      /* Store the Ci in the IV. Note: we cannot merely reuse the plaintext buffer
         because maybe pInput == pOutput */
      SMemMove(pPreviousC, pInput, SSRV_CRYPTO_IMPL_MAX_BLOCK_CIPHER_SIZE);

      /* Store the plaintext in the destination buffer */
      SMemMove(pOutput, block, SSRV_CRYPTO_IMPL_MAX_BLOCK_CIPHER_SIZE);

      pInput += SSRV_CRYPTO_IMPL_MAX_BLOCK_CIPHER_SIZE;
      pOutput += SSRV_CRYPTO_IMPL_MAX_BLOCK_CIPHER_SIZE;
   }
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
   _SLogTrace("SRVXCreate");

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
   _SLogTrace("SRVXDestroy");

   return;
}

/**
 *  Function SRVXOpenClientSession:
 *  Description:
 *        The system calls the function SSPIOpenClientSession when a new client
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
   uint8_t* pInput;
   uint32_t nInputSize;
   uint8_t* pOutput;
   uint32_t nOutputSize;

    S_VAR_NOT_USED(pSessionContext);
    
   _SLogTrace("SRVXInvokeCommand");

   switch(nCommandID)
   {
   case CMD_AES:
      if ( (S_PARAM_TYPE_GET(nParamTypes, 0) != S_PARAM_TYPE_MEMREF_INPUT)
         ||(S_PARAM_TYPE_GET(nParamTypes, 1) != S_PARAM_TYPE_MEMREF_OUTPUT))
      {
         return S_ERROR_BAD_PARAMETERS;
      }
      /* AFY: check for NULL */
      pInput      = pParams[0].memref.pBuffer;
      nInputSize  = pParams[0].memref.nSize;
      pOutput     = pParams[1].memref.pBuffer;
      nOutputSize = pParams[1].memref.nSize;
      {
         uint32_t sKeySchedule[44];
         uint8_t pPreviousC[SSRV_CRYPTO_IMPL_MAX_BLOCK_CIPHER_SIZE];
         uint32_t nBlock=nInputSize>>8;

         SSrvCryptoImplAesInvKeyExpansion(sCipherKey,0x0a,sKeySchedule);
         SMemFill(pPreviousC,0,SSRV_CRYPTO_IMPL_MAX_BLOCK_CIPHER_SIZE);
         while (nBlock!=0)
         {
            doCBC(pInput,pOutput,pPreviousC,sKeySchedule,256);
            pInput+=256;
            pOutput+=256;
            nBlock--;
         }
      }
      return S_SUCCESS;

   default:
      SLogError("invalid command ID: 0x%X", nCommandID);
      return S_ERROR_BAD_PARAMETERS;
   }
}
