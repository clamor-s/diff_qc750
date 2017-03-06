// Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
package icera.DebugAgent;

import icera.DebugAgent.Lock;

import android.content.Context;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;

public class Lock {
    private static PowerManager.WakeLock lock;

    private static PowerManager.WakeLock getLock(Context context) {
        if (lock == null) {
            PowerManager mgr = (PowerManager) context
                               .getSystemService(Context.POWER_SERVICE);

            lock = mgr.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "icera.DebugAgent.lock");
            lock.setReferenceCounted(true);
        }
        return lock;
    }

    public static synchronized void acquire(Context context) {
        WakeLock wakeLock = getLock(context);
        if (!wakeLock.isHeld()) {
            wakeLock.acquire();
            Log.d("IDA", "wake lock acquired");
        }
    }

    public static synchronized void release() {
        if (lock == null) {
            Log.w(Lock.class.getSimpleName(),"release attempted, but wake lock was null");
        } else {
            if (lock.isHeld()) {
                lock.release();
                lock = null;
                Log.d("IDA", "wake lock released");
            } else {
                Log.w("IDA","release attempted, but wake lock was not held");
            }
        }
    }
}