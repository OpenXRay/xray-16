The following keys are supported / required:
-? or -h       == this help
-f <NAME>      == compile 'level.ai' from 'build.aimap'
    -out <FILE>         == name of output file (default: 'level.ai')
    -draft              == do not load and do not process some stages
    -pure_covers        == <? need investigation ?>
    -keep_temp_files    == do not delete 'build.aimap' after load
-verify <NAME> == verify compiled 'level.ai'
    -noverbose          == do not print all single linked vertices (print only count)
-s <NAME,...>  == build game spawn data
    -out <FILE>         == name of output file (default: 'NAME.spawn')
    -start <NAME>       == name of game start level
    -no_separator_check == do not verify that restrictors separates AI map into several disconnected components

<NAME> == level name as 'gamedata/levels/<NAME>/'
<FILE> == any file name

Используемые в работе компилятора файлы:

При сборке level.ai из build.aimap:
Пример ключей запуска:
    "-f labx8 -keep_temp_files -out level.ai -pure_alloc"
Вход:
    gamedata/shaders_xrlc.xr           - Создаётся и редактируется в ShaderEditor.
    gamedata/levels/<NAME>/build.cform - Создаётся с помошью xrLC.
    gamedata/levels/<NAME>/build.prj   - Создаётся с помошью LevelEditor.
    gamedata/levels/<NAME>/build.aimap - Создаётся с помошью LevelEditor.
    gamedata/textures/<FILE>.[thm,dds] - Множество материалов (создаются с помошью LevelEditor?).
Выход:
    gamedata/levels/<NAME>/level.ai
Примечания:
    В дальнейшем файл используется напрямую движком, при проверке и при сборке game spawn data (загружается в конструкторе CLevelGraph).

При проверке level.ai:
Пример ключей запуска:
    "-verify zaton -pure_alloc"
Вход:
    gamedata/levels/<NAME>/level.ai    - Создаётся с помошью xrAI.
Выход:
    Не создаёт каких-либо файлов.

При сборке game spawn data в случае использования набора игровых карт:
Пример ключей запуска:
    "-s zaton,jupiter,jupiter_underground,labx8,pripyat -start zaton -out all -pure_alloc"
Вход:
    gamedata/configs/system.ltx        - А также множество других файлов конфигурации (*.ltx) загружается посредством xrSE_Factory.
    gamedata/configs/gameplay/*.xml    - Множество *.xml файлов загружается посредством xrSE_Factory. Смотри ниже примечания.
    gamedata/configs/game.ltx          - А также загружаются другие файлы game_*.ltx из этой же папки. Смотри ниже примечания.
    gamedata/scripts/*.script          - Множество скриптов загружается посредством xrSE_Factory.
    gamedata/levels/<NAME>/level.ai    - Создаётся с помошью xrAI.
    gamedata/levels/<NAME>/level.spawn - Создаётся с помошью LevelEditor.
    gamedata/levels/<NAME>/level.game  - Создаётся с помошью LevelEditor.
Временные файлы:
    _appdata_/temp/*                   - Несколько временных файлов.
Выход:
    gamedata/spawns/all.spawn
Примечания:
    ВНИМАНИЕ: Небольшая часть *.xml загружаются множество раз.
    ВНИМАНИЕ: Несоответствие имён файлов all.spawn (в паках игры) и game.spawn (исходные коды движка).
    ВНИМАНИЕ: game.ltx и другие файлы game_*.ltx загружаются огромное количество (тысячи) раз в процессе сборки!
        Проблема находится в "static SFillPropData fp_data;" объявленном в двух местах и используемом только xrSE_Factory.
        Судя по всему разработчики предполагали, что используемая ими схема inc --> load, dec --> unload будет работать.
        Но в реальности для большинства (если не всех) inc сразу происходит dec.
