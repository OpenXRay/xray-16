#include "cat.h"
#include "pch_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CCat, (CGameObject), { module(luaState)[class_<CCat, CGameObject>("CCat").def(constructor<>())]; });
