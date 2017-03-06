/*
* Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property
* and proprietary rights in and to this software and related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA Corporation is strictly prohibited.
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tee_client_api.h"
#include "oemcrypto_stub.h"
#include "oemcrypto_secure_protocol.h"
#include "OEMCrypto.h"
#include "otf_secure_protocol.h"
#include "nvavp.h"

static const TEEC_UUID SERVICE_UUID = SERVICE_OEMCRYPTO_UUID;

/* There is one global device context */
static TEEC_Context g_sContext;
static bool g_bInitialized = false;
TEEC_Session *OEMCryptoKeySession = NULL;

static OEMCryptoResult static_convertErrorCode(TEEC_Result nError)
{
    switch (nError) {
    case TEEC_SUCCESS:
        return OEMCrypto_SUCCESS;
    case TEEC_ERROR_BAD_PARAMETERS:
        return OEMCRYPTO_ERROR_ILLEGAL_ARGUMENT;
    case TEEC_ERROR_BAD_STATE:
        return OEMCRYPTO_ERROR_ILLEGAL_STATE;
    case TEEC_ERROR_OUT_OF_MEMORY:
        return OEMCRYPTO_ERROR_OUT_OF_MEMORY;
    case OEMCRYTO_ERROR_INIT_FAILED:
        return OEMCrypto_ERROR_INIT_FAILED;
    case OEMCRYTO_ERROR_TERMINATE_FAILED:
        return OEMCrypto_ERROR_TERMINATE_FAILED;
    case OEMCRYTO_ERROR_ENTER_SECURE_PLAYBACK_FAILED:
        return OEMCrypto_ERROR_ENTER_SECURE_PLAYBACK_FAILED;
    case OEMCRYTO_ERROR_EXIT_SECURE_PLAYBACK_FAILED:
        return OEMCrypto_ERROR_EXIT_SECURE_PLAYBACK_FAILED;
    case OEMCRYTO_ERROR_SHORT_BUFFER:
        return OEMCrypto_ERROR_SHORT_BUFFER;
    case OEMCRYTO_ERROR_NO_DEVICE_KEY:
        return OEMCrypto_ERROR_NO_DEVICE_KEY;
    case OEMCRYTO_ERROR_NO_ASSET_KEY:
        return OEMCrypto_ERROR_NO_ASSET_KEY;
    case OEMCRYTO_ERROR_KEYBOX_INVALID:
        return OEMCrypto_ERROR_KEYBOX_INVALID;
    case OEMCRYTO_ERROR_NO_KEYDATA:
        return OEMCrypto_ERROR_NO_KEYDATA;
    case OEMCRYTO_ERROR_NO_CW:
        return OEMCrypto_ERROR_NO_CW;
    case OEMCRYTO_ERROR_DECRYPT_FAILED:
        return OEMCrypto_ERROR_DECRYPT_FAILED;
    case OEMCRYTO_ERROR_WRITE_KEYBOX:
        return OEMCrypto_ERROR_WRITE_KEYBOX;
    case OEMCRYTO_ERROR_WRAP_KEYBOX:
        return OEMCrypto_ERROR_WRAP_KEYBOX;
    case OEMCRYTO_ERROR_BAD_MAGIC:
        return OEMCrypto_ERROR_BAD_MAGIC;
    case OEMCRYTO_ERROR_BAD_CRC:
        return OEMCrypto_ERROR_BAD_CRC;
    case OEMCRYTO_ERROR_NO_DEVICEID:
        return OEMCrypto_ERROR_NO_DEVICEID;
    case OEMCRYTO_ERROR_RNG_FAILED:
        return OEMCrypto_ERROR_RNG_FAILED;
    case OEMCRYTO_ERROR_RNG_NOT_SUPPORTED:
        return OEMCrypto_ERROR_RNG_NOT_SUPPORTED;
    case OEMCRYTO_ERROR_SETUP:
        return OEMCrypto_ERROR_SETUP;
    default:
        return OEMCRYPTO_ERROR_GENERIC;
    }
}

OEMCryptoResult OEMCrypto_Initialize(void)
{
    TEEC_Result nTeeError;
    TEEC_Operation sOperation;
    const TEEC_UUID *pServiceUUID;
    TEEC_Session *session;

    NvOsDebugPrintf( ">> init()" );

    if (g_bInitialized == true) {
        NvOsDebugPrintf( "<< init() already initialized" );
        return static_convertErrorCode(TEEC_SUCCESS);
    }

    nTeeError = TEEC_InitializeContext(NULL, &g_sContext);
    if (nTeeError != TEEC_SUCCESS) {
        NvOsDebugPrintf( "<< init() cannot init TEEC context" );
        return OEMCrypto_ERROR_INIT_FAILED;
    }

    pServiceUUID = &SERVICE_UUID;

    session = (TEEC_Session *) malloc(sizeof(TEEC_Session));
    if (session == NULL) {
        NvOsDebugPrintf( "<< init() not enough mem" );
        return OEMCRYPTO_ERROR_OUT_OF_MEMORY;
    }

    sOperation.paramTypes =
    TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

    nTeeError = TEEC_OpenSession(&g_sContext, session,  /* OUT session */
                                 pServiceUUID,  /* destination UUID */
                                 TEEC_LOGIN_PUBLIC, /* connectionMethod */
                                 NULL,  /* connectionData */
                                 &sOperation,   /* IN OUT operation */
                                 NULL   /* OUT returnOrigin, optional */
                                );
    if (nTeeError == TEEC_SUCCESS) {
        OEMCryptoKeySession = session;
        g_bInitialized = true;
    } else {
        OEMCryptoKeySession = OEMCRYPTO_KEY_SESSION_HANDLE_INVALID;
        free(session);
    }

    NvOsDebugPrintf( "<< init()" );

    return static_convertErrorCode(nTeeError);
}

OEMCryptoResult OEMCrypto_Terminate(void)
{
    TEEC_Session *session;
    TEEC_Result nTeeError = TEEC_SUCCESS;
    NvAvpHandle hAvp;

    NvOsDebugPrintf( ">> term()" );

    /* Check OEMCrypto Agent is initialized */
    if (!g_bInitialized) {
        NvOsDebugPrintf( "<< term() never initialized" );
        return OEMCrypto_ERROR_TERMINATE_FAILED;
    }

    if (OEMCryptoKeySession == OEMCRYPTO_KEY_SESSION_HANDLE_INVALID) {
        NvOsDebugPrintf( "<< term() invalid handle" );
        return OEMCrypto_ERROR_TERMINATE_FAILED;
    }

    if (NvSuccess != NvAvpOpen(&hAvp))
    {
        NvOsDebugPrintf( "<< term() cannot open nvavp" );
        return OEMCrypto_ERROR_TERMINATE_FAILED;
    }

    NvOsDebugPrintf( "!! term() nvavp opened" );

    if (NvSuccess != NvAvpForceClockStayOn(hAvp, NVAVP_CLIENT_CLOCK_STAY_ON_ENABLED))
    {
        NvOsDebugPrintf( "<< term() stay-on on request has failed" );
        nTeeError = OEMCrypto_ERROR_TERMINATE_FAILED;
        goto error_close_avp;
    }
    session = OEMCryptoKeySession;
    TEEC_CloseSession(session);
    free(session);
    OEMCryptoKeySession = OEMCRYPTO_KEY_SESSION_HANDLE_INVALID;

    TEEC_FinalizeContext(&g_sContext);
    g_bInitialized = false;

    if (NvSuccess != NvAvpForceClockStayOn(hAvp, NVAVP_CLIENT_CLOCK_STAY_ON_DISABLED))
    {
        NvOsDebugPrintf( "<< term() stay-on off request has failed" );
        nTeeError = OEMCrypto_ERROR_TERMINATE_FAILED;
        goto error_close_avp;
    }

    error_close_avp:
    NvAvpClose(hAvp);

    NvOsDebugPrintf( "<< term()" );
    return nTeeError;
}

OEMCryptoResult OEMCrypto_EnterSecurePlayback(void)
{
    return TEEC_SUCCESS;
}

OEMCryptoResult OEMCrypto_ExitSecurePlayback(void)
{
    return TEEC_SUCCESS;
}

OEMCryptoResult OEMCrypto_SetEntitlementKey(const OEMCrypto_UINT8 * emmKey,
                                            const OEMCrypto_UINT32 emmKeyLength)
{
    TEEC_Result nTeeError;
    TEEC_Operation sOperation;
    TEEC_Session *session;

    NvOsDebugPrintf( ">> set_emm() emmKeyLength=%d\n",emmKeyLength );

    /* Check  OEMCrypto agent is initialized */
    if (!g_bInitialized) {
        NvOsDebugPrintf( "<< set_emm() never initialized" );
        return static_convertErrorCode(TEEC_ERROR_BAD_STATE);
    }

    /* Check OEMCryptoKeySession is valid */
    if (OEMCryptoKeySession == OEMCRYPTO_KEY_SESSION_HANDLE_INVALID) {
        NvOsDebugPrintf( "<< set_emm() invalid handle" );
        return static_convertErrorCode(TEEC_ERROR_BAD_STATE);
    }
    session = OEMCryptoKeySession;

    sOperation.paramTypes =
    TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE,
                     TEEC_NONE);
    sOperation.params[0].tmpref.buffer = emmKey;
    sOperation.params[0].tmpref.size = emmKeyLength;

    nTeeError = TEEC_InvokeCommand(session, OEMCRYPTO_SET_ENTITLEMENT_KEY, &sOperation, /* IN OUT operation */
                                   NULL /* OUT returnOrigin, optional */
                                  );

    NvOsDebugPrintf( "<< set_emm()" );
    return static_convertErrorCode(nTeeError);
}

OEMCryptoResult OEMCrypto_DeriveControlWord(const OEMCrypto_UINT8 * ecm,
                                            const OEMCrypto_UINT32 length,
                                            OEMCrypto_UINT32 * flags)
{
    TEEC_Result nTeeError;
    TEEC_Operation sOperation;
    TEEC_Session *session;

    NvOsDebugPrintf( ">> deriveCW() length=%d\n",length );

    // Sleep 200ms
    usleep(2000*1000);

    session = OEMCryptoKeySession;
    if (session == NULL) {
        NvOsDebugPrintf( "<< deriveCW() invalid session" );
        return static_convertErrorCode(TEEC_ERROR_BAD_STATE);
    }

    sOperation.paramTypes =
    TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_VALUE_OUTPUT,
                     TEEC_NONE, TEEC_NONE);
    sOperation.params[0].tmpref.buffer = ecm;
    sOperation.params[0].tmpref.size = length;

    nTeeError = TEEC_InvokeCommand(session, OEMCRYPTO_DERIVE_CONTROL_WORD, &sOperation, /* IN OUT operation */
                                   NULL /* OUT returnOrigin, optional */
                                  );

    if (nTeeError != TEEC_SUCCESS) {
        NvOsDebugPrintf( "<< deriveCW() error in invoke command" );
        return static_convertErrorCode(nTeeError);
    }

    *flags = sOperation.params[1].value.a;

    NvOsDebugPrintf( "<< deriveCW()" );
    return static_convertErrorCode(nTeeError);
}

OEMCryptoResult
OEMCrypto_DecryptVideo(const OEMCrypto_UINT8* iv,
                       const OEMCrypto_UINT8* input,
                       const OEMCrypto_UINT32 inputLength,
                       OEMCrypto_UINT32 outputHandle,
                       OEMCrypto_UINT32 outputOffset,
                       OEMCrypto_UINT32 *outputLength)
{
    TEEC_Result                             nTeeError;
    TEEC_Operation                  sOperation;
    TEEC_Session*                   session;
    NvMMAesWvMetadata*              pNvMMAesWvMetadata = NULL;

    session = OEMCryptoKeySession;

    if (!outputHandle)
        return OEMCrypto_ERROR_DECRYPT_FAILED;

    pNvMMAesWvMetadata = (NvMMAesWvMetadata*)(outputHandle);

    sOperation.paramTypes = TEEC_PARAM_TYPES(
                                              TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_INOUT, TEEC_MEMREF_TEMP_INOUT, TEEC_NONE);

    if(iv == NULL){
        sOperation.params[0].value.a = 0;
    }
    else{
        sOperation.params[0].value.a = 1;
    }

    sOperation.params[0].value.b = outputOffset;
    sOperation.params[1].tmpref.buffer = input;
    sOperation.params[1].tmpref.size = inputLength;
    sOperation.params[2].tmpref.buffer = outputHandle + outputOffset;
    sOperation.params[2].tmpref.size = inputLength + (outputOffset ? 0 : (sizeof(NvMMAesWvMetadata) + 3) & (~3) );

    nTeeError = TEEC_InvokeCommand(session,
                               OEMCRYPTO_DECRYPT_VIDEO,
                               &sOperation,   /* IN OUT operation */
                               NULL       /* OUT returnOrigin, optional */
                               );
    if (nTeeError != TEEC_SUCCESS) {
        return static_convertErrorCode(nTeeError);
    }

    *outputLength = sOperation.params[0].value.a;
    pNvMMAesWvMetadata->NonAlignedOffset = sOperation.params[0].value.b;

    return static_convertErrorCode(TEEC_SUCCESS);
}

OEMCryptoResult
OEMCrypto_DecryptAudio(const OEMCrypto_UINT8* iv,
                       const OEMCrypto_UINT8* input, const OEMCrypto_UINT32 inputLength,
                       OEMCrypto_UINT8 *output, OEMCrypto_UINT32 *outputLength)
{
    TEEC_Result                             nTeeError;
    TEEC_Operation                  sOperation;
    TEEC_Session*                   session;

    session = OEMCryptoKeySession;

    if (!iv) {
        memcpy(output, input, inputLength);
        *outputLength = inputLength;
        return OEMCrypto_SUCCESS;
    }

    sOperation.paramTypes = TEEC_PARAM_TYPES(
                                            /*iv                        input,length            output                  outputLength*/
                                            TEEC_MEMREF_TEMP_INOUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INOUT, TEEC_VALUE_INOUT);

    sOperation.params[0].tmpref.buffer = iv;
    sOperation.params[0].tmpref.size   = sizeof(iv);

    sOperation.params[1].tmpref.buffer = input;
    sOperation.params[1].tmpref.size   = inputLength;

    sOperation.params[2].tmpref.buffer = output;
    sOperation.params[2].tmpref.size   = *outputLength;

    sOperation.params[3].value.a = *outputLength;

    nTeeError = TEEC_InvokeCommand(session,
                                   OEMCRYPTO_DECRYPT_AUDIO,
                                   &sOperation,   /* IN OUT operation */
                                   NULL       /* OUT returnOrigin, optional */
                                  );
    if (nTeeError != TEEC_SUCCESS) {
        return static_convertErrorCode(nTeeError);
    }

    *outputLength = sOperation.params[3].value.a;

    return static_convertErrorCode(TEEC_SUCCESS);
}

OEMCryptoResult OEMCrypto_InstallKeybox(OEMCrypto_UINT8 * keybox,
                                        OEMCrypto_UINT32 keyBoxLength)
{
    TEEC_Result nTeeError;
    TEEC_Operation sOperation;
    TEEC_Session *session;

    session = OEMCryptoKeySession;

    sOperation.paramTypes =
    TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

    sOperation.params[0].tmpref.buffer = keybox;
    sOperation.params[0].tmpref.size  = keyBoxLength;

    nTeeError = TEEC_InvokeCommand(session, OEMCRYPTO_INSTALL_KEYBOX, &sOperation, /* IN OUT operation */
                                   NULL /* OUT returnOrigin, optional */
                                  );
    if (nTeeError != TEEC_SUCCESS) {
        return static_convertErrorCode(nTeeError);
    }

    return static_convertErrorCode(TEEC_SUCCESS);
}

OEMCryptoResult OEMCrypto_IsKeyboxValid(void)
{
    TEEC_Result nTeeError;
    TEEC_Operation sOperation;
    TEEC_Session *session;

    NvOsDebugPrintf( ">> IsKeyboxValid()" );

    session = OEMCryptoKeySession;

    sOperation.paramTypes =
    TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    nTeeError = TEEC_InvokeCommand(session, OEMCRYPTO_IS_KEYBOX_VALID, &sOperation, /* IN OUT operation */
                                   NULL /* OUT returnOrigin, optional */
                                  );
    if (nTeeError != TEEC_SUCCESS) {
        NvOsDebugPrintf( "<< IsKeyboxValid() error in invokeCMD" );
        return static_convertErrorCode(nTeeError);
    }

    NvOsDebugPrintf( "<< IsKeyboxValid()" );
    return static_convertErrorCode(TEEC_SUCCESS);
}

OEMCryptoResult OEMCrypto_GetDeviceID(OEMCrypto_UINT8 * deviceID,
                                      OEMCrypto_UINT32 * idLength)
{
    TEEC_Result nTeeError;
    TEEC_Operation sOperation;
    TEEC_Session *session;

    NvOsDebugPrintf( ">> GetDeviceID()" );

    session = OEMCryptoKeySession;

    sOperation.paramTypes =
    TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_VALUE_INOUT,
                     TEEC_NONE, TEEC_NONE);

    sOperation.params[0].tmpref.buffer = deviceID;
    sOperation.params[0].tmpref.size = *idLength;

    sOperation.params[1].value.a = *idLength;

    nTeeError = TEEC_InvokeCommand(session, OEMCRYPTO_GET_DEVICE_ID, &sOperation,   /* IN OUT operation */
                                   NULL /* OUT returnOrigin, optional */
                                  );
    if (nTeeError != TEEC_SUCCESS) {
        NvOsDebugPrintf( "<< GetDeviceID() error in invokeCMD" );
        return static_convertErrorCode(nTeeError);
    }

    *idLength = sOperation.params[1].value.a;

    NvOsDebugPrintf( "<< GetDeviceID()" );
    return static_convertErrorCode(TEEC_SUCCESS);
}

OEMCryptoResult OEMCrypto_GetKeyData(OEMCrypto_UINT8 * keyData,
                                     OEMCrypto_UINT32 * keyDataLength)
{
    TEEC_Result nTeeError;
    TEEC_Operation sOperation;
    TEEC_Session *session;

    session = OEMCryptoKeySession;

    sOperation.paramTypes =
    TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_VALUE_INOUT,
                     TEEC_NONE, TEEC_NONE);

    sOperation.params[0].tmpref.buffer = keyData;
    sOperation.params[0].tmpref.size = *keyDataLength;

    sOperation.params[1].value.a = *keyDataLength;

    nTeeError = TEEC_InvokeCommand(session, OEMCRYPTO_GET_KEY_DATA, &sOperation,    /* IN OUT operation */
                                   NULL /* OUT returnOrigin, optional */
                                  );
    if (nTeeError != TEEC_SUCCESS) {
        return static_convertErrorCode(nTeeError);
    }

    *keyDataLength = sOperation.params[1].value.a;

    return static_convertErrorCode(TEEC_SUCCESS);
}

OEMCryptoResult OEMCrypto_GetRandom(OEMCrypto_UINT8 * randomData,
                                    OEMCrypto_UINT32 dataLength)
{
    TEEC_Result nTeeError;
    TEEC_Operation sOperation;
    TEEC_Session *session;

    NvOsDebugPrintf( ">> GetRDN()" );

    session = OEMCryptoKeySession;

    sOperation.paramTypes =
    TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_VALUE_INPUT,
                     TEEC_NONE, TEEC_NONE);

    sOperation.params[0].tmpref.buffer = randomData;
    sOperation.params[0].tmpref.size = dataLength;

    sOperation.params[1].value.a = dataLength;

    nTeeError = TEEC_InvokeCommand(session, OEMCRYPTO_GET_RANDOM, &sOperation,  /* IN OUT operation */
                                   NULL /* OUT returnOrigin, optional */
                                  );
    if (nTeeError != TEEC_SUCCESS) {
        NvOsDebugPrintf( "<< GetRDN() error in invokeCMD" );
        return static_convertErrorCode(nTeeError);
    }

    NvOsDebugPrintf( "<< GetRDN()" );
    return static_convertErrorCode(TEEC_SUCCESS);
}

OEMCryptoResult OEMCrypto_WrapKeybox(OEMCrypto_UINT8 * keybox,
                                     OEMCrypto_UINT32 keyBoxLength,
                                     OEMCrypto_UINT8 * wrappedKeybox,
                                     OEMCrypto_UINT32 * wrappedKeyBoxLength,
                                     OEMCrypto_UINT8 * transportKey,
                                     OEMCrypto_UINT32 transportKeyLength)
{
    return static_convertErrorCode(TEEC_SUCCESS);
}
