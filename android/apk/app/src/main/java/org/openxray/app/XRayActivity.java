package org.openxray.app;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;
import java.nio.charset.StandardCharsets;

import org.libsdl.app.SDLActivity;

/**
 * SDL entry point for the Android renderer bring-up APK.
 *
 * The default mode creates a real GLES 3.1 context and runs a native shader
 * smoke test without bundling proprietary game data. Java-side lifecycle and
 * uncaught-exception records are written to app-specific external storage so
 * a phone test remains diagnosable without root access.
 */
public final class XRayActivity extends SDLActivity {
    private static final String TAG = "OpenXRay";
    private File diagnosticsFile;
    private boolean immersiveMode = true;
    private TouchControlsView touchControls;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Apply orientation before SDL creates its SurfaceView/EGL surface.
        // Doing it after super.onCreate leaves the first drawable portrait
        // and forces a destructive surface recreation during native startup.
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        if (getIntent().getBooleanExtra(LauncherActivity.EXTRA_KEEP_SCREEN_ON, true))
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        immersiveMode = getIntent().getBooleanExtra(LauncherActivity.EXTRA_IMMERSIVE, true);
        try {
            diagnosticsFile = createDiagnosticsFile();
        } catch (RuntimeException error) {
            Log.e(TAG, "Unable to initialize diagnostics storage", error);
            diagnosticsFile = null;
        }
        installCrashHandler();
        writeDiagnostic("activity onCreate; version=" + BuildConfig.VERSION_NAME
                + "; code=" + BuildConfig.VERSION_CODE + "; sdk=" + Build.VERSION.SDK_INT
                + "; abi=" + (Build.SUPPORTED_ABIS.length == 0 ? "unknown" : Build.SUPPORTED_ABIS[0]));
        try {
            super.onCreate(savedInstanceState);
            if (getIntent().getBooleanExtra(LauncherActivity.EXTRA_TOUCH_CONTROLS, true)) {
                touchControls = new TouchControlsView(this);
                addContentView(touchControls, new ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            }
            applyImmersiveMode();
        } catch (RuntimeException error) {
            writeDiagnostic("SDL activity startup failed: " + Log.getStackTraceString(error));
            throw error;
        }
    }

    @Override
    protected void onResume() {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        super.onResume();
        applyImmersiveMode();
        writeDiagnostic("activity onResume");
    }

    @Override
    protected void onPause() {
        if (touchControls != null)
            touchControls.releaseAllControls();
        writeDiagnostic("activity onPause");
        super.onPause();
    }

    @Override
    public void onBackPressed() {
        // Keep the SDL Activity and its native surface owner alive underneath
        // the launcher. A subsequent REORDER_TO_FRONT then reattaches to the
        // same engine instead of constructing another SDL entry point.
        Intent launcher = new Intent(this, LauncherActivity.class);
        launcher.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(launcher);
        writeDiagnostic("engine activity moved behind launcher");
    }

    @Override
    public void superOnBackPressed() {
        onBackPressed();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        // A launcher reattach intent intentionally has no engine arguments.
        // Keep the original launch intent so an Activity recreation cannot
        // silently lose the selected game root or profile.
        writeDiagnostic("existing engine activity brought to foreground");
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        applyImmersiveMode();
    }

    @Override
    public void setOrientationBis(int width, int height, boolean resizable, String hint) {
        // SDL calls this from native startup after Activity.onCreate(). Its
        // default implementation derives an orientation from the requested
        // window size and can overwrite the manifest/runtime landscape lock.
        // On several phones that leaves a landscape Surface containing a
        // portrait-rotated image. The engine Activity is always landscape;
        // the launcher remains independently locked to portrait.
        writeDiagnostic("SDL orientation request ignored; width=" + width
                + "; height=" + height + "; resizable=" + resizable + "; hint=" + hint);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }

    @Override
    protected void onDestroy() {
        writeDiagnostic("activity onDestroy");
        super.onDestroy();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            applyImmersiveMode();
    }

    @Override
    protected String[] getLibraries() {
        // SDL2 is linked statically into libmain.so for this APK.
        return new String[] { "main" };
    }

    @Override
    protected String[] getArguments() {
        boolean rendererSmoke = getIntent().getBooleanExtra(LauncherActivity.EXTRA_RENDERER_SMOKE, false);
        boolean vulkanRendererSmoke = getIntent().getBooleanExtra(LauncherActivity.EXTRA_RENDERER_VULKAN_SMOKE, false);
        String selectedPath = getIntent().getStringExtra(LauncherActivity.EXTRA_GAME_PATH);
        boolean gamepadEnabled = getIntent().getBooleanExtra(LauncherActivity.EXTRA_GAMEPAD_ENABLED, false);
        boolean splashEnabled = getIntent().getBooleanExtra(LauncherActivity.EXTRA_SPLASH_ENABLED, false);
        int gameVariant = getIntent().getIntExtra(LauncherActivity.EXTRA_GAME_VARIANT, 3);
        String[] additionalArgs = getIntent().getStringArrayExtra(LauncherActivity.EXTRA_ADDITIONAL_ARGS);
        int rendererMode = getIntent().getIntExtra(
                LauncherActivity.EXTRA_RENDERER_MODE, LauncherActivity.RENDERER_AUTO);
        int graphicsPreset = getIntent().getIntExtra(
                LauncherActivity.EXTRA_GRAPHICS_PRESET, LauncherActivity.GRAPHICS_AUTO);
        int renderWidth = getIntent().getIntExtra(LauncherActivity.EXTRA_RENDER_WIDTH, 1280);
        int renderHeight = getIntent().getIntExtra(LauncherActivity.EXTRA_RENDER_HEIGHT, 720);
        boolean showFps = getIntent().getBooleanExtra(LauncherActivity.EXTRA_SHOW_FPS, true);
        ArrayList<String> args = new ArrayList<>();
        args.add("-unique_logs");

        if (rendererSmoke) {
            args.add(vulkanRendererSmoke ? "-renderer-vulkan-smoke" : "-renderer-smoke");
            args.add("-nogame");
        } else if (selectedPath != null && !selectedPath.isEmpty()) {
            // The Android data path is slow and fragmented on many devices.
            // Disabling the desktop prefetch worker keeps a new-game load
            // single-threaded and avoids the post-loading stall/crash seen
            // when the cache races the ALife bootstrap.
            args.add("-noprefetch");
            args.add("-android-lazy-textures");
            args.add("-android-game-root-hex");
            args.add(encodeHex(selectedPath));
            String gameArg = OptionCatalog.value(OptionCatalog.GAME_ARGS, gameVariant, 0);
            if (!gameArg.isEmpty())
                args.add(gameArg);
        } else {
            args.add("-headless-smoke");
        }

        if (!rendererSmoke) {
            args.add(OptionCatalog.value(OptionCatalog.RENDERER_ARGS, rendererMode, 0));

            args.add("-android-render-width");
            args.add(Integer.toString(Math.max(320, renderWidth)));
            args.add("-android-render-height");
            args.add(Integer.toString(Math.max(320, renderHeight)));
            args.add("-android-mobile-preset");
            args.add(OptionCatalog.value(OptionCatalog.GRAPHICS_PRESETS, graphicsPreset, 0));
            if (showFps)
                args.add("-android-show-fps");
        }

        if (!gamepadEnabled)
            args.add("-no_gamepad");
        if (!splashEnabled)
            args.add("-nosplash");
        if (additionalArgs != null) {
            for (String argument : additionalArgs) {
                if (argument != null && !argument.isEmpty())
                    args.add(argument);
            }
        }

        String[] result = args.toArray(new String[0]);
        writeDiagnostic("native arguments: " + String.join(" ", result));
        return result;
    }

    static void setTouchControl(int control, boolean pressed) {
        nativeSetTouchControl(control, pressed);
    }

    static void moveTouchMouse(float deltaX, float deltaY) {
        nativeMoveTouchMouse(deltaX, deltaY);
    }

    static void clickTouchMouse(boolean pressed) {
        nativeClickTouchMouse(pressed);
    }

    private static native void nativeSetTouchControl(int control, boolean pressed);
    private static native void nativeMoveTouchMouse(float deltaX, float deltaY);
    private static native void nativeClickTouchMouse(boolean pressed);

    private void applyImmersiveMode() {
        if (!immersiveMode)
            return;
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }

    private String encodeHex(String value) {
        byte[] bytes = value.getBytes(StandardCharsets.UTF_8);
        StringBuilder result = new StringBuilder(bytes.length * 2);
        for (byte item : bytes)
            result.append(String.format(Locale.US, "%02x", item & 0xff));
        return result.toString();
    }

    private File createDiagnosticsFile() {
        try {
            String enginePath = getIntent().getStringExtra(LauncherActivity.EXTRA_ENGINE_LOG);
            String activityPath = getIntent().getStringExtra(LauncherActivity.EXTRA_ACTIVITY_LOG);
            if (activityPath == null || activityPath.isEmpty())
                throw new IOException("Missing activity log path");
            SessionLogs.activate(enginePath);
            File activity = new File(activityPath);
            if (!activity.isFile()) throw new IOException("Activity log was not created: " + activity);
            return activity;
        } catch (IOException | SecurityException error) {
            Log.e(TAG, "Unable to activate launcher log session", error);
            try {
                SessionLogs.Session fallback = SessionLogs.start(this,
                        getIntent().getStringExtra(LauncherActivity.EXTRA_GAME_PATH));
                SessionLogs.activate(fallback.engine.getAbsolutePath());
                return fallback.activity;
            } catch (IOException fallback) {
                throw new IllegalStateException("Unable to create diagnostics", fallback);
            }
        }
    }

    private void installCrashHandler() {
        final Thread.UncaughtExceptionHandler previous = Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler((thread, throwable) -> {
            writeDiagnostic("uncaught Java exception on " + thread.getName());
            writeDiagnostic(Log.getStackTraceString(throwable));
            Log.e(TAG, "uncaught Java exception", throwable);
            if (previous != null) {
                previous.uncaughtException(thread, throwable);
            }
        });
    }

    private synchronized void writeDiagnostic(String message) {
        Log.i(TAG, message);
        if (diagnosticsFile == null) {
            try {
                diagnosticsFile = createDiagnosticsFile();
            } catch (RuntimeException error) {
                Log.e(TAG, "Unable to open a timestamped diagnostic log", error);
                return;
            }
        }
        try (PrintWriter writer = new PrintWriter(new FileWriter(diagnosticsFile, true))) {
            String timestamp = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US)
                    .format(new Date());
            writer.println(timestamp + " " + message);
        } catch (IOException | SecurityException error) {
            Log.e(TAG, "Unable to write diagnostics: " + diagnosticsFile, error);
        }
    }
}
