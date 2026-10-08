package org.openxray.app;

import android.content.Context;
import android.os.Build;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Locale;

final class AppUpdater {
    private static final String RELEASES_API =
            "https://api.github.com/repos/r0shn1ch/xray-16/releases?per_page=20";
    private static final String MANIFEST_ASSET = "android-update.json";
    private static final int MAX_RESPONSE_BYTES = 4 * 1024 * 1024;
    private static final long MAX_APK_BYTES = 512L * 1024L * 1024L;

    static final class Update {
        final String versionName;
        final int versionCode;
        final String releaseNotes;
        final String apkUrl;
        final String apkFileName;
        final String sha256;
        final long apkSize;

        Update(String versionName, int versionCode, String releaseNotes, String apkUrl,
                String apkFileName, String sha256, long apkSize) {
            this.versionName = versionName;
            this.versionCode = versionCode;
            this.releaseNotes = releaseNotes;
            this.apkUrl = apkUrl;
            this.apkFileName = apkFileName;
            this.sha256 = sha256;
            this.apkSize = apkSize;
        }
    }

    private AppUpdater() { }

    static Update checkForUpdate(int installedVersionCode) throws IOException {
        try {
            JSONArray releases = new JSONArray(new String(readUrl(RELEASES_API, MAX_RESPONSE_BYTES),
                    StandardCharsets.UTF_8));
            for (int index = 0; index < releases.length(); ++index) {
                JSONObject release = releases.getJSONObject(index);
                if (release.optBoolean("draft") || release.optBoolean("prerelease"))
                    continue;

                JSONObject manifestAsset = findAsset(release.optJSONArray("assets"), MANIFEST_ASSET);
                if (manifestAsset == null)
                    continue;

                JSONObject manifest = new JSONObject(new String(readUrl(
                        requireHttpsUrl(manifestAsset.getString("browser_download_url")), MAX_RESPONSE_BYTES),
                        StandardCharsets.UTF_8));
                if (manifest.optInt("schema_version") != 1
                        || !"org.openxray.stalker".equals(manifest.getString("application_id")))
                    continue;
                if (manifest.getInt("min_sdk") > Build.VERSION.SDK_INT)
                    throw new IOException("Эта версия обновления требует более новую версию Android.");

                int versionCode = manifest.getInt("version_code");
                if (versionCode <= installedVersionCode)
                    return null;

                String versionName = manifest.getString("version_name");
                String apkName = manifest.getString("apk_file");
                String digest = manifest.getString("apk_sha256").toLowerCase(Locale.ROOT);
                long apkSize = manifest.getLong("apk_size_bytes");
                if (!versionName.matches("\\d+\\.\\d+\\.\\d+")
                        || !isSafeAssetName(apkName)
                        || !digest.matches("[0-9a-f]{64}")
                        || apkSize <= 0 || apkSize > MAX_APK_BYTES) {
                    throw new IOException("Релиз содержит некорректные данные обновления.");
                }

                JSONObject apkAsset = findAsset(release.optJSONArray("assets"), apkName);
                if (apkAsset == null)
                    throw new IOException("В релизе отсутствует APK " + apkName + ".");

                String notes = release.optString("body", "").trim();
                if (notes.length() > 1200)
                    notes = notes.substring(0, 1200) + "…";
                return new Update(versionName, versionCode, notes,
                        requireHttpsUrl(apkAsset.getString("browser_download_url")), apkName, digest, apkSize);
            }
            return null;
        } catch (JSONException error) {
            throw new IOException("Не удалось прочитать сведения о релизе.", error);
        }
    }

    static File download(Context context, Update update) throws IOException {
        MessageDigest digest;
        try {
            digest = MessageDigest.getInstance("SHA-256");
        } catch (NoSuchAlgorithmException error) {
            throw new IOException("SHA-256 недоступен на этом устройстве.", error);
        }
        File partial = File.createTempFile("openxray-update-", ".part", context.getCacheDir());
        File apk = new File(context.getCacheDir(), "openxray-update-" + update.versionName + ".apk");

        try {
            HttpURLConnection connection = openConnection(update.apkUrl);
            try {
                int responseCode = connection.getResponseCode();
                if (responseCode < 200 || responseCode >= 300)
                    throw new IOException("Сервер вернул HTTP " + responseCode + ".");
                if (!"https".equalsIgnoreCase(connection.getURL().getProtocol()))
                    throw new IOException("Загрузка обновления перенаправлена на незащищённый адрес.");

                long received = 0;
                try (InputStream input = connection.getInputStream();
                        FileOutputStream output = new FileOutputStream(partial)) {
                    byte[] buffer = new byte[32 * 1024];
                    int count;
                    while ((count = input.read(buffer)) != -1) {
                        received += count;
                        if (received > MAX_APK_BYTES || received > update.apkSize)
                            throw new IOException("Размер APK не совпадает с манифестом обновления.");
                        digest.update(buffer, 0, count);
                        output.write(buffer, 0, count);
                    }
                }
                if (received != update.apkSize)
                    throw new IOException("Загрузка APK завершилась не полностью.");
            } finally {
                connection.disconnect();
            }

            String actualDigest = toHex(digest.digest());
            if (!actualDigest.equals(update.sha256))
                throw new IOException("Контрольная сумма APK не совпала; установка отменена.");

            if (apk.exists() && !apk.delete())
                throw new IOException("Не удалось заменить временный APK.");
            if (!partial.renameTo(apk))
                throw new IOException("Не удалось подготовить APK к установке.");
            return apk;
        } catch (IOException error) {
            partial.delete();
            throw error;
        }
    }

    private static JSONObject findAsset(JSONArray assets, String name) throws JSONException {
        if (assets == null)
            return null;
        for (int index = 0; index < assets.length(); ++index) {
            JSONObject asset = assets.getJSONObject(index);
            if (name.equals(asset.optString("name")))
                return asset;
        }
        return null;
    }

    private static boolean isSafeAssetName(String name) {
        return name != null && name.endsWith(".apk") && name.equals(new File(name).getName())
                && !name.contains("\\");
    }

    private static byte[] readUrl(String url, int maximumBytes) throws IOException {
        HttpURLConnection connection = openConnection(url);
        try {
            int responseCode = connection.getResponseCode();
            if (responseCode < 200 || responseCode >= 300)
                throw new IOException("GitHub вернул HTTP " + responseCode + ".");
            if (!"https".equalsIgnoreCase(connection.getURL().getProtocol()))
                throw new IOException("Ответ перенаправлен на незащищённый адрес.");
            try (InputStream input = connection.getInputStream();
                    ByteArrayOutputStream output = new ByteArrayOutputStream()) {
                byte[] buffer = new byte[8192];
                int count;
                while ((count = input.read(buffer)) != -1) {
                    if (output.size() + count > maximumBytes)
                        throw new IOException("Ответ GitHub превышает допустимый размер.");
                    output.write(buffer, 0, count);
                }
                return output.toByteArray();
            }
        } finally {
            connection.disconnect();
        }
    }

    private static HttpURLConnection openConnection(String url) throws IOException {
        URL parsed = new URL(requireHttpsUrl(url));
        HttpURLConnection connection = (HttpURLConnection) parsed.openConnection();
        connection.setConnectTimeout(15000);
        connection.setReadTimeout(30000);
        connection.setInstanceFollowRedirects(true);
        connection.setRequestProperty("User-Agent", "OpenXRay-Android/" + BuildConfig.VERSION_NAME);
        if (parsed.getHost().equals("api.github.com"))
            connection.setRequestProperty("Accept", "application/vnd.github+json");
        return connection;
    }

    private static String requireHttpsUrl(String value) throws IOException {
        try {
            URL url = new URL(value);
            if (!"https".equalsIgnoreCase(url.getProtocol()) || url.getUserInfo() != null)
                throw new IOException("Ссылка на обновление должна использовать HTTPS.");
            return value;
        } catch (RuntimeException error) {
            throw new IOException("Некорректная ссылка на обновление.", error);
        }
    }

    private static String toHex(byte[] bytes) {
        StringBuilder text = new StringBuilder(bytes.length * 2);
        for (byte value : bytes)
            text.append(String.format(Locale.ROOT, "%02x", value & 0xff));
        return text.toString();
    }
}
