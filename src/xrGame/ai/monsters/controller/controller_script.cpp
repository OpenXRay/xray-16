#include "controller.h"
#include "pch_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CController, (CGameObject),
    { module(luaState)[class_<CController, CGameObject>("CController").def(constructor<>())]; });
