package org.openxray.app;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Process;
import android.util.Log;

/** Receives explicit launcher commands inside the dedicated :engine process. */
public final class EngineControlReceiver extends BroadcastReceiver {
    public static final String ACTION_STOP_ENGINE = "org.openxray.action.STOP_ENGINE";

    private static final String TAG = "OpenXRay";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent == null ? null : intent.getAction();
        if (ACTION_STOP_ENGINE.equals(action)) {
            Log.w(TAG, "launcher requested forced engine stop");
            Process.killProcess(Process.myPid());
        }
    }
}
