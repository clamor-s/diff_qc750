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
* Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property
* and proprietary rights in and to this software and related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA Corporation is strictly prohibited.
*/

#include "OEMCrypto.h"
#include <iostream>
#include <string.h>

using namespace std;

namespace WV {

    class Tester {
    public:
        void Run();

    private:
        void InstallKeybox();
    };


    void Tester::InstallKeybox()
    {
        cout << "calling OEMCrypto_InstallKeybox with encrypted wv keybox" << endl;

        OEMCryptoResult result;
        OEMCrypto_UINT8 Keybox[144];
        /*Decrypted keybox size is 128 bytes, but when it's encrypted by AES-ECB, it's goiong to be 144 Bytes.*/
        OEMCrypto_UINT32 KeyboxLen = 144;
        size_t r;

        FILE* fp;
        fp = fopen("/system/bin/wv_keybox.dat", "r");
        if(fp == NULL){
            cout << "OEMCrypto_InstallKeybox fopen failed" << endl;
            exit(-1);
        }

        r = fread(Keybox, 1 , KeyboxLen, fp);
        if(r != KeyboxLen){
            cout << "OEMCrypto_InstallKeybox fread failed" << endl;
            exit(-1);
        }
        fclose(fp);

        result = OEMCrypto_InstallKeybox(Keybox, KeyboxLen);
        if (result != OEMCrypto_SUCCESS) {
            cerr << "OEMCrypto_InstallKeybox failed : " << (int)result << endl;
            exit(-1);
        }
    }

    void Tester::Run()
    {
        cout << "Tester::Run" << endl;

        cout << "calling OEMCrypto_Initialize" << endl;

        OEMCryptoResult result = OEMCrypto_Initialize();
        if (result != OEMCrypto_SUCCESS) {
            cerr << "OEMCrypto_Initialize failed : " << (int)result << endl;
            exit(-1);
        }

        InstallKeybox();

        cout << "calling OEMCrypto_Terminate" << endl;

        result = OEMCrypto_Terminate();
        if (result != OEMCrypto_SUCCESS) {
            cerr << "OEMCrypto_Terminate failed : " << (int)result << endl;
            exit(-1);
        }
    }
}

int main(int argc, char **argv)
{
    WV::Tester tester;
    tester.Run();
    cout << "Test completed successfully!" << endl;
}

