/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "NVCryptoPlugin"

#include <utils/Log.h>
#include <media/stagefright/MediaErrors.h>

#include "NvCryptoPlugin.h"

android::CryptoFactory *createCryptoFactory() {
    ALOGV("\n%s[%d]", __FUNCTION__, __LINE__);
    return new android::NvCryptoFactory;
}

namespace android {

// static
const uint8_t NvCryptoFactory::mNvCryptoUUID[16] = {
    0x31, 0x70, 0x8f, 0x40, 0xe5, 0x20, 0x11, 0xe1,
    0x87, 0x91, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b
};

status_t NvCryptoFactory::createPlugin(
        const uint8_t uuid[16],
        const void *data,
        size_t size,
        CryptoPlugin **plugin) {

    ALOGV("\n%s[%d]",__FUNCTION__, __LINE__);

    if (memcmp(uuid, mNvCryptoUUID, 16)) {
        return -ENOENT;
    }

    *plugin = new NvCryptoPlugin(data, size);

    if (*plugin == NULL)
        return -ENOMEM;

    return OK;
}

NvCryptoPlugin::NvCryptoPlugin(const void *pKey, size_t keySize) {

    // Key should be of 128 byte
    if (pKey == NULL || keySize != 128)
        return;

    pExtraDataFormat  = (NvExtraDataFormat *)malloc(sizeof(NvExtraDataFormat));

    if (pExtraDataFormat) {
        memset(pExtraDataFormat, 0, sizeof(NvExtraDataFormat));
        pExtraDataFormat->size[0] = 0x30;
        pExtraDataFormat->version[0] = 0x01;
        pExtraDataFormat->eType[0] = 0x04;
        pExtraDataFormat->eType[1] = 0x00;
        pExtraDataFormat->eType[2] = 0xe0;
        pExtraDataFormat->eType[3] = 0x7f;
        pExtraDataFormat->dataSize[0] = 0x19;
    }

#ifdef SECUREOS
    ALOGV("\n%s[%d] Secure OS is defined", __FUNCTION__, __LINE__);

    TEEC_Result nTeeError;
    const TEEC_UUID *pServiceUUID;
    const TEEC_UUID SERVICE_UUID = SERVICE_RSA_UUID;

    nTeeError = TEEC_InitializeContext(NULL, &mSecureContext);
    ALOGV("TEEC_INIT = %x\n", nTeeError);
    if (nTeeError != TEEC_SUCCESS) {
        return;
    }

    pServiceUUID = &SERVICE_UUID;
    mpSession = (TEEC_Session *)malloc(sizeof(TEEC_Session));
    if (mpSession == NULL) {
        ALOGV("TEEC_Session Memory allocation failed");
        return;
    }

    mSecureOperation.paramTypes = TEEC_PARAM_TYPES(
        TEEC_NONE,
        TEEC_NONE,
        TEEC_NONE,
        TEEC_NONE);

    nTeeError = TEEC_OpenSession(
        &mSecureContext,
        mpSession,                 /* OUT session */
        pServiceUUID,              /* destination UUID */
        TEEC_LOGIN_PUBLIC,         /* connectionMethod */
        NULL,                      /* connectionData */
        &mSecureOperation,         /* IN OUT operation */
        NULL);                     /* OUT returnOrigin, optional */

    ALOGV("TEEC_OPEN = %x\n", nTeeError);
    if (nTeeError != TEEC_SUCCESS) {
        free(mpSession);
        mpSession = NULL;
        return;
    }

    mSecureOperation.paramTypes = TEEC_PARAM_TYPES(
        TEEC_MEMREF_TEMP_INPUT,
        TEEC_NONE,
        TEEC_NONE,
        TEEC_NONE);
    mSecureOperation.params[0].tmpref.buffer = (char *)pKey;
    mSecureOperation.params[0].tmpref.size = (int)keySize;

    nTeeError = TEEC_InvokeCommand(
        mpSession,
        RSA_SET_AES_KEY,
        &mSecureOperation,       /* IN OUT operation */
        NULL);                   /* OUT returnOrigin, optional */

    ALOGV("TEEC_SETKEY = %x\n", nTeeError);
    if (nTeeError != TEEC_SUCCESS) {
        TEEC_CloseSession(mpSession);
        free(mpSession);
        mpSession = NULL;
        return;
    }
#else
    ALOGV("\n%s[%d]Secure OS is not defined", __FUNCTION__, __LINE__);
#endif
}

NvCryptoPlugin::~NvCryptoPlugin() {
#ifdef SECUREOS
    if (mpSession != NULL) {
        TEEC_CloseSession(mpSession);
        free(mpSession);
        mpSession = NULL;
        TEEC_FinalizeContext(&mSecureContext);
    }
#endif
    if (pExtraDataFormat)
        free(pExtraDataFormat);
}

bool NvCryptoPlugin::requiresSecureDecoderComponent(const char *mime) const {
    return false;
}

//Error Handling is pending like exception and null checks
ssize_t NvCryptoPlugin::decrypt(
        bool secure,
        const uint8_t key[16],
        const uint8_t iv[16],
        Mode mode,
        const void *srcPtr,
        const SubSample *subSamples,
        size_t numSubSamples,
        void *dstPtr,
        AString *errorDetailMsg) {

    ALOGV("\n%s[%d] numSubSamples:%d", __FUNCTION__, __LINE__, numSubSamples);

    size_t dataSize = 0;
    size_t i = 0;
    size_t prvSubSampleEnd = 0;
    bool containsEncrypted = false;

    if (numSubSamples > 1) {
        for (i = 0; i < numSubSamples - 1; i++) {
            dataSize += subSamples[i].mNumBytesOfClearData +
                subSamples[i].mNumBytesOfEncryptedData;
            // Check if any of subsample has encrypted data
            if (subSamples[i].mNumBytesOfEncryptedData)
                containsEncrypted = true;
        }
        // align encoded size to 4-byte boundary to put extradata
        dataSize = (dataSize + 3) & ~3;
    } else {
        dataSize += subSamples[0].mNumBytesOfClearData +
            subSamples[0].mNumBytesOfEncryptedData;
    }

    // srcPtr will always be greater than dataSize as allocated inside ICrypto
    memcpy(dstPtr, srcPtr, dataSize);

#ifdef SECUREOS
    if (containsEncrypted) {
        if (mpSession == NULL) {
            return -ENODEV;
        }

        uint8_t *pData = (uint8_t *)dstPtr + dataSize;

        for (i = 0; i < (numSubSamples - 1); i++) {
            if (subSamples[i].mNumBytesOfEncryptedData > 0) {
                pExtraDataFormat->encryptionSize =
                    subSamples[i].mNumBytesOfEncryptedData;
                pExtraDataFormat->encryptionOffset = prvSubSampleEnd +
                    subSamples[i].mNumBytesOfClearData;

                memcpy(pExtraDataFormat->IVdata, iv, 16);
                memcpy(pData, pExtraDataFormat, sizeof(NvExtraDataFormat));
                pData += sizeof(NvExtraDataFormat);
                prvSubSampleEnd += subSamples[i].mNumBytesOfClearData +
                    subSamples[i].mNumBytesOfEncryptedData;
            }
        }
    }
#endif

    return dataSize;
}

}  // namespace android

