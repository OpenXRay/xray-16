package org.openxray.app;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.pm.PackageInstaller;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.provider.DocumentsContract;
import android.provider.Settings;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Configuration and diagnostics front end for the native SDL engine.
 *
 * The launcher never rewrites game resources or fsgame.ltx. It stages only
 * OpenXRay-owned renderer data privately, verifies the installation's normal
 * _appdata_ directory is writable, and passes explicit choices to the engine.
 */
public final class LauncherActivity extends Activity {
    public static final String EXTRA_GAME_PATH = "org.openxray.extra.GAME_PATH";
    public static final String EXTRA_GAME_VARIANT = "org.openxray.extra.GAME_VARIANT";
    public static final String EXTRA_ADDITIONAL_ARGS = "org.openxray.extra.ADDITIONAL_ARGS";
    public static final String EXTRA_RENDERER_SMOKE = "org.openxray.extra.RENDERER_SMOKE";
    public static final String EXTRA_RENDERER_VULKAN_SMOKE = "org.openxray.extra.RENDERER_VULKAN_SMOKE";
    public static final String EXTRA_GAMEPAD_ENABLED = "org.openxray.extra.GAMEPAD_ENABLED";
    public static final String EXTRA_SPLASH_ENABLED = "org.openxray.extra.SPLASH_ENABLED";
    public static final String EXTRA_KEEP_SCREEN_ON = "org.openxray.extra.KEEP_SCREEN_ON";
    public static final String EXTRA_IMMERSIVE = "org.openxray.extra.IMMERSIVE";
    public static final String EXTRA_TOUCH_CONTROLS = "org.openxray.extra.TOUCH_CONTROLS";
    public static final String EXTRA_RENDERER_MODE = "org.openxray.extra.RENDERER_MODE";
    public static final String EXTRA_GRAPHICS_PRESET = "org.openxray.extra.GRAPHICS_PRESET";
    public static final String EXTRA_ENGINE_LOG = "org.openxray.extra.ENGINE_LOG";
    public static final String EXTRA_ACTIVITY_LOG = "org.openxray.extra.ACTIVITY_LOG";
    public static final String EXTRA_RENDER_WIDTH = "org.openxray.extra.RENDER_WIDTH";
    public static final String EXTRA_RENDER_HEIGHT = "org.openxray.extra.RENDER_HEIGHT";
    public static final String EXTRA_SHOW_FPS = "org.openxray.extra.SHOW_FPS";

    private static final String PREFS = "openxray_launcher";
    private static final String PREF_GAME_PATH = "game_path";
    private static final String PREF_GAME_URI = "game_uri";
    private static final String PREF_GAME_PATH_PREFIX = "game_path_profile_";
    private static final String PREF_GAME_URI_PREFIX = "game_uri_profile_";
    private static final String PREF_GAME_VARIANT = "game_variant";
    private static final String PREF_CUSTOM_ARGS = "custom_args";
    private static final String PREF_GAMEPAD = "gamepad";
    private static final String PREF_SPLASH = "splash";
    private static final String PREF_KEEP_SCREEN_ON = "keep_screen_on";
    private static final String PREF_IMMERSIVE = "immersive";
    private static final String PREF_TOUCH_CONTROLS = "touch_controls";
    private static final String PREF_ACTIVE_PAGE = "active_page";
    private static final String PREF_RENDERER_MODE = "renderer_mode";
    private static final String PREF_GRAPHICS_PRESET = "graphics_preset";
    private static final String PREF_RENDER_RESOLUTION = "render_resolution";
    private static final String PREF_SHOW_FPS = "show_fps";

    public static final int RENDERER_AUTO = 0;
    public static final int RENDERER_GLES = 1;
    public static final int RENDERER_VULKAN = 2;

    public static final int GRAPHICS_AUTO = 0;
    public static final int GRAPHICS_MINIMUM = 1;
    public static final int GRAPHICS_LOW = 2;
    public static final int GRAPHICS_DEFAULT = 3;
    public static final int GRAPHICS_HIGH = 4;
    public static final int GRAPHICS_EXTREME = 5;

    private static final int PAGE_GAME = 0;
    private static final int PAGE_SETTINGS = 1;
    private static final int PAGE_DIAGNOSTICS = 2;
    private static final int REQUEST_GAME_TREE = 1001;
    private static final int REQUEST_STORAGE_PERMISSIONS = 1002;
    private static final int MAX_LOG_BYTES = 32 * 1024;
    private static final int MAX_SHARED_LOG_CHARS = 600 * 1024;

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final ExecutorService logExecutor = Executors.newSingleThreadExecutor();
    private final ExecutorService updateExecutor = Executors.newSingleThreadExecutor();
    private EditText gamePath;
    private EditText customArgs;
    private Spinner gameVariant;
    private Spinner rendererMode;
    private Spinner graphicsPreset;
    private Spinner renderResolution;
    private CheckBox gamepadEnabled;
    private CheckBox splashEnabled;
    private CheckBox keepScreenOn;
    private CheckBox immersiveMode;
    private CheckBox touchControlsEnabled;
    private CheckBox showFps;
    private CheckBox[] advancedOptions;
    private TextView accessStatus;
    private TextView gameInspection;
    private TextView status;
    private TextView updateStatus;
    private TextView logView;
    private Button launchButton;
    private Button stopButton;
    private Button updateButton;
    private Button[] tabButtons;
    private View[] pages;
    private SharedPreferences preferences;
    private long engineLaunchTime;
    private int stopGeneration;
    private boolean engineFailureToastShown;
    private boolean suppressProfileCallbacks = true;
    private int activeGameVariant = 3;
    private int activePage = PAGE_GAME;
    private boolean logReadPending;
    private String cachedLog = "";
    private AppUpdater.Update pendingUpdate;
    private final ArrayList<RenderResolution> renderResolutions = new ArrayList<>();

    private static final class RenderResolution {
        final String id;
        final String label;
        final int width;
        final int height;

        RenderResolution(String id, String label, int width, int height) {
            this.id = id;
            this.label = label;
            this.width = width;
            this.height = height;
        }

        @Override
        public String toString() {
            return label;
        }
    }

    private final Runnable logPoller = new Runnable() {
        @Override
        public void run() {
            if (activePage == PAGE_DIAGNOSTICS)
                refreshLog();
            refreshRunningState();
            handler.postDelayed(this, activePage == PAGE_DIAGNOSTICS ? 1500 : 3000);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        preferences = getSharedPreferences(PREFS, MODE_PRIVATE);
        buildInterface();
        restorePreferences();
        refreshAccessStatus();
        refreshGameInspection();
        refreshRunningState();
        handler.post(this::showStorageAccessPromptIfNeeded);
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshAccessStatus();
        refreshGameInspection();
        if (activePage == PAGE_DIAGNOSTICS)
            refreshLog();
        refreshRunningState();
        handler.removeCallbacks(logPoller);
        handler.post(logPoller);
        if (pendingUpdate != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                && getPackageManager().canRequestPackageInstalls()) {
            AppUpdater.Update update = pendingUpdate;
            pendingUpdate = null;
            handler.post(() -> installUpdate(update));
        }
    }

    @Override
    protected void onPause() {
        savePreferences();
        handler.removeCallbacks(logPoller);
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacks(logPoller);
        logExecutor.shutdownNow();
        updateExecutor.shutdownNow();
        super.onDestroy();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_GAME_TREE || resultCode != RESULT_OK || data == null || data.getData() == null)
            return;

        Uri treeUri = data.getData();
        final int takeFlags = data.getFlags()
                & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        try {
            getContentResolver().takePersistableUriPermission(treeUri, takeFlags);
        } catch (SecurityException ignored) {
            // Some OEM document providers do not offer persistable grants.
        }

        String resolvedPath = resolvePrimaryStoragePath(treeUri);
        if (resolvedPath != null) {
            gamePath.setText(resolvedPath);
            preferences.edit().putString(PREF_GAME_PATH, resolvedPath)
                    .putString(profilePreference(PREF_GAME_PATH_PREFIX, activeGameVariant), resolvedPath)
                    .putString(PREF_GAME_URI, treeUri.toString())
                    .putString(profilePreference(PREF_GAME_URI_PREFIX, activeGameVariant), treeUri.toString())
                    .apply();
            setStatus("Выбрана папка: " + resolvedPath);
        } else {
            preferences.edit()
                    .putString(PREF_GAME_URI, treeUri.toString())
                    .putString(profilePreference(PREF_GAME_URI_PREFIX, activeGameVariant), treeUri.toString())
                    .apply();
            setStatus("Папка выбрана через системный проводник, но Android не раскрыл прямой путь. "
                    + "Укажите его вручную: native-движок не может читать content:// URI как обычный каталог.");
        }
        refreshAccessStatus();
        refreshGameInspection();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != REQUEST_STORAGE_PERMISSIONS)
            return;

        refreshAccessStatus();
        setStatus(hasStorageAccess()
                ? "Доступ к файлам выдан."
                : "Доступ к файлам не выдан. Его можно включить в настройках приложения.");
    }

    private void buildInterface() {
        final int padding = dp(16);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(padding, dp(12), padding, dp(10));

        TextView title = new TextView(this);
        title.setText("OpenXRay Android");
        title.setTextSize(25);
        title.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        root.addView(title, matchWrap());

        TextView version = new TextView(this);
        version.setText("Версия " + BuildConfig.VERSION_NAME + " · ARMv7");
        version.setTextSize(13);
        version.setPadding(0, 0, 0, dp(10));
        root.addView(version, matchWrap());

        LinearLayout tabs = new LinearLayout(this);
        tabs.setOrientation(LinearLayout.HORIZONTAL);
        tabButtons = new Button[3];
        for (int page = 0; page < tabButtons.length; ++page)
            tabButtons[page] = makeTab(OptionCatalog.TABS[page], page);
        for (Button button : tabButtons)
            tabs.addView(button, new LinearLayout.LayoutParams(0, dp(48), 1));
        root.addView(tabs, matchWrap());

        pages = new View[] { buildGamePage(), buildSettingsPage(), buildDiagnosticsPage() };
        for (View page : pages)
            root.addView(page, new LinearLayout.LayoutParams(-1, 0, 1));

        setContentView(root);
    }

    private Button makeTab(String label, int page) {
        Button button = new Button(this);
        button.setText(label);
        button.setAllCaps(false);
        button.setOnClickListener(view -> showPage(page));
        return button;
    }

    private View buildGamePage() {
        LinearLayout content = pageContent();
        addSectionTitle(content, "Профиль игры");
        content.addView(bodyText("Выберите профиль игры."), matchWrap());

        gameVariant = new Spinner(this);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, OptionCatalog.GAME_LABELS);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        gameVariant.setAdapter(adapter);
        gameVariant.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (!suppressProfileCallbacks && gamePath != null)
                    switchGameProfile(position);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
        content.addView(gameVariant, matchWrap());

        addSectionTitle(content, "Установка игры");

        content.addView(bodyText("Выберите папку установки с fsgame.ltx."), matchWrap());

        gamePath = new EditText(this);
        gamePath.setSingleLine(true);
        gamePath.setHint("/storage/emulated/0/STALKER");
        gamePath.setOnFocusChangeListener((view, focused) -> {
            if (!focused)
                refreshGameInspection();
        });
        content.addView(gamePath, matchWrap());

        LinearLayout pathActions = horizontalRow();
        pathActions.addView(actionButton("Выбрать папку", view -> chooseGameFolder()), weightedButton());
        pathActions.addView(actionButton("Проверить", view -> refreshGameInspection()), weightedButton());
        content.addView(pathActions, matchWrap());

        gameInspection = bodyText("");
        gameInspection.setTextIsSelectable(true);
        gameInspection.setPadding(dp(2), dp(6), dp(2), dp(10));
        content.addView(gameInspection, matchWrap());

        accessStatus = bodyText("");
        content.addView(accessStatus, matchWrap());

        content.addView(actionButton("Настроить доступ к памяти", view -> requestAllFilesAccess()), matchWrap());

        addSectionTitle(content, "Запуск");
        LinearLayout launchActions = horizontalRow();
        launchButton = actionButton("Запустить игру", view -> launchEngine(false));
        launchButton.setTextSize(17);
        launchActions.addView(launchButton, new LinearLayout.LayoutParams(0, dp(60), 1));
        stopButton = actionButton("■", view -> confirmStopEngine());
        stopButton.setContentDescription("Остановить запущенный движок");
        stopButton.setTextSize(18);
        stopButton.setTextColor(Color.rgb(170, 25, 25));
        LinearLayout.LayoutParams stopParams = new LinearLayout.LayoutParams(dp(58), dp(60));
        stopParams.setMarginStart(dp(6));
        launchActions.addView(stopButton, stopParams);
        content.addView(launchActions, matchWrap());
        content.addView(actionButton("Проверка GLES", view -> launchEngine(true)),
                new LinearLayout.LayoutParams(-1, dp(52)));
        content.addView(actionButton("Проверка Vulkan", view -> launchVulkanSmoke()),
                new LinearLayout.LayoutParams(-1, dp(52)));

        status = bodyText("Готово к настройке.");
        status.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        status.setPadding(dp(2), dp(10), dp(2), dp(14));
        content.addView(status, matchWrap());
        return scrollPage(content);
    }

    private View buildSettingsPage() {
        LinearLayout content = pageContent();
        addSectionTitle(content, OptionCatalog.SETTINGS_SECTIONS[0]);
        content.addView(bodyText("Для игры используется OpenGL ES. Vulkan доступен для проверки."),
                matchWrap());
        rendererMode = new Spinner(this);
        ArrayAdapter<String> rendererAdapter = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, OptionCatalog.RENDERER_LABELS);
        rendererAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        rendererMode.setAdapter(rendererAdapter);
        content.addView(rendererMode, matchWrap());

        addSectionTitle(content, OptionCatalog.SETTINGS_SECTIONS[1]);
        content.addView(bodyText("Автоматический профиль: Low."),
                matchWrap());
        graphicsPreset = new Spinner(this);
        ArrayAdapter<String> graphicsAdapter = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, OptionCatalog.GRAPHICS_LABELS);
        graphicsAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        graphicsPreset.setAdapter(graphicsAdapter);
        content.addView(graphicsPreset, matchWrap());

        addSectionTitle(content, OptionCatalog.SETTINGS_SECTIONS[2]);
        content.addView(bodyText("Снижение разрешения ускоряет рендеринг, но уменьшает чёткость."), matchWrap());
        buildRenderResolutionList();
        renderResolution = new Spinner(this);
        ArrayAdapter<RenderResolution> resolutionAdapter = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, renderResolutions);
        resolutionAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        renderResolution.setAdapter(resolutionAdapter);
        content.addView(renderResolution, matchWrap());

        addSectionTitle(content, OptionCatalog.SETTINGS_SECTIONS[3]);
        gamepadEnabled = makeCheckBox("Включить поддержку геймпада", "Подключённый контроллер.");
        touchControlsEnabled = makeCheckBox("Показывать сенсорное управление",
                "Экранный стик и кнопки движения, огня, взаимодействия и инвентаря. "
                        + "Свободная область работает как мышь.");
        splashEnabled = makeCheckBox("Показывать заставку OpenXRay", "Заставка при запуске.");
        keepScreenOn = makeCheckBox("Не выключать экран во время игры",
                "Предотвращает системную блокировку при загрузке.");
        immersiveMode = makeCheckBox("Полноэкранный режим Android",
                "Скрывает системные панели; жест от края временно возвращает их.");
        showFps = makeCheckBox("Показывать FPS", "Счётчик кадров во время игры.");
        content.addView(gamepadEnabled, matchWrap());
        content.addView(touchControlsEnabled, matchWrap());
        content.addView(splashEnabled, matchWrap());
        content.addView(keepScreenOn, matchWrap());
        content.addView(immersiveMode, matchWrap());
        content.addView(showFps, matchWrap());

        addSectionTitle(content, OptionCatalog.SETTINGS_SECTIONS[4]);
        advancedOptions = new CheckBox[OptionCatalog.ADVANCED_KEYS.length];
        for (int index = 0; index < advancedOptions.length; ++index) {
            advancedOptions[index] = makeCheckBox(OptionCatalog.ADVANCED_LABELS[index],
                    OptionCatalog.ADVANCED_DESCRIPTIONS[index]);
            content.addView(advancedOptions[index], matchWrap());
        }

        content.addView(actionButton(OptionCatalog.ADVANCED_RESET_LABEL, view -> {
            for (CheckBox option : advancedOptions)
                option.setChecked(false);
            savePreferences();
        }), matchWrap());

        addSectionTitle(content, OptionCatalog.SETTINGS_SECTIONS[5]);
        content.addView(bodyText("Необязательные параметры движка."), matchWrap());
        customArgs = new EditText(this);
        customArgs.setHint("Например: -novtf");
        customArgs.setMinLines(2);
        customArgs.setGravity(Gravity.TOP | Gravity.START);
        content.addView(customArgs, matchWrap());

        content.addView(actionButton("Сохранить параметры", view -> {
            savePreferences();
            setStatus("Параметры сохранены.");
            showPage(PAGE_GAME);
        }), new LinearLayout.LayoutParams(-1, dp(52)));
        content.addView(actionButton("Сбросить настройки лаунчера", view -> confirmResetPreferences()),
                new LinearLayout.LayoutParams(-1, dp(52)));

        addSectionTitle(content, "Обновления");
        content.addView(bodyText("Проверка опубликованных версий OpenXRay для Android. "
                + "Перед установкой APK проверяется по SHA-256."), matchWrap());
        updateStatus = bodyText("Установлена версия " + BuildConfig.VERSION_NAME + ".");
        content.addView(updateStatus, matchWrap());
        updateButton = actionButton("Проверить обновления", view -> checkForUpdates());
        content.addView(updateButton, new LinearLayout.LayoutParams(-1, dp(52)));
        return scrollPage(content);
    }

    private void checkForUpdates() {
        if (updateButton != null)
            updateButton.setEnabled(false);
        setUpdateStatus("Проверяю опубликованные релизы…");
        updateExecutor.execute(() -> {
            AppUpdater.Update update = null;
            String error = null;
            try {
                update = AppUpdater.checkForUpdate(BuildConfig.VERSION_CODE);
            } catch (IOException exception) {
                error = exception.getMessage();
            }

            final AppUpdater.Update availableUpdate = update;
            final String checkError = error;
            handler.post(() -> {
                if (isFinishing())
                    return;
                if (updateButton != null)
                    updateButton.setEnabled(true);
                if (checkError != null) {
                    setUpdateStatus("Не удалось проверить обновления: " + checkError);
                    return;
                }
                if (availableUpdate == null) {
                    setUpdateStatus("Обновлений нет. Установлена последняя опубликованная версия.");
                    return;
                }

                String message = "Доступна версия " + availableUpdate.versionName + ".\n\n"
                        + (availableUpdate.releaseNotes.isEmpty()
                                ? "APK будет проверен по SHA-256 перед установкой."
                                : availableUpdate.releaseNotes);
                new AlertDialog.Builder(this)
                        .setTitle("Доступно обновление")
                        .setMessage(message)
                        .setPositiveButton("Установить", (dialog, which) -> installUpdate(availableUpdate))
                        .setNegativeButton("Позже", (dialog, which) ->
                                setUpdateStatus("Доступна версия " + availableUpdate.versionName + "."))
                        .show();
            });
        });
    }

    private void installUpdate(AppUpdater.Update update) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                && !getPackageManager().canRequestPackageInstalls()) {
            pendingUpdate = update;
            try {
                Intent settingsIntent = new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                        Uri.parse("package:" + getPackageName()));
                startActivity(settingsIntent);
                setUpdateStatus("Разрешите установку из OpenXRay. После возврата начнётся загрузка.");
            } catch (ActivityNotFoundException error) {
                pendingUpdate = null;
                setUpdateStatus("Android не открыл разрешение на установку приложений.");
            }
            return;
        }

        pendingUpdate = null;
        if (updateButton != null)
            updateButton.setEnabled(false);
        setUpdateStatus("Загружаю и проверяю APK версии " + update.versionName + "…");
        updateExecutor.execute(() -> {
            File apk = null;
            String error = null;
            try {
                apk = AppUpdater.download(this, update);
                installDownloadedApk(apk);
            } catch (IOException | RuntimeException exception) {
                error = exception.getMessage();
            }

            final String installError = error;
            handler.post(() -> {
                if (isFinishing())
                    return;
                if (updateButton != null)
                    updateButton.setEnabled(true);
                if (installError != null) {
                    setUpdateStatus("Не удалось установить обновление: " + installError);
                } else {
                    setUpdateStatus("APK версии " + update.versionName
                            + " проверен. Подтвердите установку в системном окне Android.");
                }
            });
        });
    }

    private void installDownloadedApk(File apk) throws IOException {
        PackageInstaller installer = getPackageManager().getPackageInstaller();
        PackageInstaller.SessionParams parameters = new PackageInstaller.SessionParams(
                PackageInstaller.SessionParams.MODE_FULL_INSTALL);
        parameters.setAppPackageName(getPackageName());
        parameters.setSize(apk.length());

        int sessionId = installer.createSession(parameters);
        PackageInstaller.Session session = installer.openSession(sessionId);
        try {
            try (InputStream input = new FileInputStream(apk);
                    OutputStream output = session.openWrite("openxray-update.apk", 0, apk.length())) {
                byte[] buffer = new byte[32 * 1024];
                int count;
                while ((count = input.read(buffer)) != -1)
                    output.write(buffer, 0, count);
                session.fsync(output);
            }

            Intent statusIntent = new Intent(this, UpdateInstallReceiver.class)
                    .setAction(UpdateInstallReceiver.ACTION_INSTALL_STATUS);
            PendingIntent statusPendingIntent = PendingIntent.getBroadcast(this, sessionId, statusIntent,
                    PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);
            session.commit(statusPendingIntent.getIntentSender());
        } catch (IOException | RuntimeException error) {
            session.abandon();
            throw error;
        } finally {
            session.close();
        }
    }

    private void setUpdateStatus(String message) {
        if (updateStatus != null)
            updateStatus.setText(message);
    }

    private View buildDiagnosticsPage() {
        LinearLayout content = pageContent();
        addSectionTitle(content, "Журнал движка");
        LinearLayout actions = horizontalRow();
        actions.addView(actionButton("Обновить", view -> refreshLog()), weightedButton());
        actions.addView(actionButton("Поделиться", view -> shareLogs()), weightedButton());
        actions.addView(actionButton("Очистить", view -> clearLogs()), weightedButton());
        content.addView(actions, matchWrap());

        TextView help = bodyText(
                "Показываются native-лог OpenXRay и события Android Activity. Полный logcat полезен "
                        + "для ошибок драйвера или системного завершения процесса.");
        help.setPadding(0, dp(8), 0, dp(8));
        content.addView(help, matchWrap());

        logView = new TextView(this);
        logView.setTextSize(11);
        logView.setTypeface(Typeface.MONOSPACE);
        logView.setTextIsSelectable(true);
        logView.setPadding(dp(8), dp(8), dp(8), dp(16));
        content.addView(logView, matchWrap());
        return scrollPage(content);
    }

    private LinearLayout pageContent() {
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(0, dp(8), 0, dp(16));
        return content;
    }

    private View scrollPage(LinearLayout content) {
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.addView(content, new ScrollView.LayoutParams(-1, -2));
        return scroll;
    }

    private LinearLayout horizontalRow() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        return row;
    }

    private Button actionButton(String text, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setText(text);
        button.setAllCaps(false);
        button.setOnClickListener(listener);
        return button;
    }

    private CheckBox makeCheckBox(String title, String description) {
        CheckBox checkBox = new CheckBox(this);
        checkBox.setText(title + "\n" + description);
        checkBox.setPadding(0, dp(5), 0, dp(5));
        return checkBox;
    }

    private TextView bodyText(String value) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(14);
        view.setLineSpacing(0, 1.08f);
        return view;
    }

    private void addSectionTitle(LinearLayout parent, String value) {
        TextView title = new TextView(this);
        title.setText(value);
        title.setTextSize(18);
        title.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        title.setPadding(0, dp(14), 0, dp(7));
        parent.addView(title, matchWrap());
    }

    private void restorePreferences() {
        int restoredVariant = clampVariant(preferences.getInt(PREF_GAME_VARIANT, 3));
        activeGameVariant = restoredVariant;
        suppressProfileCallbacks = true;
        gameVariant.setSelection(restoredVariant);
        suppressProfileCallbacks = false;
        String legacyPath = preferences.getString(PREF_GAME_PATH, "/storage/emulated/0/STALKER");
        gamePath.setText(preferences.getString(
                profilePreference(PREF_GAME_PATH_PREFIX, restoredVariant), legacyPath));
        customArgs.setText(preferences.getString(PREF_CUSTOM_ARGS, ""));
        gamepadEnabled.setChecked(preferences.getBoolean(PREF_GAMEPAD, false));
        touchControlsEnabled.setChecked(preferences.getBoolean(PREF_TOUCH_CONTROLS, true));
        splashEnabled.setChecked(preferences.getBoolean(PREF_SPLASH, false));
        keepScreenOn.setChecked(preferences.getBoolean(PREF_KEEP_SCREEN_ON, true));
        immersiveMode.setChecked(preferences.getBoolean(PREF_IMMERSIVE, true));
        rendererMode.setSelection(clampRendererMode(preferences.getInt(PREF_RENDERER_MODE, RENDERER_AUTO)));
        graphicsPreset.setSelection(clampGraphicsPreset(
                preferences.getInt(PREF_GRAPHICS_PRESET, GRAPHICS_AUTO)));
        selectStoredResolution(preferences.getString(PREF_RENDER_RESOLUTION, "auto"));
        showFps.setChecked(preferences.getBoolean(PREF_SHOW_FPS, true));
        for (int index = 0; index < advancedOptions.length; ++index)
            advancedOptions[index].setChecked(preferences.getBoolean(
                    "advanced_" + OptionCatalog.ADVANCED_KEYS[index], false));
        showPage(preferences.getInt(PREF_ACTIVE_PAGE, PAGE_GAME));
    }

    private void savePreferences() {
        if (preferences == null || gamePath == null)
            return;
        preferences.edit()
                .putString(PREF_GAME_PATH, gamePath.getText().toString().trim())
                .putString(profilePreference(PREF_GAME_PATH_PREFIX, activeGameVariant),
                        gamePath.getText().toString().trim())
                .putString(PREF_CUSTOM_ARGS, customArgs.getText().toString())
                .putInt(PREF_GAME_VARIANT, activeGameVariant)
                .putBoolean(PREF_GAMEPAD, gamepadEnabled.isChecked())
                .putBoolean(PREF_TOUCH_CONTROLS, touchControlsEnabled.isChecked())
                .putBoolean(PREF_SPLASH, splashEnabled.isChecked())
                .putBoolean(PREF_KEEP_SCREEN_ON, keepScreenOn.isChecked())
                .putBoolean(PREF_IMMERSIVE, immersiveMode.isChecked())
                .putInt(PREF_RENDERER_MODE, rendererMode.getSelectedItemPosition())
                .putInt(PREF_GRAPHICS_PRESET, graphicsPreset.getSelectedItemPosition())
                .putString(PREF_RENDER_RESOLUTION, selectedRenderResolution().id)
                .putBoolean(PREF_SHOW_FPS, showFps.isChecked())
                .putInt(PREF_ACTIVE_PAGE, activePage)
                .apply();
        SharedPreferences.Editor advancedEditor = preferences.edit();
        for (int index = 0; index < advancedOptions.length; ++index)
            advancedEditor.putBoolean("advanced_" + OptionCatalog.ADVANCED_KEYS[index],
                    advancedOptions[index].isChecked());
        advancedEditor.apply();
    }

    private int clampVariant(int value) {
        return value >= 0 && value < OptionCatalog.GAME_LABELS.length ? value : 3;
    }

    private int clampRendererMode(int value) {
        return value >= 0 && value < OptionCatalog.RENDERER_LABELS.length ? value : RENDERER_AUTO;
    }

    private int clampGraphicsPreset(int value) {
        return value >= 0 && value < OptionCatalog.GRAPHICS_LABELS.length ? value : GRAPHICS_AUTO;
    }

    private void buildRenderResolutionList() {
        renderResolutions.clear();
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getRealMetrics(metrics);
        int nativeWidth = Math.max(metrics.widthPixels, metrics.heightPixels);
        int nativeHeight = Math.min(metrics.widthPixels, metrics.heightPixels);
        if (nativeWidth <= 0 || nativeHeight <= 0) {
            nativeWidth = 1280;
            nativeHeight = 720;
        }

        int autoWidth = Math.min(nativeWidth, 1280);
        int autoHeight = aspectHeight(autoWidth, nativeWidth, nativeHeight);
        renderResolutions.add(new RenderResolution("auto",
                "Автоматически — " + autoWidth + "×" + autoHeight, autoWidth, autoHeight));

        for (int width : OptionCatalog.RESOLUTION_WIDTHS) {
            if (width >= nativeWidth)
                continue;
            int height = aspectHeight(width, nativeWidth, nativeHeight);
            addRenderResolution(width + "x" + height, width + "×" + height, width, height);
        }
        addRenderResolution("native", "Нативное — " + nativeWidth + "×" + nativeHeight,
                nativeWidth, nativeHeight);
    }

    private int aspectHeight(int width, int nativeWidth, int nativeHeight) {
        int height = Math.max(320, Math.round((float) width * nativeHeight / nativeWidth));
        return height & ~1;
    }

    private void addRenderResolution(String id, String label, int width, int height) {
        for (RenderResolution item : renderResolutions) {
            if (item.width == width && item.height == height)
                return;
        }
        renderResolutions.add(new RenderResolution(id, label, width, height));
    }

    private void selectStoredResolution(String id) {
        for (int index = 0; index < renderResolutions.size(); ++index) {
            if (renderResolutions.get(index).id.equals(id)) {
                renderResolution.setSelection(index);
                return;
            }
        }
        renderResolution.setSelection(0);
    }

    private RenderResolution selectedRenderResolution() {
        int index = renderResolution != null ? renderResolution.getSelectedItemPosition() : 0;
        if (index < 0 || index >= renderResolutions.size())
            index = 0;
        return renderResolutions.get(index);
    }

    private String profilePreference(String prefix, int variant) {
        return prefix + clampVariant(variant);
    }

    private String profileName(int variant) {
        return OptionCatalog.GAME_NAMES[clampVariant(variant)];
    }

    private void switchGameProfile(int requestedVariant) {
        int nextVariant = clampVariant(requestedVariant);
        if (nextVariant == activeGameVariant)
            return;

        preferences.edit()
                .putString(profilePreference(PREF_GAME_PATH_PREFIX, activeGameVariant),
                        gamePath.getText().toString().trim())
                .putInt(PREF_GAME_VARIANT, nextVariant)
                .apply();
        activeGameVariant = nextVariant;
        gamePath.setText(preferences.getString(
                profilePreference(PREF_GAME_PATH_PREFIX, nextVariant), ""));
        refreshGameInspection();
        setStatus("Выбран профиль: " + profileName(nextVariant) + ".");
    }

    private void confirmResetPreferences() {
        new AlertDialog.Builder(this)
                .setTitle("Сбросить настройки лаунчера?")
                .setMessage("Будут забыты пути и параметры только этого лаунчера. Файлы игр, модов, "
                        + "fsgame.ltx и user.ltx останутся без изменений.")
                .setPositiveButton("Сбросить", (dialog, which) -> {
                    preferences.edit().clear().apply();
                    restorePreferences();
                    refreshAccessStatus();
                    refreshGameInspection();
                    setStatus("Настройки лаунчера сброшены. Файлы игры не изменялись.");
                    showPage(PAGE_GAME);
                })
                .setNegativeButton("Отмена", null)
                .show();
    }

    private void showPage(int requestedPage) {
        int page = requestedPage >= PAGE_GAME && requestedPage <= PAGE_DIAGNOSTICS
                ? requestedPage : PAGE_GAME;
        activePage = page;
        if (pages == null)
            return;
        for (int index = 0; index < pages.length; ++index) {
            boolean selected = index == page;
            pages[index].setVisibility(selected ? View.VISIBLE : View.GONE);
            tabButtons[index].setEnabled(!selected);
            tabButtons[index].setTypeface(Typeface.DEFAULT,
                    selected ? Typeface.BOLD : Typeface.NORMAL);
        }
        if (page == PAGE_DIAGNOSTICS)
            refreshLog();
    }

    private void chooseGameFolder() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
        try {
            startActivityForResult(intent, REQUEST_GAME_TREE);
        } catch (ActivityNotFoundException error) {
            setStatus("На устройстве нет системного выбора папки: " + error.getMessage());
        }
    }

    private void requestAllFilesAccess() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                && Build.VERSION.SDK_INT < Build.VERSION_CODES.R
                && requestLegacyStoragePermissions()) {
            return;
        }

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            setStatus("На этой версии Android отдельный All files access не требуется.");
            return;
        }
        if (Environment.isExternalStorageManager()) {
            setStatus("Доступ ко всей памяти уже выдан.");
            return;
        }

        try {
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                    Uri.parse("package:" + getPackageName()));
            startActivity(intent);
        } catch (SecurityException | ActivityNotFoundException error) {
            try {
                startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
            } catch (SecurityException | ActivityNotFoundException fallbackError) {
                setStatus("Android не смог открыть страницу доступа к файлам: "
                        + fallbackError.getMessage());
            }
        }
    }

    private boolean requestLegacyStoragePermissions() {
        ArrayList<String> missing = new ArrayList<>();
        if (checkSelfPermission(android.Manifest.permission.READ_EXTERNAL_STORAGE)
                != android.content.pm.PackageManager.PERMISSION_GRANTED)
            missing.add(android.Manifest.permission.READ_EXTERNAL_STORAGE);
        if (checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != android.content.pm.PackageManager.PERMISSION_GRANTED)
            missing.add(android.Manifest.permission.WRITE_EXTERNAL_STORAGE);
        if (missing.isEmpty())
            return false;
        requestPermissions(missing.toArray(new String[0]), REQUEST_STORAGE_PERMISSIONS);
        return true;
    }

    private void showStorageAccessPromptIfNeeded() {
        if (hasStorageAccess())
            return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            new AlertDialog.Builder(this)
                    .setTitle("Нужен доступ к файлам")
                    .setMessage("OpenXRay читает оригинальные файлы STALKER непосредственно из выбранной папки. "
                            + "На Android 11 и новее включите «Разрешить управление всеми файлами» для приложения.")
                    .setPositiveButton("Открыть настройки", (dialog, which) -> requestAllFilesAccess())
                    .setNegativeButton("Позже", (dialog, which) ->
                            setStatus("Доступ пока не выдан. Его можно включить на вкладке «Игра»."))
                    .show();
        } else {
            requestAllFilesAccess();
        }
    }

    private boolean hasStorageAccess() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
            return Environment.isExternalStorageManager();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
            return checkSelfPermission(android.Manifest.permission.READ_EXTERNAL_STORAGE)
                    == android.content.pm.PackageManager.PERMISSION_GRANTED
                    && checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    == android.content.pm.PackageManager.PERMISSION_GRANTED;
        return true;
    }

    private void launchEngine(boolean rendererSmoke) {
        launchEngine(rendererSmoke, false);
    }

    private void launchVulkanSmoke() {
        launchEngine(true, true);
    }

    private void launchEngine(boolean rendererSmoke, boolean vulkanRendererSmoke) {
        ++stopGeneration; // Cancel pending retries before any new launch or reattach.
        if (!rendererSmoke && isEngineProcessRunning()) {
            setStatus("Возвращаю уже запущенный движок на экран…");
            Intent resume = new Intent(this, XRayActivity.class);
            resume.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT | Intent.FLAG_ACTIVITY_SINGLE_TOP);
            try {
                startActivity(resume);
            } catch (RuntimeException error) {
                setStatus("Не удалось вернуть игру: " + error.getMessage());
            }
            return;
        }
        if (!rendererSmoke && !prepareEngineLaunch())
            return;

        String[] additionalArgs;
        try {
            additionalArgs = parseAdditionalArguments(customArgs.getText().toString());
            List<String> selectedArgs = new ArrayList<>();
            for (String argument : additionalArgs)
                selectedArgs.add(argument);
            for (int index = 0; index < advancedOptions.length; ++index)
                if (advancedOptions[index].isChecked())
                    selectedArgs.add(OptionCatalog.ADVANCED_ARGS[index]);
            additionalArgs = selectedArgs.toArray(new String[0]);
        } catch (IllegalArgumentException error) {
            setStatus("Ошибка в дополнительных аргументах: " + error.getMessage());
            showPage(PAGE_SETTINGS);
            return;
        }

        savePreferences();
        String selectedPath = gamePath.getText().toString().trim();
        SessionLogs.Session logSession;
        try {
            logSession = SessionLogs.start(this, rendererSmoke ? null : selectedPath);
        } catch (IOException | SecurityException error) {
            setStatus("Не удалось создать журналы в папке игры: " + error.getMessage());
            return;
        }
        writeLauncherLog("[launcher] starting "
                + (rendererSmoke ? "renderer smoke test" : "OpenXRay; game root=" + selectedPath),
                logSession.activity);
        engineLaunchTime = SystemClock.elapsedRealtime();
        engineFailureToastShown = false;

        Intent intent = createEngineIntent(rendererSmoke, vulkanRendererSmoke);
        intent.putExtra(EXTRA_ENGINE_LOG, logSession.engine.getAbsolutePath());
        intent.putExtra(EXTRA_ACTIVITY_LOG, logSession.activity.getAbsolutePath());
        intent.putExtra(EXTRA_ADDITIONAL_ARGS, additionalArgs);
        String launchStatus;
        if (vulkanRendererSmoke) {
            launchStatus = "Запускаю Vulkan probe и GLES fallback…";
        } else if (rendererSmoke) {
            launchStatus = "Запускаю GLES smoke test…";
        } else {
            launchStatus = "Запускаю OpenXRay: "
                    + OptionCatalog.value(OptionCatalog.RENDERER_LABELS,
                            clampRendererMode(rendererMode.getSelectedItemPosition()), 0)
                    + ", качество: "
                    + OptionCatalog.value(OptionCatalog.GRAPHICS_LABELS,
                            clampGraphicsPreset(graphicsPreset.getSelectedItemPosition()), 0);
        }
        setStatus(launchStatus);
        Toast.makeText(this, "OpenXRay: запуск движка…", Toast.LENGTH_SHORT).show();
        try {
            startActivity(intent);
        } catch (RuntimeException error) {
            setStatus("Не удалось запустить процесс движка: " + error.getMessage());
            Toast.makeText(this, "OpenXRay: не удалось запустить движок", Toast.LENGTH_LONG).show();
        }
    }

    private Intent createEngineIntent(boolean rendererSmoke) {
        return createEngineIntent(rendererSmoke, false);
    }

    private Intent createEngineIntent(boolean rendererSmoke, boolean vulkanRendererSmoke) {
        Intent intent = new Intent(this, XRayActivity.class);
        intent.putExtra(EXTRA_RENDERER_SMOKE, rendererSmoke);
        intent.putExtra(EXTRA_RENDERER_VULKAN_SMOKE, vulkanRendererSmoke);
        intent.putExtra(EXTRA_GAMEPAD_ENABLED, gamepadEnabled.isChecked());
        intent.putExtra(EXTRA_TOUCH_CONTROLS, touchControlsEnabled.isChecked());
        intent.putExtra(EXTRA_SPLASH_ENABLED, splashEnabled.isChecked());
        intent.putExtra(EXTRA_KEEP_SCREEN_ON, keepScreenOn.isChecked());
        intent.putExtra(EXTRA_IMMERSIVE, immersiveMode.isChecked());
        intent.putExtra(EXTRA_RENDERER_MODE,
                clampRendererMode(rendererMode.getSelectedItemPosition()));
        intent.putExtra(EXTRA_GRAPHICS_PRESET,
                clampGraphicsPreset(graphicsPreset.getSelectedItemPosition()));
        RenderResolution resolution = selectedRenderResolution();
        intent.putExtra(EXTRA_RENDER_WIDTH, resolution.width);
        intent.putExtra(EXTRA_RENDER_HEIGHT, resolution.height);
        intent.putExtra(EXTRA_SHOW_FPS, showFps.isChecked());
        if (!rendererSmoke) {
            intent.putExtra(EXTRA_GAME_PATH, gamePath.getText().toString().trim());
            intent.putExtra(EXTRA_GAME_VARIANT, activeGameVariant);
        }
        return intent;
    }

    private String[] parseAdditionalArguments(String commandLine) {
        ArrayList<String> result = new ArrayList<>();
        StringBuilder token = new StringBuilder();
        char quote = 0;
        boolean escaped = false;
        boolean tokenStarted = false;

        for (int index = 0; index < commandLine.length(); ++index) {
            char current = commandLine.charAt(index);
            if (escaped) {
                token.append(current);
                escaped = false;
                tokenStarted = true;
                continue;
            }
            if (current == '\\') {
                escaped = true;
                tokenStarted = true;
                continue;
            }
            if (quote != 0) {
                if (current == quote)
                    quote = 0;
                else
                    token.append(current);
                tokenStarted = true;
                continue;
            }
            if (current == '\'' || current == '"') {
                quote = current;
                tokenStarted = true;
                continue;
            }
            if (Character.isWhitespace(current)) {
                if (tokenStarted) {
                    addCheckedArgument(result, token.toString());
                    token.setLength(0);
                    tokenStarted = false;
                }
                continue;
            }
            token.append(current);
            tokenStarted = true;
        }

        if (escaped)
            throw new IllegalArgumentException("последний символ '\\' не экранирует аргумент");
        if (quote != 0)
            throw new IllegalArgumentException("не закрыта кавычка");
        if (tokenStarted)
            addCheckedArgument(result, token.toString());
        return result.toArray(new String[0]);
    }

    private void addCheckedArgument(List<String> target, String argument) {
        String normalized = argument.toLowerCase(Locale.US);
        if (normalized.equals("-android-game-root-hex")
                || normalized.equals("-renderer-smoke")
                || normalized.equals("-renderer-vulkan-smoke")
                || normalized.equals("-renderer-auto")
                || normalized.equals("-renderer-gles")
                || normalized.equals("-renderer-vulkan")
                || normalized.equals("-android-render-scale")
                || normalized.equals("-android-mobile-preset")
                || normalized.equals("-android-show-fps")
                || normalized.equals("-headless-smoke")
                || normalized.equals("-nogame")
                || normalized.equals("-soc")
                || normalized.equals("-shoc")
                || normalized.equals("-cs")
                || normalized.equals("-cop")) {
            throw new IllegalArgumentException("зарезервированный аргумент " + argument);
        }
        target.add(argument);
    }

    private boolean prepareEngineLaunch() {
        if (!hasStorageAccess()) {
            writeLauncherLog("[launcher] storage access is missing; game root was not passed to the engine");
            setStatus("Сначала включите доступ ко всей памяти.");
            return false;
        }

        try {
            String selectedPath = gamePath.getText().toString().trim();
            if (selectedPath.isEmpty()) {
                setStatus("Папка STALKER не выбрана.");
                return false;
            }
            File root = new File(selectedPath);
            if (!root.isDirectory() || !root.canRead()) {
                writeLauncherLog("[launcher] selected game root is not a readable directory: " + selectedPath);
                setStatus("Выбранный путь не является доступной для чтения папкой.");
                refreshGameInspection();
                return false;
            }
            try {
                prepareWritableAppData(root);
            } catch (IOException | SecurityException error) {
                writeLauncherLog("[launcher] _appdata_ write check failed: "
                        + error.getClass().getSimpleName() + ": " + error.getMessage());
                setStatus("Нет записи в папку STALKER/_appdata_: " + error.getMessage());
                showPage(PAGE_GAME);
                return false;
            }
            if (!prepareBundledEngineData()) {
                writeLauncherLog("[launcher] bundled OpenXRay engine data is unavailable");
                setStatus("Не удалось подготовить внутренние данные рендера. Смотрите диагностику.");
                return false;
            }
            writeLauncherLog("[launcher] game root and writable _appdata_ are ready: " + selectedPath);
            return true;
        } catch (IOException | SecurityException error) {
            writeLauncherLog("[launcher] cannot prepare engine data: "
                    + error.getClass().getSimpleName() + ": " + error.getMessage());
            setStatus("Не удалось подготовить внутренние данные движка: " + error.getMessage());
            return false;
        }
    }

    private void prepareWritableAppData(File gameRoot) throws IOException {
        File appData = new File(gameRoot, "_appdata_");
        ensureDirectory(appData);
        File saves = new File(appData, "savedgames");
        File screenshots = new File(appData, "screenshots");
        File logs = new File(appData, "logs");
        ensureDirectory(saves);
        ensureDirectory(screenshots);
        ensureDirectory(logs);

        probeWritableDirectory(appData);
        probeWritableDirectory(saves);
        probeWritableDirectory(screenshots);
        probeWritableDirectory(logs);

        writeLauncherLog("[launcher] verified writable app data: " + appData.getAbsolutePath());
    }

    private void probeWritableDirectory(File directory) throws IOException {
        File probe = File.createTempFile(".openxray-write-test-", ".tmp", directory);
        boolean deleted = false;
        try (FileOutputStream output = new FileOutputStream(probe, false)) {
            output.write("OpenXRay Android write test\n".getBytes(StandardCharsets.UTF_8));
            output.flush();
        } finally {
            deleted = !probe.exists() || probe.delete();
        }
        if (!deleted)
            throw new IOException("не удалось удалить проверочный файл " + probe.getAbsolutePath());
    }

    private void ensureDirectory(File directory) throws IOException {
        if (directory.isDirectory())
            return;
        if (directory.exists())
            throw new IOException(directory.getAbsolutePath() + " существует, но это не папка");
        if (!directory.mkdirs() && !directory.isDirectory())
            throw new IOException("не удалось создать " + directory.getAbsolutePath());
    }

    private boolean prepareBundledEngineData() throws IOException {
        File privateRoot = new File(getFilesDir(), "openxray");
        File destination = new File(privateRoot, "engine-gamedata");
        File marker = new File(privateRoot, "engine-data.version");
        String desiredVersion = Integer.toString(BuildConfig.VERSION_CODE);

        if (desiredVersion.equals(readSmallTextFile(marker)) && isCompleteEngineData(destination)) {
            writeLauncherLog("[launcher] OpenXRay engine data is current: " + destination.getAbsolutePath());
            return true;
        }
        if (!privateRoot.exists() && !privateRoot.mkdirs())
            throw new IOException("cannot create " + privateRoot);

        File staging = new File(privateRoot, "engine-gamedata.new");
        File backup = new File(privateRoot, "engine-gamedata.old");
        deleteRecursively(staging);
        deleteRecursively(backup);
        copyBundledAssetTree("gamedata", staging);
        if (!isCompleteEngineData(staging)) {
            deleteRecursively(staging);
            writeLauncherLog("[launcher] bundled engine gamedata is incomplete");
            return false;
        }

        boolean hadDestination = destination.exists();
        if (hadDestination && !destination.renameTo(backup)) {
            deleteRecursively(staging);
            throw new IOException("cannot replace " + destination);
        }
        if (!staging.renameTo(destination)) {
            if (hadDestination)
                backup.renameTo(destination);
            throw new IOException("cannot activate staged engine data");
        }
        deleteRecursively(backup);
        writeSmallTextFileAtomically(marker, desiredVersion);
        writeLauncherLog("[launcher] OpenXRay engine data updated atomically: "
                + destination.getAbsolutePath());
        return true;
    }

    private boolean isCompleteEngineData(File directory) {
        File configs = new File(directory, "configs");
        File shaders = new File(directory, "shaders");
        return configs.isDirectory() && shaders.isDirectory()
                && hasDirectoryEntries(configs) && hasDirectoryEntries(shaders);
    }

    private boolean hasDirectoryEntries(File directory) {
        File[] entries = directory.listFiles();
        return entries != null && entries.length != 0;
    }

    private void copyBundledAssetTree(String assetPath, File destination) throws IOException {
        String[] children = getAssets().list(assetPath);
        if (children == null || children.length == 0) {
            File parent = destination.getParentFile();
            if (parent != null && !parent.exists() && !parent.mkdirs())
                throw new IOException("cannot create " + parent);
            try (InputStream input = getAssets().open(assetPath);
                 OutputStream output = new FileOutputStream(destination)) {
                byte[] buffer = new byte[8192];
                int count;
                while ((count = input.read(buffer)) != -1)
                    output.write(buffer, 0, count);
            }
            return;
        }
        if (!destination.exists() && !destination.mkdirs())
            throw new IOException("cannot create " + destination);
        for (String child : children)
            copyBundledAssetTree(assetPath + "/" + child, new File(destination, child));
    }

    private void deleteRecursively(File target) throws IOException {
        if (!target.exists())
            return;
        if (target.isDirectory()) {
            File[] children = target.listFiles();
            if (children == null)
                throw new IOException("cannot list " + target);
            for (File child : children)
                deleteRecursively(child);
        }
        if (!target.delete())
            throw new IOException("cannot delete " + target);
    }

    private String readSmallTextFile(File file) {
        if (!file.isFile() || file.length() > 128)
            return "";
        try (FileInputStream input = new FileInputStream(file)) {
            byte[] bytes = new byte[(int) file.length()];
            int count = input.read(bytes);
            return count > 0 ? new String(bytes, 0, count, StandardCharsets.UTF_8).trim() : "";
        } catch (IOException | SecurityException ignored) {
            return "";
        }
    }

    private void writeSmallTextFileAtomically(File file, String value) throws IOException {
        File temporary = new File(file.getParentFile(), file.getName() + ".new");
        try (FileOutputStream output = new FileOutputStream(temporary)) {
            output.write(value.getBytes(StandardCharsets.UTF_8));
            output.getFD().sync();
        }
        if (file.exists() && !file.delete())
            throw new IOException("cannot replace " + file);
        if (!temporary.renameTo(file))
            throw new IOException("cannot activate " + file);
    }

    private String resolvePrimaryStoragePath(Uri treeUri) {
        if (!DocumentsContract.isTreeUri(treeUri))
            return null;
        String documentId = DocumentsContract.getTreeDocumentId(treeUri);
        if (documentId == null || !documentId.startsWith("primary:"))
            return null;
        String relative = Uri.decode(documentId.substring("primary:".length()));
        return "/storage/emulated/0" + (relative.isEmpty() ? "" : "/" + relative);
    }

    private void refreshAccessStatus() {
        if (accessStatus == null)
            return;
        boolean granted = hasStorageAccess();
        accessStatus.setText("Доступ ко всей памяти: " + (granted ? "выдан" : "требуется"));
        accessStatus.setTextColor(granted ? Color.rgb(25, 115, 55) : Color.rgb(190, 70, 35));
    }

    private void refreshGameInspection() {
        if (gameInspection == null || gamePath == null)
            return;
        String selectedPath = gamePath.getText().toString().trim();
        if (selectedPath.isEmpty()) {
            gameInspection.setText("Профиль: " + profileName(activeGameVariant)
                    + ". Путь для этого профиля ещё не выбран.");
            gameInspection.setTextColor(Color.rgb(150, 95, 25));
            return;
        }
        File root = new File(selectedPath);
        if (!root.isDirectory()) {
            gameInspection.setText("Профиль: " + profileName(activeGameVariant)
                    + ". Папка не найдена или недоступна.");
            gameInspection.setTextColor(Color.rgb(190, 70, 35));
            return;
        }

        boolean fsgame = new File(root, "fsgame.ltx").isFile();
        boolean gamedata = new File(root, "gamedata").isDirectory();
        boolean resources = new File(root, "resources").isDirectory();
        File appData = new File(root, "_appdata_");
        boolean appDataWritable = appData.isDirectory() ? appData.canWrite() : root.canWrite();
        boolean socExecutable = new File(root, "bin/XR_3DA.exe").isFile()
                || new File(root, "bin/xr_3da.exe").isFile();
        boolean laterExecutable = new File(root, "bin/xrEngine.exe").isFile()
                || new File(root, "bin/xrengine.exe").isFile();
        String hint = socExecutable ? "похоже на Shadow of Chernobyl"
                : laterExecutable ? "обнаружена установка CS/CoP"
                : "точная игра будет определена выбранным профилем";
        String mismatch = socExecutable && activeGameVariant > 1
                ? " · профиль не совпадает с найденным XR_3DA.exe"
                : laterExecutable && activeGameVariant == 1
                ? " · профиль SoC не совпадает с найденным xrEngine.exe" : "";
        gameInspection.setText("Профиль: " + profileName(activeGameVariant)
                + " · папка читается · fsgame.ltx: " + yesNo(fsgame)
                + " · gamedata: " + yesNo(gamedata)
                + " · resources: " + yesNo(resources)
                + " · _appdata_ доступна для записи: " + yesNo(appDataWritable) + "\n" + hint
                + mismatch + ". Проверка информационная и не изменяет файлы.");
        gameInspection.setTextColor(fsgame && appDataWritable && mismatch.isEmpty()
                ? Color.rgb(25, 115, 55) : Color.rgb(150, 95, 25));
    }

    private String yesNo(boolean value) {
        return value ? "есть" : "нет";
    }

    private void refreshRunningState() {
        boolean running = isEngineProcessRunning();
        if (launchButton != null)
            launchButton.setText(running ? "Вернуться в запущенную игру" : "Запустить игру");
        if (stopButton != null) {
            stopButton.setEnabled(running);
            stopButton.setVisibility(running ? View.VISIBLE : View.GONE);
        }
    }

    private void confirmStopEngine() {
        if (!isEngineProcessRunning()) {
            refreshRunningState();
            setStatus("Процесс движка уже остановлен.");
            return;
        }
        new AlertDialog.Builder(this)
                .setTitle("Остановить движок?")
                .setMessage("Процесс игры будет принудительно завершён. Несохранённый прогресс потеряется.")
                .setPositiveButton("Остановить", (dialog, which) -> stopEngine())
                .setNegativeButton("Отмена", null)
                .show();
    }

    private void stopEngine() {
        setStatus("Останавливаю процесс движка…");
        stopEngineAttempt(0, ++stopGeneration, findEngineProcessPid());
    }

    private void stopEngineAttempt(int attempt, int generation, int requestedPid) {
        if (generation != stopGeneration)
            return;
        int enginePid = findEngineProcessPid();
        if (requestedPid > 0 && enginePid > 0 && requestedPid != enginePid)
            return; // Another session has started; never signal its PID.
        if (enginePid <= 0 && attempt > 0) {
            refreshRunningState();
            setStatus("Процесс движка остановлен.");
            return;
        }
        writeLauncherLog("[launcher] force-stop attempt=" + attempt + " for engine process; pid=" + enginePid);
        if (enginePid > 0) {
            // Both processes belong to this application UID, so the launcher
            // can terminate a wedged engine directly. An in-process broadcast
            // is not reliable when the engine main looper itself is stalled.
            android.os.Process.killProcess(enginePid);
        } else {
            Intent stop = new Intent(this, EngineControlReceiver.class);
            stop.setAction(EngineControlReceiver.ACTION_STOP_ENGINE);
            sendBroadcast(stop);
        }
        if (attempt < 3) {
            handler.postDelayed(() -> stopEngineAttempt(attempt + 1, generation, requestedPid),
                    500L * (attempt + 1));
        } else {
            refreshRunningState();
            if (isEngineProcessRunning())
                setStatus("Движок не завершился после SIGKILL. Отправьте журнал для диагностики.");
            else
                setStatus("Процесс движка остановлен.");
        }
    }

    private void refreshLog() {
        if (logView == null || logReadPending)
            return;
        logReadPending = true;
        logExecutor.execute(() -> {
            final String log = collectLogs();
            handler.post(() -> {
                logReadPending = false;
                cachedLog = log;
                if (isFinishing() || isDestroyed() || logView == null)
                    return;
                logView.setText(log.isEmpty()
                        ? "Лог пока пуст. Запустите GLES/Vulkan-проверку или игру." : log);
                updateEngineStatus(log);
            });
        });
    }

    private String collectLogs() {
        StringBuilder result = new StringBuilder();
        File[] session = SessionLogs.latest(this);
        for (File file : session) appendLog(result, file);
        return result.toString();
    }

    private void updateEngineStatus(String log) {
        if (engineLaunchTime == 0)
            return;
        if (log.contains("[renderer-vulkan] PASS: Vulkan command buffer")) {
            setStatus("Самостоятельный Vulkan render pass завершён: PASS.");
            return;
        }
        if (log.contains("[renderer-smoke] center pixel") && log.contains(": PASS")) {
            setStatus("GLES renderer smoke test завершён: PASS.");
            return;
        }
        if (log.contains("[android] engine loaded")) {
            setStatus("Движок загрузил игру. Если Activity была свёрнута, нажмите «Вернуться в запущенную игру».");
            return;
        }
        if (log.contains("[renderer-smoke] initialization failed") || log.contains("engine load failed")) {
            showEngineFailureStatus();
            return;
        }
        long elapsed = SystemClock.elapsedRealtime() - engineLaunchTime;
        if (elapsed > 4000 && !isEngineProcessRunning())
            showEngineFailureStatus();
    }

    private void showEngineFailureStatus() {
        setStatus("Процесс движка завершился до штатной загрузки. Откройте вкладку «Диагностика».");
        if (!engineFailureToastShown) {
            Toast.makeText(this, "OpenXRay: загрузка не удалась; смотрите лог", Toast.LENGTH_LONG).show();
            engineFailureToastShown = true;
        }
    }

    private boolean isEngineProcessRunning() {
        return findEngineProcessPid() > 0;
    }

    private int findEngineProcessPid() {
        ActivityManager manager = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
        if (manager == null)
            return -1;
        String engineProcess = getPackageName() + ":engine";
        List<ActivityManager.RunningAppProcessInfo> processes = manager.getRunningAppProcesses();
        if (processes == null)
            return -1;
        for (ActivityManager.RunningAppProcessInfo process : processes) {
            if (engineProcess.equals(process.processName))
                return process.pid;
        }
        return -1;
    }

    private void appendLog(StringBuilder result, File file) {
        if (!file.isFile())
            return;
        String text = readTail(file);
        if (!text.isEmpty())
            result.append("\n===== ").append(file.getAbsolutePath()).append(" =====\n").append(text);
    }

    private String readTail(File file) {
        try (FileInputStream input = new FileInputStream(file)) {
            long skip = Math.max(0, file.length() - MAX_LOG_BYTES);
            while (skip > 0) {
                long skipped = input.skip(skip);
                if (skipped <= 0)
                    break;
                skip -= skipped;
            }
            byte[] data = new byte[(int) Math.min(file.length(), MAX_LOG_BYTES)];
            int size = input.read(data);
            return size > 0 ? new String(data, 0, size, StandardCharsets.UTF_8) : "";
        } catch (IOException | SecurityException error) {
            return "Не удалось прочитать " + file + ": " + error.getMessage();
        }
    }

    private void shareLogs() {
        logExecutor.execute(() -> {
            String collected = collectLogs();
            if (collected.length() > MAX_SHARED_LOG_CHARS)
                collected = collected.substring(collected.length() - MAX_SHARED_LOG_CHARS);
            final String log = collected;
            handler.post(() -> {
                if (log.isEmpty()) {
                    Toast.makeText(this, "Лог пока пуст", Toast.LENGTH_SHORT).show();
                    return;
                }
                Intent share = new Intent(Intent.ACTION_SEND);
                share.setType("text/plain");
                share.putExtra(Intent.EXTRA_SUBJECT,
                        "OpenXRay Android " + BuildConfig.VERSION_NAME + " logs");
                share.putExtra(Intent.EXTRA_TEXT, log);
                try {
                    startActivity(Intent.createChooser(share, "Поделиться логом OpenXRay"));
                } catch (ActivityNotFoundException error) {
                    Toast.makeText(this, "Нет приложения для отправки текста", Toast.LENGTH_LONG).show();
                }
            });
        });
    }

    private void clearLogs() {
        if (isEngineProcessRunning()) {
            Toast.makeText(this, "Закройте игру перед очисткой текущего журнала", Toast.LENGTH_LONG).show();
            return;
        }
        for (File file : SessionLogs.latest(this)) deleteLog(file);
        refreshLog();
    }

    private void writeLauncherLog(String message) {
        File[] session = SessionLogs.latest(this);
        writeLauncherLog(message, session.length != 0 ? session[1] : null);
    }

    private void writeLauncherLog(String message, File sessionFile) {
        if (sessionFile == null) {
            String gameRoot = gamePath == null ? "" : gamePath.getText().toString().trim();
            File directory = gameRoot.isEmpty() ? new File(getFilesDir(), "openxray/logs")
                    : new File(gameRoot, "_appdata_/logs");
            if (!directory.isDirectory() && !directory.mkdirs()) return;
            String stamp = new SimpleDateFormat("yyyyMMdd_HHmmss_SSS", Locale.US).format(new Date());
            sessionFile = new File(directory, "activity_" + stamp + "_launcher.log");
        }
        String timestamp = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US)
                .format(new Date());
        File parent = sessionFile.getParentFile();
        if (parent == null || (!parent.exists() && !parent.mkdirs())) return;
        try (PrintWriter writer = new PrintWriter(new FileWriter(sessionFile, true))) {
            writer.println(timestamp + " " + message);
        } catch (IOException | SecurityException ignored) { }
    }

    private void deleteLog(File file) {
        if (file.isFile())
            file.delete();
    }

    private void setStatus(String message) {
        if (status != null)
            status.setText(message);
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private LinearLayout.LayoutParams matchWrap() {
        return new LinearLayout.LayoutParams(-1, -2);
    }

    private LinearLayout.LayoutParams weightedButton() {
        return new LinearLayout.LayoutParams(0, dp(52), 1);
    }
}
