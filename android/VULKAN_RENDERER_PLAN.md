# Vulkan: план и статус

Vulkan — отдельный игровой рендерер рядом с OpenGL ES. При явном выборе Vulkan ошибка требований сообщается пользователю; Auto может выбрать OpenGL ES. Рендерер читает ресурсы выбранной игры или мода. Неподдерживаемый формат или материал должен сообщать файл, имя и ID. Сборка сама по себе не подтверждает правильность изображения.

## Статус разработки

- Пункты 1–101 отмечены завершёнными по реализации и проверкам на host.
- **102 — частично выполнен.** `shaders.xr` и DDS читаются через VFS в том числе из смонтированных `.db`, без ручной распаковки. Парсер читает класс, версию, смешивание и alpha reference; `GpuLevel` и `GpuModel` передают индивидуальный alpha reference, частицы поддерживают режимы 0–5, UI выбирает pipeline для режимов `S_SET` 0–9. Остаются многотекстурные и многопроходные blender уровня, все варианты свойств и геометрии SoC/CS/CoP и модов, диагностика каждого неподдерживаемого сочетания и дополнительные синтетические фикстуры. Проверка на фактических `.db` выполняется при наличии установленных игр или модов, а не при сборке движка.
- **103 — частично выполнен.** Есть ARM64 ReleaseMasterGold native и подписанный ARM64 debug APK с проверкой состава, ABI и маршрута Vulkan. До закрытия нужны результаты 102, Debug и Release для ARMv7 и ARM64, проверки SPIR-V и полный журнал host-тестов с commit и SHA-256 артефактов.

## Выполненный код (1–101)

Отметки означают реализацию и доступные проверки на host.

### Vulkan и кадр

- [x] 1. Vulkan probe и защита выбора
- [x] 2. `IRender::Create/Destroy` и владение `VulkanGameDevice`
- [x] 3. `OnDeviceCreate/OnDeviceDestroy`, `SetupStates` и порядок создания ресурсов
- [x] 4. `GetDeviceState`, `Reset`, `reset_begin/reset_end`
- [x] 5. `Begin/Clear/ClearTarget/End`
- [x] 6. Камера, `SetCacheXform`, контексты и `OnCameraUpdated`
- [x] 7. `Calculate/Render/RenderMenu` и порядок вызовов UI
- [x] 8. Видимость и отправка статических визуалов
- [x] 9. Основной G-buffer для непрозрачных игровых материалов
- [x] 10. Солнце и окружение в deferred-проходе
- [x] 11. Динамические источники, `IRender_Light`, `IRender_Glow`, `IRender_ObjectSpecific`
- [x] 12. Прозрачные материалы, alpha test и HUD-геометрия
- [x] 13. Подготовка HLSL/SPIR-V вариантов и диагностика `shader_compile`
- [x] 14. Загрузка и кэш материалов/текстур

### Уровни и модели

- [x] 15. Статический уровень и смена таблицы визуалов
- [x] 16. Пул статических OGF
- [x] 17. Дублирование и общие GPU-ресурсы
- [x] 18. Статические иерархии OGF
- [x] 19. Progressive 2 и sliding windows
- [x] 20. Базовый IKinematics
- [x] 21. Данные анимации OGF/OMF
- [x] 22. IKinematicsAnimated, blends и независимые позы
- [x] 23. Skeletal skinned и progressive OGF
- [x] 24. Skeleton rigid и коллизия
- [x] 25. Particle effect
- [x] 26. Particle group
- [x] 27. OGF LOD/impostor
- [x] 28. Деревья OGF
- [x] 29. Fluid visual и таблица OGF

### Интерфейс и эффекты

- [x] 30. `IRenderFactory` для `IUIShader` и `IFontRender`
- [x] 31. `IUIRender`: текстуры, TL/LIT, world transform и состояния
- [x] 32. `IImGuiRender` и его текстуры
- [x] 33. `IUISequenceVideoItem`
- [x] 34. `IStatGraphRender`
- [x] 35. `IWallMarkArray` и методы wallmarks в `IRender`
- [x] 36. `IEnvDescriptorRender` и игровая библиотека частиц окружения
- [x] 37. `IEnvironmentRender`: небо и облака
- [x] 38. `IRainRender`
- [x] 39. `IFlareRender` и `ILensFlareRender`
- [x] 40. `IThunderboltRender` и `IThunderboltDescRender`
- [x] 41. `IDrawUtils` и отладочные примитивы
- [x] 42. `IDebugRender` и `IObjectSpaceRender` для debug-сборки

### Подключение рендерера и шейдеры

- [x] 43. Управление ресурсами `IRender`: deferred upload/unload, память, `OnAssetsChanged`
- [x] 44. `Screenshot`, gamma/brightness/contrast и postprocess
- [x] 45. Статистика, счётчики и остальные диагностические методы `IRender`
- [x] 46. `IRender` в `SetupEnv/ClearEnv`
- [x] 47. Фабрика, UI и ImGui в `SetupEnv/ClearEnv`
- [x] 48. `DU`, debug и `IObjectSpaceRender`
- [x] 49. Выбор режима и `CheckGameRequirements`
- [x] 50. Варианты непрозрачных игровых шейдеров
- [x] 51. Alpha test и прозрачные shader variants
- [x] 52. Skeletal, HUD, tree и progressive variants
- [x] 53. Связь legacy SVS/SPS с Vulkan pipeline
- [x] 54. VFS-кэш игровых shader/material ресурсов

### Игровые пути

- [x] 55. Android/JNI и упаковка Vulkan
- [x] 56. Инициализация игрового запуска
- [x] 57. Загрузочный экран до уровня
- [x] 58. Главное меню
- [x] 59. Действия меню
- [x] 60. Загрузка геометрии SoC
- [x] 61. Загрузка геометрии CS
- [x] 62. Загрузка геометрии CoP
- [x] 63. Полный путь OGF 0–5
- [x] 64. Полный путь OGF 6–12
- [x] 65. Порталы, frustum и LOD
- [x] 66. Детальные объекты и трава
- [x] 67. Запечённый свет и lightmap
- [x] 68. Карта теней солнца
- [x] 69. Применение теней солнца
- [x] 70. Карты теней локальных источников
- [x] 71. Применение локальных теней
- [x] 72. Туман уровня
- [x] 73. Водная поверхность
- [x] 74. Отражения и преломления
- [x] 75. Небо и смена погоды
- [x] 76. Динамическое освещение
- [x] 77. Частицы эффектов и групп
- [x] 78. Дождь, flare и thunderbolt
- [x] 79. Alpha test, прозрачность и декали
- [x] 80. Postprocess, UI и screenshot
- [x] 81. Выгрузка уровня в меню
- [x] 82. Замена уровня в одном процессе
- [x] 83. Reload ассетов при кадрах в полёте
- [x] 84. Игровой NPC motion из OMF
- [x] 85. Независимые экземпляры NPC
- [x] 86. Скиннинг и skeletal LOD
- [x] 87. HUD рук и оружия
- [x] 88. Rigid, bone queries и collision
- [x] 89. Удаление и повторное создание NPC/HUD

### Сборка Android и диагностика

- [x] 90. Интеграция Validation Layers
- [x] 91. Воспроизводимые диагностические сценарии
- [x] 92. Синхронизация ресурсов
- [x] 93. Present и владение swapchain
- [x] 94. Матрица возможностей и форматов
- [x] 95. Pause/resume в коде
- [x] 96. Размер, ориентация и zero extent
- [x] 97. Потеря Android surface
- [x] 98. VK_ERROR_DEVICE_LOST
- [x] 99. Сборка arm64-v8a
- [x] 100. APK с обеими ABI
- [x] 101. Метрики кадра и памяти

## Незавершённая реализация и сборочная проверка

- [ ] **102. Аудит ресурсов и игровых путей.** Поддержать многопроходные blender и варианты игровых шейдеров и свойств SoC, CS, CoP и модов без закрытого списка имён файлов. Сохранить alpha reference и blending каждого материала; проверить варианты раскладок геометрии и интерфейсы. Для неподдерживаемых данных сообщать файл, класс, версию, ID материала или визуала и имя шейдера. Расширить синтетические host-фикстуры на версии форматов, варианты свойств и отрицательные случаи.
- [ ] **103. Проверка сборок.** Завершить 102, собрать Debug и Release для ARMv7 и ARM64, выполнить host-тесты рендерера, проверить SPIR-V, состав APK, ABI, выравнивание и подпись. Записать commit и SHA-256 каждой сборки.

## Диагностика

Лаунчер записывает выбор рендерера, качество, разрешение, Android и ABI. Движок записывает проверку Vulkan и выбор устройства, изменения swapchain, материалы, загрузку уровня, прогресс кадров, lifecycle и завершение. При включённом validation layer сообщения содержат VUID. Экспорт логов описан в [`VULKAN_DIAGNOSTICS.md`](VULKAN_DIAGNOSTICS.md).

`shaders.xr` может находиться внутри `.db`/`.xdb`: VFS открывает его по `$game_data$/shaders.xr` с учётом loose-файлов мода. Для локального аудита параметр запуска `-vk_export_shaders_xr` сохраняет открытый файл как `$app_data_root$/vulkan-shaders.xr` и пишет путь в лог. Для самой игры экспорт не требуется.
