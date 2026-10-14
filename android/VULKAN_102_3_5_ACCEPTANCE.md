# PDF 102, пункты 3–5: проверяемый контракт host

Дата среза: 14.10.2026. Этот документ фиксирует только принятый Vulkan
контракт и результаты воспроизводимых host-проверок. Изображение и frame-time
на Android проверяются после окончания реализации 102. Новые механизмы получают
отдельный ID; исходные подпункты 3.1–5.4 не расширяются.

## 3. Текстурные роли и время жизни

| Слот | Роль и проверенный выход fragment SPIR-V |
| --- | --- |
| 0, 1 | Base RGB и alpha; lightmap G → sun occlusion в albedo.a, A → hemi в normal.a. |
| 2 | Single detail RGB/alpha либо terrain mask RGBA, 2D environment или cube environment в соответствующем варианте. В тесте выборка меняет G-buffer, а не только отражается в дескрипторе. |
| 3–6 | Четыре terrain diffuse при разных масках; веса нормализуются. Для environment вариант slot 3 имеет отдельную роль normal detail. |
| 7–10 | Base bump/aux и detail bump/aux: gloss из bump.r², height из aux.b; detail gloss из собственного red. |
| 11–14, 16–19 | Четыре terrain bump и их parallax companion: независимо влияют на gloss/height. |
| 15 | External normal меняет normal attachment при неизменных других слотах. |
| Параметры | Material category и emission записываются в surface attachment; LaEmB 2D и cube reflection читают собственные варианты шейдера. |

`vulkan_surface_channels` создаёт 20 различных RGBA32F изображений, кубическую
текстуру и три attachment, исполняет поставляемые `*.ps.spv` через headless
Vulkan и сверяет числовые readback значения. `vulkan_asset_flow` проверяет
разделение cache key по каждому имени, sampler и surface-параметрам, повторное
получение descriptor, reload всех 20 одновременно, выпуск после последней
ссылки и ожидание idle GPU до освобождения. Отсутствующие обязательные роли
возвращают ошибку имени слота в загрузчике, не подменяются запасным именем.
Выбор устройства требует 20 fragment sampled images/samplers и три color
attachments, поскольку material descriptor объявляет bindings 0–19; меньший
лимит получает явную ошибку capability до создания pipeline.

## 4. Граф и исполнение

| Семейство/элемент исходного Compile | Принятый проход и стадия | Состояние/зависимость |
| --- | --- | --- |
| LM, V, LM_AREF, V_AREF, BmmD и terrain HQ/LQ | 1 × G-buffer; HQ выбирает detail, bump и terrain variant, LQ использует base при доступном исходном контракте | Depth test/write, base + объявленные lightmap/detail/mask/bump роли; shadow отдельно рисует depth-only, alpha-ref при cutout. |
| MODEL, D_TREE, MODELEbB | 1 × G-buffer для opaque/cutout либо 1 × forward для transparent | Статический/1–4 skin weights; HUD имеет отдельную проекцию, depth hierarchy и variant. Оpaque MODELEbB не читает cube, blended читает. |
| LmEbB, LaEmB | LmEbB: 1 × G-buffer/forward по alpha. LaEmB: G-buffer pass 0 → pass 1 | LaEmB pass 0 читает lightmap + environment 2D и constant; pass 1 читает base и умножает **тот же color attachment** DST_COLOR × SRC_COLOR. LEQUAL, depth read/no write, normal/surface write mask 0. Внутри одного subpass результат проходит через fixed-function attachment blend; sampled image edge здесь отсутствует. |
| PARTICLE и S_SET | 1 × G-buffer для world PARTICLE SET (N4), остальные world forward; HUD отдельно | SET alpha-ref 200/255, z-write, tint; forward использует точные blend factors. S_SET world SET — отдельный depth-writing forward route, S_SET particle не смешивается с PARTICLE. |
| UI S_SET | 1 × UI overlay | Нет world depth attachment, адресация clamp/wrap на descriptor. |

`validate_level_pass_graph` отклоняет незаданный shader/slot, зависимость от
несуществующего прохода и cross-target previous-color edge с номером прохода.
`vulkan_deferred_pass` проверяет реальную последовательность двух bind/draw,
привязку одного material descriptor, маски attachment и depth/blend state.
SPIR-V reflection проверяет `Camera` offsets 0/64/80 во всех статических и
skinned vertex variants; headless прогон с `VK_LAYER_KHRONOS_validation`
прошёл без сообщений ERROR/WARNING при лимите push constants 128 байт.
`GBufferTargets` переводит attachments в shader-readable состояние перед
lighting и копирует depth для следующей стадии; `DeferredFrame` очищает depth
перед HUD. Режим editor не запускается Android gameplay renderer; отдельный
editor Compile не объявлен поддерживаемым Vulkan материалом. Нестандартный
priority и именованная анимированная матрица сохраняют точный отказ N2/N1.

## 5. Матрица принятых режимов

В таблице `C` означает оба значения `Texture clamp`; указанные Z/Lighting/Fog
— единственные принятые значения. Alpha ref сериализован (кроме PARTICLE SET
и S_SET 2R SET). Любая не перечисленная комбинация сообщает конкретное поле.

| Контекст | Blending | Z-test / Z-write / Lighting / Fog | Число сочетаний с C |
| --- | --- | --- | ---: |
| UI overlay | S_SET 0–9 | depth отсутствует / 0 / 0 / 0 | 20 |
| Particle sprite world | S_SET 0–5 | 1 / 1 только SET, иначе 0 / 0 / 0 | 12 |
| Particle sprite HUD | S_SET 0–5 | 1 / 1 только SET, иначе 0 / 0 / 0 | 12 |
| Level/decal/tracer | S_SET 0; 1; 2–9 | 1 / 1 / 0 / 0; 1 / 0 / 0 / 0 либо 1; 1 / 0 / 0 / 0 | 22 |
| OGF model world | S_SET 0–9 | 1 / 1 только SET, иначе 0 / 0 / 0 | 20 |
| OGF model HUD | S_SET 0–9 | 1 / 1 только SET, иначе 0 / 0 / 0 | 20 |
| PARTICLE world + HUD | PARTICLE 0–5 | World SET до lighting в G-buffer, HUD SET в своём depth, остальные forward; параметры Lighting/Fog не сериализованы | 12 × 2 |

Всего для S_SET: 106 принятых маршрутов в шести контекстах, 320 разных
сериализованных записей × 4 consumer проверки с отклонением остальных.
Для PARTICLE проверены 12 записей × world/HUD. У S_SET modes 0–9 проверены
replace, source-alpha, ONE/ONE, DST_COLOR/ZERO, DST_COLOR/SRC_COLOR,
SRC_ALPHA/ONE, wallmark MUL_2X, 2R/4R и точные depth/write; sampler и alpha-ref
сверяются отдельно. То же определение разрешается для UI, particle, level и
OGF, причём ошибки `Z-test`, `Z-write`, `Lighting`, `Fog` не исчезают при смене
контекста. Вне данной таблицы есть отдельный принимаемый water route с
serialized strict sorting + Lighting, который проверяется профильным тестом.

## N4 — отделённый механизм, выполнен на host

Обнаружено после фиксации 3–5: исходный R2 `CBlender_Particle::Compile`
направляет мировой `PARTICLE SET` в `deffer_particle`, а старый Vulkan route
записывал его после lighting. Критерий N4: один draw на частицу в G-buffer до
lighting с alpha-ref 200/255 и tint, ни одного повторного draw в forward;
смешанная группа отправляет остальные элементы в forward/HUD. Реализация
разделяет фазу группы по каждому эффекту; graph, pipeline mock и числовой
headless SPIR-V readback проверяют route, alpha threshold и tint. Проверка
полного кадра на Android остаётся общим устройственным шлюзом.
