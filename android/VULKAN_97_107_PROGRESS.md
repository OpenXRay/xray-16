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
- #103: Debug/Release собраны для обеих ABI; 33/33 host CTest прошли на проверяемом дереве, 38 SPIR-V прошли `spirv-val --target-env vulkan1.0`. Универсальный APK прошёл проверку маршрута Vulkan, состава обеих ABI, совпадения упакованных ELF после `--strip-unneeded`, zipalign -P 16 и подписи v2/v3. Пункт остаётся открытым до завершения #102; Debug/Release были собраны на промежуточных commit перед финальной правкой завершения Vulkan module.

| Артефакт | Commit исходников | SHA-256 |
| --- | --- | --- |
| ARM64 Debug `libmain.so` | `236754216469b0e7ebd8cb4e7734fe7630f2afa2` | `1d8b89f0c5a1cbdb382914f47864b998052a922f5d493da67e4e0521eaa8dc04` |
| ARM64 Release `libmain.so` | `236754216469b0e7ebd8cb4e7734fe7630f2afa2` | `b829ee24a2cab768e4e8e590236a611a171107b930f5c31c987d9536873d6f65` |
| ARMv7 Debug `libmain.so` | `ca3b175e57b9858c9b9680f5a67a0b19d98a31d2` | `ff30bbb8720289f2603b4140b367617d7bb52c624ff75ec544f3fc3783f8743c` |
| ARMv7 Release `libmain.so` | `ca3b175e57b9858c9b9680f5a67a0b19d98a31d2` | `9483c1ba0183df0960399dd728407842bb74d75115516ccb433312b78aa3f751` |
| ARM64 ReleaseMasterGold `libmain.so` | `aa71fe5a36de4b38daafa5b16a3cb8afcbce88f7` | `1f5536142d3572b67abba207a0719297ba74ad42a45f13a3d434390dbd216ae9` |
| ARMv7 ReleaseMasterGold `libmain.so` | `aa71fe5a36de4b38daafa5b16a3cb8afcbce88f7` | `425204612174e838e5c06b92166b6ec2eec94415631ceb1f45f1ff289f5fe5d1` |
| Универсальный debug-signed APK `0.9.128` | `aa71fe5a36de4b38daafa5b16a3cb8afcbce88f7` | `88301bfa6d88aeaa648ef85b1fbcc4f7c5e8bc4551074431883c2ba6b0215725` |
- #104–107: требуются подключённое Android Vulkan устройство и правомерные игровые данные, чтобы записать JNI/выбор рендера, загрузочный экран, меню, лог и кадры. В среде `adb devices -l` не показывает устройств. APK и host-тесты не заменяют эти испытания.

Точный commit для этого среза фиксируется после публикации PR. Результаты на устройстве фиксируются отдельно по сценарию с моделью, Android/GPU/драйвером, логом и кадрами.
