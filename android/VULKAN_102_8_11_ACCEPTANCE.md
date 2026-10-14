# PDF 102, пункты 8–11: контракт реализации и доказательства

Исходные подпункты 8.1–8.4, 9.1–9.4, 10.1–10.4 и 11.1–11.4 взяты из
`openxray-vulkan-plan-102-103.pdf`. Таблицы ниже описывают **обещанную**
семантику текущего рендерера. Неизвестный формат мода не считается
поддержанным по совпадению версии или имени. Для оригинальных архивов и
изображения на устройстве действует общий шлюз после реализации 102.

## 8. Форматы и сочетания

| Вход | Заявленная область и нормализация | Отказ или fallback | Host-свидетельство |
| --- | --- | --- | --- |
| SoC, CS, CoP `level` | `hdrLEVEL` v14, quality 0–2; профиль берётся из режима игры; shader table сохраняет sparse ID и произвольное имя | Усечённый header отличается от неизвестной версии/quality; отсутствующий material ID сообщает visual/type/ID | `vulkan_level_models`: три профиля × float/packed, качества 0–2, sparse ID 7, версии 13/15 и quality 3 |
| `level.geom` | D3D declaration: float3 POSITION/NORMAL, packed D3DCOLOR NORMAL, float2/short2/short2n UV0/UV1, D3DCOLOR COLOR/TANGENT/BINORMAL; uint16 triangle indices; UV1, baked colour и material ID сохраняются | Дубли POSITION/NORMAL/UV/COLOR/TANGENT/BINORMAL, неизвестный type/stream, повреждённый buffer, index за пределами модели получают отказ; нет догадки по имени профиля | `vulkan_level_models`: UV, normals, packed, lightmap, повторный UV, усечённый buffer, некорректный индекс |
| level OGF | v4: static 0, hierarchy 1, progressive 2, LOD 6, tree 7/11; linked/embedded children, windows и tree transform; типы 8/9 принадлежат particle runtime | Skeletal 3/4/5/10 не выдаются за level static; OGF 12 относится к отдельному DX11 fog volume; неверные ссылки и SWI не загружают частичную модель | `vulkan_level_models`, `vulkan_visual_catalog`, `vulkan_slide_windows` |
| standalone OGF | v4: static/progressive, tree 7/11, hierarchy/skeleton 3/10 с 1–4 skin weights, отдельные LOD facets и model instance; внутренние данные переходят в `ModelGeometry` с UV, normal, bones/weights | Неизвестные type и vertex encoding с числовыми type/format, повреждённые weights/window/index и ссылки получают отказ с именем OGF и shader ID на границе загрузки | `vulkan_model_geometry`, `vulkan_skeleton_bones`, `vulkan_kinematics`, `vulkan_skeleton_motions` |
| `level.geomX` | Если оба буфера совместимы, type-2 `OGF_FASTPATH` с GCONTAINER и SWIDATA создаёт отдельную fast-модель; обычная геометрия остаётся в модели | При отсутствии или несовместимости необязательного VB/IB используются `level.geom` и точная причина в логе. Повреждённые собственные SWI/OGF-чанки **не** скрываются fallback; испорченный основной `level.geom` всегда ошибка | `vulkan_level_models`: fast draw/window, отсутствующий VB/IB, несовместимый VB, усечённый IB, испорченный SWI |
| Материал × геометрия | Разрешение материала следует shader ID, имени и свойствам `shaders.xr` после декодирования; строки 1–7 и тесты 3–5 задают принятые классы и режимы | Неразрешённый/неподдерживаемый blender не превращается в opaque; ошибка содержит material ID, shader, texture/slot и stage | `vulkan_shader_material_library`, `vulkan_asset_flow`, `vulkan_surface_channels` |

Это матрица **синтетически проверенных декодеров**, а не утверждение, что
каждая версия всех трёх установленных игр испытана. Игра требует настоящий
архив для проверки встретившихся layout/OGF/material сочетаний и кадра.

## 9. Доступные интерфейсы

| Интерфейс | Игровой контракт и проверка |
| --- | --- |
| `particles_systems_library` | `CPGDef` владеет именами/эффектами; begin/end используют полуоткрытый диапазон, next принимает только адрес текущего элемента без сравнения несвязанных указателей. Пустой каталог, два ID, чужой/end итератор, смена каталога и clear проверены в `vulkan_renderer_module`. Определение после reload удерживать нельзя. |
| `ImGui OnDeviceResetBegin/End` | Begin освобождает атласные descriptors и запрашивает новые пиксели; End восстанавливает текущий context/backend flags, следующий `Render` загружает атлас. Последовательность сброса без устройства проверена в `vulkan_renderer_module`; atlas upload проверяется UI fixtures. |
| `updateGamma` | Применяет установленные gamma/brightness/contrast к postprocess constants немедленно; `End` передаёт их в кадр. Console и конец precache вызывают именно этот метод. |
| `BeforeWorldRender` / `AfterWorldRender` | Вызовы `xrGame/Level.cpp` обрамляют мир; Vulkan кадр отправляется в `End`. У R2 эти hooks также пусты. |
| `overdrawBegin/End` | `xrEngine/Device_overdraw.cpp` безусловно ASSERT до обращения к интерфейсу: штатный игровой путь недостижим. Editor-only операции не объявлены игровым контрактом. |
| Lens flare, thunderbolt, particle lifecycle | Контракт `Copy`, reset, владения buffers и текущего ресурса описан в `VULKAN_102_INTERFACES.md`; эффекты ждут визуальной приёмки на устройстве. |

## 10. Диагностика и поколение ресурса

- Декодер разделяет truncation/invalid payload и unsupported version/type/stream;
  ошибки уровня несут профиль, `level.geom`/`level.geomX`, visual/type/material ID
  и shader/texture, когда header корректен. Независимый OGF указывает имя,
  type, shader ID и числовой vertex encoding.
- Загрузка `GpuLevel` добавляет путь, mesh ID, regular/fast, material ID,
  shader/texture и стадию GPU preparation к внутренней ошибке. Ошибка
  texture role или прохода сохраняет исходный slot/property/pass.
- Fallback `level.geomX → level.geom` допускается только для необязательных
  буферов и пишет исходный путь, выбранный путь, профиль и причину.
  Отсутствующая таблица portal/sector отдельно сообщает переход к полным roots.
- `ResourceIssueCounter` сообщает полный контекст при первом отказе и число
  повторов при unload/reload; level draw и model/particle resource paths
  используют один контракт. `vulkan_renderer_module` проверяет разделение
  ресурсов, три повтора и сброс поколения.

## 11. Регрессионная матрица и предел доказательства

| Контракт | Parser/unit | Host Vulkan или mock | Устройство |
| --- | --- | --- | --- |
| 1–5: классы, каналы, граф и режимы | `vulkan_shader_material_library`, `vulkan_level_pass_graph`, `vulkan_detail_assets`; произвольные имена, VFS override и повреждённые чанки | `vulkan_asset_flow`, `vulkan_surface_channels`, `vulkan_particle_visual`, `vulkan_deferred_pass`; descriptor, push, blending/depth, readback и порядок | Кадр и цвет не приняты |
| 6–7: прозрачные детали и выбор по свойствам | `vulkan_level_models`, `vulkan_detail_assets`, `vulkan_scene_pass` | `vulkan_asset_flow`, `vulkan_particle_visual` | Перекрытия растительности не приняты |
| 8: версии, layout, OGF, fast fallback | `vulkan_level_models`, `vulkan_visual_catalog`, `vulkan_model_geometry`, `vulkan_slide_windows`, skeleton fixtures | `vulkan_scene_pass` и модельные pipeline fixtures | Реальные SoC/CS/CoP и изображения не приняты |
| 9–10: interface/lifetime/ошибки | `vulkan_renderer_module`, декодерные отрицательные случаи и сообщения с ID/причиной | `vulkan_asset_flow`, `vulkan_surface_lifecycle` | Эффекты/reload не приняты |

Host fixture не доказывает цвет, lighting, FPS или отсутствие вылета на Adreno.
Если реальный вход вне этой матрицы окажется обязательным, отдельная находка
получит N-номер и измеримый критерий, а исходные подпункты не расширяются.

Проверка этого среза: 38/38 `ctest` без оконного WSI, числовой Vulkan readback
на Lavapipe с validation layer без ошибок/предупреждений, ARM64 native/Gradle
APK и Vulkan package checks; для APK использовался `glslc` из NDK r30.
SHA-256 APK:
`6a941393976d224d3fc3ee71b3976b8e173abae018614526ba6ef7c873a26b6e`.
Отдельная полная host-сборка игры с `XRAY_USE_LUAJIT=0` блокирована прежним
`LUA_GCTIMEOUT` в `xrGame/Level.cpp`; тестовые цели и Android собираются.
