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

������������ � ������ ����������� �����:

��� ������ level.ai �� build.aimap:
������ ������ �������:
    "-f labx8 -keep_temp_files -out level.ai -pure_alloc"
����:
    gamedata/shaders_xrlc.xr           - �������� � ������������� � ShaderEditor.
    gamedata/levels/<NAME>/build.cform - �������� � ������� xrLC.
    gamedata/levels/<NAME>/build.prj   - �������� � ������� LevelEditor.
    gamedata/levels/<NAME>/build.aimap - �������� � ������� LevelEditor.
    gamedata/textures/<FILE>.[thm,dds] - ��������� ���������� (��������� � ������� LevelEditor?).
�����:
    gamedata/levels/<NAME>/level.ai
����������:
    � ���������� ���� ������������ �������� �������, ��� �������� � ��� ������ game spawn data (����������� � ������������ CLevelGraph).

��� �������� level.ai:
������ ������ �������:
    "-verify zaton -pure_alloc"
����:
    gamedata/levels/<NAME>/level.ai    - �������� � ������� xrAI.
�����:
    �� ������ �����-���� ������.

��� ������ game spawn data � ������ ������������� ������ ������� ����:
������ ������ �������:
    "-s zaton,jupiter,jupiter_underground,labx8,pripyat -start zaton -out all -pure_alloc"
����:
    gamedata/configs/system.ltx        - � ����� ��������� ������ ������ ������������ (*.ltx) ����������� ����������� xrSE_Factory.
    gamedata/configs/gameplay/*.xml    - ��������� *.xml ������ ����������� ����������� xrSE_Factory. ������ ���� ����������.
    gamedata/configs/game.ltx          - � ����� ����������� ������ ����� game_*.ltx �� ���� �� �����. ������ ���� ����������.
    gamedata/scripts/*.script          - ��������� �������� ����������� ����������� xrSE_Factory.
    gamedata/levels/<NAME>/level.ai    - �������� � ������� xrAI.
    gamedata/levels/<NAME>/level.spawn - �������� � ������� LevelEditor.
    gamedata/levels/<NAME>/level.game  - �������� � ������� LevelEditor.
��������� �����:
    _appdata_/temp/*                   - ��������� ��������� ������.
�����:
    gamedata/spawns/all.spawn
����������:
    ��������: ��������� ����� *.xml ����������� ��������� ���.
    ��������: �������������� ��� ������ all.spawn (� ����� ����) � game.spawn (�������� ���� ������).
    ��������: game.ltx � ������ ����� game_*.ltx ����������� �������� ���������� (������) ��� � �������� ������!
        �������� ��������� � "static SFillPropData fp_data;" ����������� � ���� ������ � ������������ ������ xrSE_Factory.
        ���� �� ����� ������������ ������������, ��� ������������ ��� ����� inc --> load, dec --> unload ����� ��������.
        �� � ���������� ��� ����������� (���� �� ����) inc ����� ���������� dec.
