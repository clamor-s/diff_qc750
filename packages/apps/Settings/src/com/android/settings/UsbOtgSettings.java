/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

package com.android.settings;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.util.Log;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;


/*
 * Settings for USB On The Go.
 * More specifically: Host Mode
 */
public class UsbOtgSettings extends SettingsPreferenceFragment {

    private static final String FILENAME_USB_HOST_MODE = "/sys/devices/platform/tegra-otg/enable_host";
    private static final String KEY_USB_HOST_MODE = "usb_host_mode";
    private static final String USBHostModeProp = "NV_USBHOST";
    private static final String TAG = "UsbOtgSettings";

    private CheckBoxPreference mUSBHostMode;

    private HDMI3DPlayJNIHelper mJNIHelper = null;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        addPreferencesFromResource(R.xml.usb_otg_settings);

        final Activity activity = getActivity();

        mUSBHostMode = (CheckBoxPreference)findPreference(KEY_USB_HOST_MODE);
        mJNIHelper = new HDMI3DPlayJNIHelper(activity);
    }

    @Override
    public void onStart() {
        super.onStart();

        boolean enabledUSBHost = getUsbOtgHostModeEnable();

        mUSBHostMode.setEnabled(true);
        mUSBHostMode.setChecked(enabledUSBHost);
        mUSBHostMode.setSummary(R.string.usb_host_mode_subtext);
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    /**
     * Reads a line from the specified file.
     * @param filename the file to read from
     * @return the first line, if any.
     * @throws IOException if the file couldn't be read
     */
    private String readLine(String filename) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(filename), 64);
        try {
            return reader.readLine();
        } finally {
            reader.close();
        }
    }

    /*
     *  Return the current state of USB host mode
     */
    private boolean getUsbOtgHostModeEnable() {
        // The host mode state is read from the driver instead of
        // querying the android property.
        try {
            String enable_host = readLine(FILENAME_USB_HOST_MODE);
            if (Long.parseLong(enable_host) == 1) {
                return true;
            } else {
                return false;
            }
        } catch (IOException ioe) {
            // probable non-existent file
            Log.e(TAG, "Unable to read USB Host Mode status from " + FILENAME_USB_HOST_MODE +
                       " : " + ioe.getMessage());
        }
        return false;
    }


    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen screen, Preference preference) {

        if (preference == mUSBHostMode) {
            boolean bIsChecked = mUSBHostMode.isChecked();

            // mJNIHelper.setProperty will send the state to the NvCplService.
            // The service will manage the persistent state and it will also
            // handle writing the state to the sysfs node interface to the
            // usb tegra-otg driver.  See the service code for more details.
            if (mJNIHelper != null) {
                mJNIHelper.setProperty(USBHostModeProp, bIsChecked ? "1" : "0");
            }
        }

        return super.onPreferenceTreeClick(screen, preference);
    }

    public static boolean isUsbOtgAvailable() {
        // Just try reading the file to see if it exists.
        try {
            BufferedReader reader = new BufferedReader(new FileReader(FILENAME_USB_HOST_MODE), 64);
            String enable_host = reader.readLine();
            reader.close();
            return true;
        } catch (Exception e) {
            // probable non-existent file
            return false;
        }
    }
}
