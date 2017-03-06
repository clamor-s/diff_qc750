#!/usr/bin/python
#
# Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

import common

def FullOTA_InstallEnd(info):
    try:
        info.input_zip.getinfo("RADIO/blob")
    except KeyError:
        return;
    else:
        # copy the data into the package.
        blob = info.input_zip.read("RADIO/blob")
        common.ZipWriteStr(info.output_zip, "blob", blob)
        # emit the script code to install this data on the device
        info.script.AppendExtra(
                """nv_copy_blob_file("blob", "/staging");""")


def InstallAndroidInfo(input_zip,output_zip):
    data =input_zip.read("OTA/android-info.txt")
    output_zip.writestr("android-info.txt", data)


def IncrementalOTA_InstallEnd(info):
    InstallAndroidInfo(info.target_zip,info.output_zip)	
    try:
        info.target_zip.getinfo("RADIO/blob")
    except KeyError:
        return;
    else:
        target_blob = info.target_zip.read("RADIO/blob")
        try:
            info.source_zip.getinfo("RADIO/blob")
            # copy the data into the package.
            source_blob = info.source_zip.read("RADIO/blob")
            if source_blob == target_blob:
                # blob is unchanged from previous build; no
                # need to reprogram it
                return;
            else:
                # include the new blob in the OTA package
                common.ZipWriteStr(info.output_zip, "blob", target_blob)
                # emit the script code to install this data on the device
                info.script.AppendExtra(
                        """nv_copy_blob_file("blob", "/staging");""")
        except KeyError:
            # include the new blob in the OTA package
            common.ZipWriteStr(info.output_zip, "blob", target_blob)
            # emit the script code to install this data on the device
            info.script.AppendExtra(
                    """nv_copy_blob_file("blob", "/staging");""")


