/*
 * Copyright (c) 2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

package com.android.settings;

import android.app.Activity;
import android.app.Dialog;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.webkit.WebView;

import com.android.settings.StereoSepElement;
import com.android.settings.HDMI3DPlayJNIHelper;

import java.io.FileReader;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.Locale;

/*
 * Displays preferences for HDMI 3DPlay
 */
public class HDMI3DPlaySettings extends SettingsPreferenceFragment {

    private static final String HDMI_3DPLAY_CONTROL = "hdmi_3dplay_control";
    private static final String HDMI_3D_SLIDER = "hdmi_3d_slider";
    private static final String HDMI_3D_HELP = "hdmi_3d_help";
    private static final String HDMI3DPlayCtrl = "NV_STEREOCTRL";
    private static final String TAG = "HDMI3DPlaySettings";

    private static final String HDMI_HELP_URL = "file:///android_asset/html/%y%z/hdmi_help.html";
    private static final String HDMI_HELP_PATH = "html/%y%z/hdmi_help.html";

    // Id for the Depth Slider and HDMI Help dialogs
    private static final int DIALOG_HDMI_SLIDER = 1;
    private static final int DIALOG_HDMI_HELP = 2;

    // WebView to display the HDMI help information
    private WebView mWebView;

    private CheckBoxPreference m3DPlayControl;
    private PreferenceScreen mHDMI3DSlider;
    private PreferenceScreen mHDMI3DHelp;

    private HDMI3DPlayJNIHelper mJNIHelper = null;
    static final boolean DEBUG = false;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        addPreferencesFromResource(R.xml.hdmi_3dplay_prefs);

        final Activity activity = getActivity();

        m3DPlayControl = (CheckBoxPreference)findPreference(HDMI_3DPLAY_CONTROL);
        mHDMI3DSlider = (PreferenceScreen)findPreference(HDMI_3D_SLIDER);
        mHDMI3DHelp = (PreferenceScreen)findPreference(HDMI_3D_HELP);
        mJNIHelper = new HDMI3DPlayJNIHelper(activity);
        mWebView = new WebView(activity);
    }

    @Override
    public void onStart() {
        super.onStart();

        // Read the current property setting to determine whether HDMI 3DPlay is enabled
        // or disabled, and clear/set the checkbox accordingly
        int ctrlVal = 0;
        if (mJNIHelper != null) {
            String strCtrlVal = mJNIHelper.getProperty(HDMI3DPlayCtrl);
            if (!strCtrlVal.equals("")) {
                ctrlVal = Integer.parseInt(strCtrlVal);
            }
        }
        if (ctrlVal == 0) {
            m3DPlayControl.setSummary(R.string.hdmi_3dplay_control_disable_subtext);
            m3DPlayControl.setChecked(false);
        } else {
            m3DPlayControl.setSummary(R.string.hdmi_3dplay_control_enable_subtext);
            m3DPlayControl.setChecked(true);
        }

        // The availability of these options depends on the connected state
        boolean isConnected = IsHDMIConnected();
        m3DPlayControl.setEnabled(isConnected);
        mHDMI3DSlider.setEnabled(isConnected);
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen screen, Preference preference) {

        if ((mJNIHelper != null) && (preference == m3DPlayControl)) {
            boolean bIsChecked = m3DPlayControl.isChecked();
            mJNIHelper.setProperty(HDMI3DPlayCtrl, bIsChecked ? "1" : "0");

            if (bIsChecked) {
                m3DPlayControl.setSummary(R.string.hdmi_3dplay_control_enable_subtext);
            } else {
                m3DPlayControl.setSummary(R.string.hdmi_3dplay_control_disable_subtext);
            }

        } else if (preference == mHDMI3DSlider) {
            showDialog(DIALOG_HDMI_SLIDER);
            return true;
        } else if (preference == mHDMI3DHelp) {
            showDialog(DIALOG_HDMI_HELP);
            return true;
        }

        return super.onPreferenceTreeClick(screen, preference);
    }

    @Override
    public Dialog onCreateDialog(int id) {

        if (id == DIALOG_HDMI_SLIDER) {
            View sliderView = (View)(new StereoSepElement(getActivity()));
            return new AlertDialog.Builder(getActivity())
                .setCancelable(true)
                .setIcon(R.drawable.ic_hdmi_3d)
                .setTitle(R.string.adjust_3d_depth_title)
                .setView(sliderView)
                .create();

        } else if (id == DIALOG_HDMI_HELP) {
            Locale locale = Locale.getDefault();

            // check for the full language + country resource, if not there, try just language
            final AssetManager am = getActivity().getAssets();
            String path = HDMI_HELP_PATH.replace("%y", locale.getLanguage().toLowerCase());
            path = path.replace("%z", '_'+locale.getCountry().toLowerCase());
            boolean useCountry = true;
            InputStream is = null;
            try {
                is = am.open(path);
            } catch (Exception ignored) {
                useCountry = false;
            } finally {
                if (is != null) {
                    try {
                        is.close();
                    } catch (Exception ignored) {}
                }
            }

            // If the language only resource does not exist either, default to en_us
            path = HDMI_HELP_PATH.replace("%y", locale.getLanguage().toLowerCase());
            path = path.replace("%z", "");
            boolean useOnlyLanguage = true;
            is = null;
            try {
                is = am.open(path);
            } catch (Exception ignored) {
                useOnlyLanguage = false;
            } finally {
                if (is != null) {
                    try {
                        is.close();
                    } catch (Exception ignored) {}
                }
            }

            String url = "";
            if ((useCountry == false) && (useOnlyLanguage == false)) {
                url = HDMI_HELP_URL.replace("%y", "en_us");
                url = url.replace("%z", "");
            } else {
                url = HDMI_HELP_URL.replace("%y", locale.getLanguage().toLowerCase());
                url = url.replace("%z", useCountry ? '_'+locale.getCountry().toLowerCase() : "");
            }
            Log.v(TAG, "Opening asset " + url);
            mWebView.loadUrl(url);
            // Detach from old parent first
            ViewParent parent = mWebView.getParent();
            if (parent != null && parent instanceof ViewGroup) {
                ((ViewGroup) parent).removeView(mWebView);
            }
            return new AlertDialog.Builder(getActivity())
                .setCancelable(true)
                .setTitle(R.string.hdmi_3d_help_title)
                .setView(mWebView)
                .create();
        }

        return null;
    }

    public static String readHDMISwitchState() {
        FileReader hdmiStateFile = null;
        try {
            char[] buffer = new char[1024];
            hdmiStateFile = new FileReader("/sys/class/switch/tegradc.1/state");
            int len = hdmiStateFile.read(buffer, 0, 1024);

            String hdmiState = (new String(buffer, 0, len)).trim();

            return hdmiState;
        } catch (FileNotFoundException e) {
            // Should indicate unavailability of HDMI connector
            Log.e(TAG, "Couldn't read HDMI state");
            return null;
        } catch (Exception e) {
            Log.e(TAG, "", e);
            return null;
        } finally {
            if (hdmiStateFile != null) {
                try {
                    hdmiStateFile.close();
                } catch (Exception ignored) {}
            }
        }
    }

    public static boolean IsHDMICapable() {
        String hdmiState = readHDMISwitchState();

        if (hdmiState == null) {
            // no HDMI
            return false;
        }

        // Assumes a return string from readHDMISwitchState indicates a valid connector
        // Parse expected values of hdmi state ("offline", or "wxh") for logcat
        if (hdmiState.equals("offline")) {
            if (DEBUG) Log.v(TAG, "HDMI Display NOT connected");
        } else {
            String[] strDims = hdmiState.split("x");
            if (strDims.length == 2) {
                if (DEBUG) Log.v(TAG, "HDMI Display connected, dims = " + strDims[0] + ", " + strDims[1]);
            } else {
                Log.v(TAG, "Unrecognized hdmi switch state value: " + hdmiState);
            }
        }

        return true;
    }

    public static boolean IsHDMIConnected() {
        String hdmiState = readHDMISwitchState();

        if (hdmiState == null) {
            // no HDMI
            return false;
        }

        // The expected return from HDMI switch state is either "offline" (not connected)
        // or "<width>x<height>" implying connected.
        String[] strDims = hdmiState.split("x");
        if (strDims.length == 2) {
            if (DEBUG) Log.v(TAG, "HDMI Display connected, dims = " + strDims[0] + ", " + strDims[1]);
            return true;
        }

        return false;
    }

}
