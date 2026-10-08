package org.openxray.app;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import android.util.Log;
import android.widget.Toast;

public final class UpdateInstallReceiver extends BroadcastReceiver {
    static final String ACTION_INSTALL_STATUS = "org.openxray.action.UPDATE_INSTALL_STATUS";
    private static final String TAG = "OpenXRayUpdater";

    @Override
    public void onReceive(Context context, Intent intent) {
        int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
        if (status == PackageInstaller.STATUS_PENDING_USER_ACTION) {
            Intent confirmation = intent.getParcelableExtra(Intent.EXTRA_INTENT);
            if (confirmation != null) {
                confirmation.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(confirmation);
            }
            return;
        }

        if (status != PackageInstaller.STATUS_SUCCESS) {
            String detail = intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE);
            Log.e(TAG, "Update installation failed: " + detail);
            Toast.makeText(context, "Не удалось установить обновление: " + detail,
                    Toast.LENGTH_LONG).show();
        }
    }
}
