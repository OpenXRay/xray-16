#include "chimera.h"
#include "pch_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(
    CChimera, (CGameObject), { module(luaState)[class_<CChimera, CGameObject>("CChimera").def(constructor<>())]; });
