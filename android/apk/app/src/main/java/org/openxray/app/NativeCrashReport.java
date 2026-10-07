package org.openxray.app;

import android.app.ActivityManager;
import android.app.ApplicationExitInfo;
import android.content.Context;
import android.os.Build;
import android.util.AtomicFile;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.List;

/** Save Android's exit record beside the engine and Activity logs for the same launch. */
final class NativeCrashReport {
    private NativeCrashReport() {}

    private static ApplicationExitInfo latestEngineExit(Context context) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) return null;
        long started = SessionLogs.latestStartTime(context);
        if (started == 0) return null;
        ActivityManager manager = context.getSystemService(ActivityManager.class);
        if (manager == null) return null;
        List<ApplicationExitInfo> exits = manager.getHistoricalProcessExitReasons(null, 0, 32);
        if (exits == null) return null;
        String engineProcess = context.getPackageName() + ":engine";
        ApplicationExitInfo latest = null;
        for (ApplicationExitInfo exit : exits) {
            if (engineProcess.equals(exit.getProcessName()) && exit.getTimestamp() >= started &&
                    (latest == null || exit.getTimestamp() > latest.getTimestamp()))
                latest = exit;
        }
        return latest;
    }

    static synchronized void saveLatest(Context context) throws IOException {
        File[] session = SessionLogs.latest(context);
        if (session.length == 0) return;
        ApplicationExitInfo exit = latestEngineExit(context);
        if (exit == null) return;

        File detailsFile = session[3];
        File tombstoneFile = session[4];
        boolean nativeCrash = exit.getReason() == ApplicationExitInfo.REASON_CRASH_NATIVE;
        if (detailsFile.isFile() && (!nativeCrash || tombstoneFile.isFile())) return;

        StringBuilder details = new StringBuilder();
        details.append("process=").append(exit.getProcessName())
                .append(" pid=").append(exit.getPid())
                .append(" timestampMs=").append(exit.getTimestamp())
                .append(" reason=").append(exit.getReason())
                .append(" status=").append(exit.getStatus())
                .append(" rssKb=").append(exit.getRss()).append('\n');
        details.append("description=").append(exit.getDescription()).append('\n');
        if (nativeCrash && Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            try (InputStream source = exit.getTraceInputStream()) {
                if (source != null) {
                    writeAtomic(tombstoneFile, source);
                    details.append("tombstone=").append(tombstoneFile.getName())
                            .append(" (Android protobuf)\n");
                } else {
                    details.append("tombstone=unavailable from Android\n");
                }
            } catch (IOException | RuntimeException error) {
                details.append("tombstone=unavailable: ").append(error).append('\n');
            }
        } else if (nativeCrash) {
            details.append("tombstone=unavailable on Android below 12\n");
        } else {
            details.append("tombstone=unavailable (exit was not a native crash)\n");
        }
        byte[] text = details.toString().getBytes(StandardCharsets.UTF_8);
        try (InputStream source = new ByteArrayInputStream(text)) {
            writeAtomic(detailsFile, source);
        }
    }

    private static void writeAtomic(File destination, InputStream source) throws IOException {
        AtomicFile file = new AtomicFile(destination);
        FileOutputStream output = null;
        try {
            output = file.startWrite();
            byte[] buffer = new byte[65536];
            int count;
            while ((count = source.read(buffer)) != -1) output.write(buffer, 0, count);
            file.finishWrite(output);
        } catch (IOException | RuntimeException error) {
            if (output != null) file.failWrite(output);
            throw error;
        }
    }
}
