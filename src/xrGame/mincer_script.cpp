#include "pch_script.h"
#include "Mincer.h"
#include "RadioactiveZone.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CMincer, (CGameObject), {
    module(luaState)[class_<CMincer, CGameObject>("CMincer").def(constructor<>()),
        class_<CRadioactiveZone, CGameObject>("CRadioactiveZone").def(constructor<>())];
});
