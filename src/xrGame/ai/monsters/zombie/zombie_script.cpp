#include "pch_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "zombie.h"

using namespace luabind;

SCRIPT_EXPORT(
    CZombie, (CGameObject), { module(luaState)[class_<CZombie, CGameObject>("CZombie").def(constructor<>())]; });
