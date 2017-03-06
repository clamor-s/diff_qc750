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

import android.content.Context;
import android.os.Bundle;
import android.preference.SeekBarPreference;
import android.util.AttributeSet;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import com.android.settings.HDMI3DPlayJNIHelper;

public class StereoSepElement extends RelativeLayout implements
        SeekBar.OnSeekBarChangeListener {

    private SeekBar mSlider;
    private TextView mDepth3DView;
    private Context mContext;
    private int mDepth3DVal;
    private boolean mbInitDone;
    private HDMI3DPlayJNIHelper mJNIHelper = null;

    private static final String StereoSeparation = "NV_STEREOSEP";
    private static final String TAG = "StereoSepElement";

    public StereoSepElement(Context context) {
        this(context, null);
    }

    public StereoSepElement(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public StereoSepElement(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        View.inflate(context, R.layout.stereo_sep_slider_element, this);

        mJNIHelper = new HDMI3DPlayJNIHelper(context);
        mContext = context;
        mbInitDone = false;

        mDepth3DView = (TextView)findViewById(R.id.hdmi_3d_stereo_separation_val);
        mSlider = (SeekBar)findViewById(R.id.hdmi_3d_stereo_separation_slider);
        mSlider.setOnSeekBarChangeListener(this);

        // Make sure the property is updated with the default for the first time
        // Read the current property value. If it exists, then display that in the UI.
        // Otherwise, we assume that the property needs to be written for the first time, and
        // so write it to a value of 20%
        int progressVal = 20;

        if (mJNIHelper != null) {
            String strDepthPropVal = mJNIHelper.getProperty(StereoSeparation);
            if (!strDepthPropVal.equals("")) {
                mbInitDone = true;
                progressVal = Integer.parseInt(strDepthPropVal);
            }
        }
        mSlider.setProgress(progressVal);
    }

    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromTouch) {
        mDepth3DVal = progress;
        mDepth3DView.setText(progress + "%");

        if (!mbInitDone && (mJNIHelper != null)) {
            // Only to be called for initializing the first time
            // Subsequent updates will happen in onStopTrackingTouch()
            mJNIHelper.setProperty(StereoSeparation, Integer.toString(mDepth3DVal));
            mbInitDone = true;
        }
    }

    public void onStartTrackingTouch(SeekBar seekBar) {
        // Do nothing for now
    }

    public void onStopTrackingTouch(SeekBar seekBar) {
        if (mJNIHelper != null) {
            mJNIHelper.setProperty(StereoSeparation, Integer.toString(mDepth3DVal));
        }
    }
}

