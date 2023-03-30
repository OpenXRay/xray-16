#include "pch_script.h"
#include "script_sound_info.h"
#include "script_game_object.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CScriptSoundInfo, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CScriptSoundInfo>("SoundInfo")
            .def_readwrite("who", &CScriptSoundInfo::who)
            .def_readwrite("danger", &CScriptSoundInfo::dangerous)
            .def_readwrite("position", &CScriptSoundInfo::position)
            .def_readwrite("power", &CScriptSoundInfo::power)
            .def_readwrite("time", &CScriptSoundInfo::time)
    ];
});
