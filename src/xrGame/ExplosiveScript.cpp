#include "Explosive.h"
#include "pch_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(
    CExplosive, (), { module(luaState)[class_<CExplosive>("explosive").def("explode", &CExplosive::Explode)]; });
