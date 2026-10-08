package org.openxray.app;

import android.content.Context;
import android.system.Os;
import android.util.AtomicFile;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.UUID;
import java.nio.charset.StandardCharsets;

final class SessionLogs {
    private SessionLogs() {}

    private static AtomicFile pointer(Context context) {
        return new AtomicFile(new File(context.getFilesDir(), "last-engine-log.txt"));
    }

    static File pending(Context context) {
        return new File(context.getFilesDir(), "launcher-pending.log");
    }

    static final class Session {
        final File engine;
        final File activity;
        Session(File engine, File activity) { this.engine = engine; this.activity = activity; }
    }

    static Session start(Context context, String gamePath) throws IOException {
        File root = gamePath == null || gamePath.isEmpty()
                ? new File(context.getFilesDir(), "openxray/logs")
                : new File(gamePath, "_appdata_/logs");
        if (!root.isDirectory() && !root.mkdirs())
            throw new IOException("Cannot create log directory: " + root);
        String stamp = new SimpleDateFormat("yyyyMMdd_HHmmss_SSS", Locale.US).format(new Date());
        String id = stamp + "_" + android.os.Process.myPid() + "_" + UUID.randomUUID().toString().substring(0, 8);
        File engine = new File(root, "android_" + id + ".log");
        File activity = new File(root, "activity_" + id + ".log");
        if (!engine.createNewFile() || !activity.createNewFile())
            throw new IOException("Unable to create unique log session in " + root);
        File preflight = pending(context);
        if (preflight.isFile()) {
            try (FileInputStream input = new FileInputStream(preflight);
                 FileOutputStream output = new FileOutputStream(activity, true)) {
                byte[] bytes = new byte[8192];
                int count;
                while ((count = input.read(bytes)) != -1) output.write(bytes, 0, count);
            }
            if (!preflight.delete()) throw new IOException("Cannot clear launcher preflight log: " + preflight);
        }
        AtomicFile pointer = pointer(context);
        FileOutputStream output = null;
        try {
            output = pointer.startWrite();
            output.write((engine.getAbsolutePath() + "\n" + activity.getAbsolutePath() + "\n"
                    + System.currentTimeMillis()).getBytes(StandardCharsets.UTF_8));
            pointer.finishWrite(output);
        } catch (IOException error) {
            if (output != null) pointer.failWrite(output);
            throw error;
        }
        return new Session(engine, activity);
    }

    static File activate(String enginePath) throws IOException {
        if (enginePath == null || enginePath.isEmpty())
            throw new IOException("Missing engine log path");
        File engine = new File(enginePath);
        if (!engine.isFile()) throw new IOException("Engine log was not created: " + engine);
        try {
            // Set before SDL starts its native engine thread.
            Os.setenv("OPENXRAY_ENGINE_LOG", engine.getAbsolutePath(), true);
        } catch (android.system.ErrnoException error) {
            throw new IOException(error);
        }
        return engine;
    }

    static File[] latest(Context context) {
        try {
            String path = new String(pointer(context).readFully(), StandardCharsets.UTF_8);
            String[] paths = path.split("\\n", -1);
            if (paths.length < 2 || !new File(paths[0]).isFile() || !new File(paths[1]).isFile()) return new File[0];
            return new File[] {new File(paths[0]), new File(paths[1]),
                    new File(paths[0] + ".native-exit.txt"),
                    new File(paths[0] + ".native-tombstone.pb")};
        } catch (IOException error) {
            return new File[0];
        }
    }

    static long latestStartTime(Context context) {
        try {
            String[] paths = new String(pointer(context).readFully(), StandardCharsets.UTF_8).split("\\n", -1);
            return paths.length >= 3 ? Long.parseLong(paths[2]) : 0;
        } catch (IOException | NumberFormatException error) {
            return 0;
        }
    }
}
