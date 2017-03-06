/*
 * Copyright (c) 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

package com.nvidia.graphics;

import android.view.Surface;
import android.view.SurfaceSession;
import android.graphics.Canvas;
import android.graphics.Path;
import android.util.Log;
import android.graphics.PixelFormat;

/**
 * @hide
 */

public class Cursor {
    static {
        System.loadLibrary("nvidia_graphics_jni");
    };
    static final String TAG = "Cursor";
    public static final int HW_CURSOR = 0x100;
    public static final int SW_CURSOR = 0x200;

    private final int mCursorType = HW_CURSOR;

    Surface mMouseSurface = null;
    boolean mCursorVisible = false;

    private native void HwShowCursor(boolean Show);
    private native void HwMoveCursor(int x, int y, int W, int H);

    private void SwShowCursor(boolean Show)
    {
        if (mMouseSurface==null) {
            try {
                int w = 20, h = 20;
                Canvas c;
                Path p = new Path();
                SurfaceSession s = new SurfaceSession();
            
                mMouseSurface = new Surface(s, 0, -1, w, h,
                    PixelFormat.TRANSPARENT, Surface.FX_SURFACE_NORMAL);
                c = mMouseSurface.lockCanvas(null);
                c.drawColor(0x0);
                p.moveTo(0.0f,0.0f);
                p.lineTo(16.0f, 0.0f);
                p.lineTo(0.0f, 16.0f);
                p.close();
                c.clipPath(p);
                c.drawColor(0x66666666);

                mMouseSurface.unlockCanvasAndPost(c);
                mMouseSurface.openTransaction();      
                mMouseSurface.setSize(w,h);
                mMouseSurface.closeTransaction();
            } catch (Exception e) {
                Log.e(TAG, "Error creating SW cursor surface", e);
            }
        }

        if (mMouseSurface != null) {
            mMouseSurface.openTransaction();
            if (Show)
                mMouseSurface.show();
            else
                mMouseSurface.hide();
            mMouseSurface.closeTransaction();
        }
    }

    private void SwMoveCursor(int x, int y, int AnimLayer)
    {
        if (mMouseSurface != null)
        {
            try {
                mMouseSurface.openTransaction();
                mMouseSurface.setPosition(x, y);
                mMouseSurface.setLayer(AnimLayer + 1);
                mMouseSurface.closeTransaction();
            } catch (Exception e) {
                Log.e(TAG, "Failure showing mouse surface", e);
            }
        }
    }

    public void ShowCursor(boolean Show)
    {
        if (Show == mCursorVisible)
            return;

        try {
            if (mCursorType == HW_CURSOR) {
                HwShowCursor(Show);
            }
            else if (mCursorType == SW_CURSOR) {
                SwShowCursor(Show);
            }
            mCursorVisible = Show;
        } catch (Exception e) {
            Log.e(TAG, "Caught exception showing mouse surface", e);
        }
    }
    
    public void MoveCursor(int x, int y, int a)
    {
        if (mCursorVisible) {
            try {
                if (mCursorType == HW_CURSOR) {
                    HwMoveCursor(x, y, 1024, 600);
                }

                else if (mCursorType == SW_CURSOR) {
                    SwMoveCursor(x, y, a);
                }
            } catch (Exception e) {
                Log.e(TAG, "Caught exception moving cursor", e);
            }
        }
    }

    public void MoveCursor(int x, int y, int W, int H, int a)
    {
        if (mCursorVisible) {
            try {
                if (mCursorType == HW_CURSOR) {
                    HwMoveCursor(x, y, W, H);
                }

                else if (mCursorType == SW_CURSOR) {
                    SwMoveCursor(x, y, a);
                }
            } catch (Exception e) {
                Log.e(TAG, "Caught exception moving cursor", e);
            }
        }
    }
}
