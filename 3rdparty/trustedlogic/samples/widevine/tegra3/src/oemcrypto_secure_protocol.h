/*
* Copyright (c) 2011-2012 NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property
* and proprietary rights in and to this software and related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA Corporation is strictly prohibited.
*/

#ifndef __OEMCRYPTO_PROTOCOL_H__
#define __OEMCRYPTO_PROTOCOL_H__

#include "nvmm_aes.h"

/** Service Identifier */
/* {13f616f9-8572-4a6f-a1f1-03aa9b05f9ff} */
#define SERVICE_OEMCRYPTO_UUID   { 0x13F616F9, 0x8572, 0x4A6F, { 0xA1, 0xF1, 0x03, 0xAA, 0x9B, 0x05, 0xF9, 0xff } }

#define OEMCRYPTO_SET_ENTITLEMENT_KEY        0x00000001
#define OEMCRYPTO_DERIVE_CONTROL_WORD        0x00000002
#define OEMCRYPTO_IS_KEYBOX_VALID        0x00000003
#define OEMCRYPTO_GET_DEVICE_ID        0x00000004
#define OEMCRYPTO_GET_KEY_DATA        0x00000005
#define OEMCRYPTO_GET_RANDOM        0x00000006
#define OEMCRYPTO_DECRYPT_AUDIO        0x00000007
#define OEMCRYPTO_DECRYPT_VIDEO        0x00000008
#define OEMCRYPTO_INSTALL_KEYBOX        0x00000009
#define OEMCRYPTO_INSTALL_CONTROL_WORD  0x0000000A

/** Service-specific error codes */
#define OEMCRYPTO_AGENT_ERROR_CRYPTO        0x00000001
#define OTFDRIVER_AGENT_ERROR_CRYPTO        0x00000002

#define OEMCRYTO_ERROR_INIT_FAILED        0x00000001
#define OEMCRYTO_ERROR_TERMINATE_FAILED        0x00000002
#define OEMCRYTO_ERROR_ENTER_SECURE_PLAYBACK_FAILED        0x00000003
#define OEMCRYTO_ERROR_EXIT_SECURE_PLAYBACK_FAILED        0x00000004
#define OEMCRYTO_ERROR_SHORT_BUFFER        0x00000005
#define OEMCRYTO_ERROR_NO_DEVICE_KEY        0x00000006
#define OEMCRYTO_ERROR_NO_ASSET_KEY        0x00000007
#define OEMCRYTO_ERROR_KEYBOX_INVALID        0x00000008
#define OEMCRYTO_ERROR_NO_KEYDATA        0x00000009
#define OEMCRYTO_ERROR_NO_CW        0x0000000A
#define OEMCRYTO_ERROR_DECRYPT_FAILED        0x0000000B
#define OEMCRYTO_ERROR_WRITE_KEYBOX        0x0000000C
#define OEMCRYTO_ERROR_WRAP_KEYBOX        0x0000000D
#define OEMCRYTO_ERROR_BAD_MAGIC        0x0000000E
#define OEMCRYTO_ERROR_BAD_CRC        0x0000000F
#define OEMCRYTO_ERROR_NO_DEVICEID        0x00000010
#define OEMCRYTO_ERROR_RNG_FAILED        0x00000011
#define OEMCRYTO_ERROR_RNG_NOT_SUPPORTED        0x00000012
#define OEMCRYTO_ERROR_SETUP        0x00000013

#endif /* __OEMCRYPTO_PROTOCOL_H__ */
