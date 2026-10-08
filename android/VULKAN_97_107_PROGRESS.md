# Vulkan #97–107: код и допуск

Дата обновления: 2026-10-08. Статус по плану следует брать из `VULKAN_RENDERER_PLAN.md`.

## Проверенный код

- #97: повторное создание SDL Vulkan surface после `VK_ERROR_SURFACE_LOST_KHR`, освобождение GBuffer descriptors до уничтожения прежних image views при смене swapchain. Host-тест под Xvfb и Vulkan validation трижды заменяет поверхность и проверяет отсутствие ошибок validation в этих циклах.
- #98: `VK_ERROR_DEVICE_LOST` при acquire переводит frame context в состояние потери устройства; renderer записывает причину и посылает событие завершения. Освобождение upload-ресурсов и teardown принимают результат device lost, fault-injection тест уничтожает и повторно создаёт frame context.
- #99: отдельная цель `build-arm64.sh`, общий native build для `armeabi-v7a` и `arm64-v8a`, собраны ARM64 SDL2/OpenAL/libjpeg/Ogg/Vorbis/LZO/Theora и нативный `libmain.so`.
- #100: двухархитектурный debug-signed APK включает `libmain.so`, OpenAL, libc++ и validation layer для каждого ABI, SPIR-V и ресурсы. Проверены ELF, содержимое архива, Android manifest, zipalign и подпись. SHA-256 `2df296dfb294bcb23bb1df76f703470fbfd02b9282fc05f918f8807e28eb2733` (`openxray-universal-launcher-v0.9.48-debug.apk`).
- #101: timestamp queries Vulkan при поддержке очередью, CPU/frame time, FPS и P95, размеры resident textures и level/model geometry, счётчики draw; JSON сохраняется при завершении рендерера. Host-тесты проверяют P95, отсутствие GPU timestamps и fault path.

Проверки: 32/32 CTest; `vulkan_validation_host_test` под Xvfb с `VK_LAYER_KHRONOS_validation` прошёл. Сообщение VUID о неверном semaphore создаётся тестом *после* проверки замен surface, чтобы убедиться, что validation действительно включена.

## Открытые блокеры

- #102: `LevelModels.cpp::classify_surface_material` использует эвристику по подстрокам и молча относит неизвестный материал к opaque. `opaque-variants.json` содержит ограниченные семейства; нет сверки всех достигнутых shaders/materials SoC/CS/CoP с игровыми данными. В `VulkanEnvironmentRender::Library` итераторы групп возвращают `nullptr`; в `VulkanObjectSpaceRender` `SetShader` пустой. По поиску исходников игровой путь этих методов не вызывает, а редактор пользуется `particles_group_ids`, но полный аудит интерфейсов остаётся открытым (#102.3). Проверку и устранение пробелов описывают #102.1–102.3. Видимость `sPoly` уже исправлена на frustum test; неиспользуемый `VK_PENDING_FACTORY` удалён.
- #102 (дополнение): `shaders.xr` теперь читается итератором чанков с проверкой длины, включая разреженные ID модов. Для неподдерживаемого blender выводятся файл, ID, класс, версия и имя шейдера; `PARTICLE/SET` классифицируется как alpha test, а неподдерживаемые ADD/MUL и прочие режимы `PARTICLE`/`S_SET` отклоняются вместо незаметной замены на обычное alpha blending. Синтетическая фикстура покрывает разреженный ID, alpha ref, смешивание, неизвестный тип свойства, дубликат и повреждённую длину. Это не завершает поддержку дополнительных проходов/вариантов; игровые архивы SoC/CS/CoP и модов в этом дереве отсутствуют.
- #103: допуск закрывается только после #102, Debug/Release для обеих ABI и полной проверки APK. Текущие host-тесты и ReleaseMasterGold Android недостаточны для отметки #103.
- #104–107: требуются подключённое Android Vulkan устройство и правомерные игровые данные, чтобы записать JNI/выбор рендера, загрузочный экран, меню, лог и кадры. В среде `adb devices -l` не показывает устройств. APK и host-тесты не заменяют эти испытания.

Точный commit для этого среза фиксируется после публикации PR. Результаты на устройстве фиксируются отдельно по сценарию с моделью, Android/GPU/драйвером, логом и кадрами.
