# Проверка пунктов Vulkan 53–63

Дата: 2026-10-03. Ветка: `vulkan/renderer-foundation`, PR #6.

Границы результата: кодовые критерии текущего плана выполнены без устройства. Это не аппаратная приёмка рендерера; #103–145 остаются открытыми.

| Пункты | Реализация и проверка |
| --- | --- |
| 53–54 | Device-owned `GameShaderResources` хранит Vulkan counterparts SVS/SPS. `shader_compile`, startup/reload и `GpuLevel`/`GpuModel` используют общий запрос pipeline. Тест проверяет legacy names, нормализацию, cache reuse, отсутствующий вариант, stage/entry/flags/pass state и очистку. Смена уровня сохраняет device shader cache, texture cache ключуется разрешённым VFS path. |
| 55–56 | Точные токены выбора renderer, explicit failure без GLES fallback, Android splash guard. `check_android_vulkan_route.py` проверяет цепочку XRayActivity → SDL Java → nativeRunMain → SDL_main, регистрацию Vulkan и EGL guard в SDL. CTest проверяет explicit/auto/negative варианты, SetupEnv и реальные window flags; тесты device resource/frame state проверяют допустимый порядок. JNI на телефоне проверяется в #104. |
| 57–59 | Fonts/loading/menu идут через существующие Vulkan factory/UI/video реализации. Новые queued descriptor leases закрывают время жизни при обновлении video texture и удалении UI producer. `vulkan_ui_flow` записывает loading UI до уровня, два video frame, menu button/font, scissor/alpha и cancel/reset/reopen; очистка освобождает оба flight slot. Настройки/загрузка/выход используют общий engine menu и renderer reset/teardown. |
| 60–62 | Все три R2 профиля используют production `hdrLEVEL` v14; бинарный формат не определяется по названию игры. Проверены float/packed D3D9 declarations, материал, VFS level.geom, сжатые вложенные OGF и GPU upload/draw для каждого профиля. Ошибки header, shader/vertex/index/visual данных и повторной загрузки атомарны. Отсутствующие UV, нечисловые координаты/normal/UV/bounds и неподдерживаемые declarations отклоняются. |
| 63 | OGF 0/1/2 и skeleton/child 3/4/5 проходят VFS decode → GpuModel creation → upload → record → duplicate/delete. Матрица включает 1–4 weights, progressive windows, настоящий IKinematics и отдельные позы копий. `model_CreateChild` разрешает skinned leaf в отдельном child cache namespace. Ошибки содержат файл, type/shader id или chunk/child id. Сжатые children/fast chunks нормализуются engine codec с ограничением размера и проверкой результата. |

## Выполненные проверки

- Debug и Release: `cmake --build <build-dir> --target xrRenderVK` — успешно.
- Debug CTest: `ctest --test-dir build-vulkan -R 'vulkan_|android_renderer_choice' --output-on-failure` — **23/23**.
- Python: `python -m unittest discover -s tests -p '*vulkan*.py'` — **7/7**.
- ARMv7: `android/build-apk-armv7.sh` с build kit v0.8.0 — успешно, включая gameplay libmain.so, shader asset audit и Android route/package audit.
- Windows CI дополнен новыми asset/UI/module/selection тестами. Его результат проверяется GitHub Actions; локально MSVC не запускался.
- `git diff --check` — успешно.

APK: `openxray-armv7-launcher-v0.9.48-debug.apk`, versionCode 62, armeabi-v7a.

SHA-256: `a505f86ef64c969eaacd789ae2f702c4c0a8d046f4663aba50e93b5c28523197`.

[Скачать APK](https://chatgpt.com/library/libfile_6a32b74aba788191ab37977fe1a86402).

Mock dispatch проверяет вызовы, данные и время жизни ресурсов; он не подтверждает изображение или GPU synchronization на реальном драйвере. Fixtures проверяют заявленные форматы, а реальные SoC/CS/CoP архивы проверяются в #109–112 и #142–144. ARM64 собирается отдельными этапами #99–100.
