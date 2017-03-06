NVIDIA Secure Interface v1.01

# NVSI requirements:

    a) unique key-box (provided by NVIDIA) MUST be installed on each device during the manufacturing process
    b) if EKS partition is not used NVSI service MUST be modified to fetch the key-box properly (see last section below for more details)
    c) correct nvsi.sdrv service MUST be included in the secure OS image (tf_include.h)
    d) Android framework patch (provided by NVIDIA) MUST be included in the OS image to allow filtering of NVSI applications by Google Play
    e) NvCPLSvc MUST be included in the final OS image which is flashed on the production devices

PLEASE NOTE THAT FAILURE TO EXECUTE ANY OF THE STEPS ABOVE CORRECTLY WILL RENDER NVSI UNUSABLE ON YOUR DEVICES!!!

--------------------------------------------------------------

# How to test if NVSI is installed and working correctly?

At the moment one can test NVSI in two different ways:

1) Testing with specific debug key-box (tests ALL NVSI functionality):

    a) Flash your device with the specific debug key-box which is provided at $TOP/vendor/nvidia/tegra/secureos/nvsi/keybox/dbg_keybox
    b) Go to $TOP/vendor/nvidia/tegra/secureos/nvsi/tools and execute nvsi.verify.dbg_key.sh in the command window
    c) ALL tests MUST PASS otherwise NVSI is not installed correctly on your device

    Sample output:

    NVSI Verifier Copyright (C) NVIDIA Corp 2012
    Running detailed tests for specific device with known key-box ...

    Secure OS:              PASSED
    Service:                PASSED
    Memory:                 PASSED
    Version:                PASSED
    Key-box:                PASSED
    CPL:                    PASSED
    Crypto:                 PASSED
    License Verification:   PASSED
    Interpreter:            PASSED

    --------------------KEYBOX ID--------------------
    15 02 64 14 ed d4 5c 01 00 00 00 00 00 00 00 00
    --------------------SIGNATURE--------------------
    d5 84 4a b4 9d 15 96 03 ba dc c2 be bc 0e 93 46
    fa d2 7b 25 d1 03 77 32 47 8e c0 44 d8 4f 94 07


2) Testing on any device (does not test all NVSI functionality):

    a) Go to $TOP/vendor/nvidia/tegra/secureos/nvsi/tools and execute nvsi.verify.any_key.sh in the terminal window
    b) ALL tests MUST PASS otherwise NVSI is not installed correctly on your device

    Sample output:
    
    NVSI Verifier Copyright (C) NVIDIA Corp 2012
    Running limited tests for any device ...

    Secure OS:              PASSED
    Service:                PASSED
    Memory:                 PASSED
    Version:                PASSED
    Key-box:                PASSED
    CPL:                    PASSED
    Crypto:                 PASSED
    Interpreter:            PASSED

    --------------------KEYBOX ID--------------------
    15 02 64 14 ed d4 5c 01 00 00 00 00 00 00 00 00
    --------------------SIGNATURE--------------------
    d5 84 4a b4 9d 15 96 03 ba dc c2 be bc 0e 93 46
    fa d2 7b 25 d1 03 77 32 47 8e c0 44 d8 4f 94 07

PLEASE NOTE THAT ALL TESTS ARE PERFORMED OFFLINE AND DO NOT REQUIRE INTERNET ACCESS. IF ANY OF THE TESTS FAIL PLEASE CONTACT NVIDIA AND SEND US THE TEST OUTPUT.

-------------------------------------------------------------

# How to generate NVSI secure driver?

Currently there are two scenarios:

1) EKS partition is used to store the NVSI key-box on device

In this scenario $TOP/3rdparty/trustedlogic/tools/key-injection/partition/eks_encrypt_keys.py script should be used to add all the keys (NVSI, widevine etc) used by the device to the eks.dat file. See $TOP/3rdparty/trustedlogic/tools/key-injection/partition/README for more information on how to generate eks.dat file.

Since EKS partition is used the default nvsi.sdrv service (provided in this folder) can be used to generate the secure OS image (tf_include.h).

2) EKS partition is NOT used to store the NVSI key-box on device

In this scenario the OEM provides special method for fetching the key-box on a device. In order to do this the following steps should be taken.

* Go to $TOP/vendor/nvidia/tegra/secureos/nvsi/keybox folder
* Open keybox.h file which contains description of a method oemcrypto_get_nvsi_keybox which needs to be implemented (OEMs can implement this method which ever way they prefer)
* Write a C implementation of the oemcrypto_get_nvsi_keybox method from the keybox.h
* Compile the code and generate liboem_keybox.a
* Replace the $TOP/vendor/nvidia/tegra/secureos/nvsi/keybox/liboem_keybox.a with your own implementation
* Edit the $TOP/vendor/nvidia/tegra/secureos/nvsi/link_service.bat batch file to make sure the paths to TrustedLogic SDK and RVCT ARM compiler are correct and valid on your target machine
* Run the batch file $TOP/vendor/nvidia/tegra/secureos/nvsi/link_service.bat to generate custom nvsi.sdrv

The new custom nvsi.sdrv can now be used to generate the secure OS image (tf_include.h). For instructions on how to generate secure OS image please refer to the documentation in the TrustedLogic's SDK.
