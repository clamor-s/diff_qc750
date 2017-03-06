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
#include "OEMCryptoL3.h"
#include "otf_secure_protocol.h"
#include "nvavp.h"
#include <cutils/properties.h>
#include <openssl/aes.h>
#include <fcntl.h>

#define TEST_FILE2	"/data/drm/widevine.dat"
static int mKeyFd =-1;

int keyInit(){
	if(mKeyFd>=0){
		close(mKeyFd);
	}

	mKeyFd = open(TEST_FILE2, O_RDONLY);
	if(mKeyFd<0){
		NvOsDebugPrintf("open file fail=%s",TEST_FILE2);
		return -1;
	}
	NvOsDebugPrintf("keyInit ok...");
	return mKeyFd;
}

static void encryptKey(const OEMCrypto_UINT8 * inKey, int len,OEMCrypto_UINT8 * outKey){
	unsigned char iv[16]={0};
	AES_KEY aesKey;
	unsigned char key[16]={'k','e','e','n','h','i','1','2','1','5','0','0','0','0','0','0'};

	AES_set_encrypt_key(key, 16 * 8, &aesKey) ;
	AES_cbc_encrypt(inKey, outKey, len, &aesKey, iv, 1);
}

static void decryptKey(const OEMCrypto_UINT8 * inKey, int len,OEMCrypto_UINT8 * outKey){
	unsigned char iv[16]={0};
	AES_KEY aesKey;
	unsigned char key[16]={'k','e','e','n','h','i','1','2','1','5','0','0','0','0','0','0'};

	AES_set_decrypt_key(key, 16 * 8, &aesKey) ;
	AES_cbc_encrypt(inKey, outKey, len, &aesKey, iv, 0);
}

void saveToKey(const OEMCrypto_UINT8 * key, int len){
	if(mKeyFd>=0){
		close(mKeyFd);
	}
	mKeyFd = open(TEST_FILE2, O_CREAT|O_WRONLY,0700);
	if(mKeyFd<0){
		NvOsDebugPrintf("open file2 fail ..");
		return;
	}

	if(len!=128){
		NvOsDebugPrintf("saveToKey2 key size error\n");
		return;
	}

	OEMCrypto_UINT8 newKey[128+1];

	encryptKey(key, len, newKey);
	
	write(mKeyFd,newKey,len);
	NvOsDebugPrintf("saveToKey2 ok");
}

/*
OEMCryptoResult OEMCrypto_EnterSecurePlayback(void)
{
    return TEEC_SUCCESS;
}

OEMCryptoResult OEMCrypto_ExitSecurePlayback(void)
{
    return TEEC_SUCCESS;
}
*/

void dumpKey(uint8_t* key, int len){
	//NvOsDebugPrintf("start dump key");
	//int i;
	//for(i=0;i<len;i++){
	//	NvOsDebugPrintf("%0x",key[i]);
	//}
	//NvOsDebugPrintf("end dump key");
}

OEMCryptoResult OEMCrypto_EncryptAndStoreKeybox(OEMCrypto_UINT8 * keybox,
                                     OEMCrypto_UINT32 keyBoxLength)
{
	keyInit();

	dumpKey(keybox,keyBoxLength);
	saveToKey(keybox,keyBoxLength);
	
	NvOsDebugPrintf( ">> OEMCrypto_EncryptAndStoreKeybox() keyBoxLength=%d",keyBoxLength );
    	return OEMCrypto_SUCCESS;
}

OEMCryptoResult OEMCrypto_GetRandom(OEMCrypto_UINT8 * randomData,
                                    OEMCrypto_UINT32 dataLength)
{
	//keyInit();
	NvOsDebugPrintf( ">> GetRDN() dataLength=%d",dataLength );

	int fd  =open("/dev/urandom", O_RDONLY);
	if(fd<0){
		NvOsDebugPrintf("open random fail...");
		return OEMCrypto_SUCCESS;
	}

	read(fd, randomData, dataLength);
	close(fd);

	dumpKey(randomData,dataLength);

   	NvOsDebugPrintf( "<< GetRDN()" );
    	return OEMCrypto_SUCCESS;
}

OEMCryptoResult OEMCrypto_GetKeyboxData(OEMCrypto_UINT8 *buffer,
					OEMCrypto_UINT32 ofset, OEMCrypto_UINT32 length)
{
	int fd =keyInit();

	NvOsDebugPrintf( ">> OEMCrypto_GetKeyboxData() ofset=%d,length=%d ",ofset,length );

	char srcbuffer[128];
	int l = read(fd, srcbuffer, sizeof(srcbuffer));
	if(l !=128){
		NvOsDebugPrintf("read file fail!!!\n");
		return OEMCrypto_ERROR_NO_KEYDATA;
	}

	OEMCrypto_UINT8 decryptedbuffer[128+1];
	decryptKey(srcbuffer, l ,decryptedbuffer);
	
	memcpy(buffer, &decryptedbuffer[ofset], length);

	dumpKey(buffer,length);

	NvOsDebugPrintf( "<< OEMCrypto_GetKeyboxData()" );
	return OEMCrypto_SUCCESS;
}

OEMCryptoResult OEMCrypto_IdentifyDevice(OEMCrypto_UINT8 *deviceID,
					OEMCrypto_UINT32 length)
{
	NvOsDebugPrintf( ">> OEMCrypto_IdentifyDevice(),length=%d",length );
	char value[PROPERTY_VALUE_MAX] = {0};

	property_get("ro.serialno", value, NULL);
	strcpy(deviceID,value);

	dumpKey(deviceID,length);

	NvOsDebugPrintf( "<< OEMCrypto_IdentifyDevice()" );
	return OEMCrypto_SUCCESS;
}

