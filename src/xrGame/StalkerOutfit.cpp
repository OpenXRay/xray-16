#include "pch_script.h"
#include "StalkerOutfit.h"
#include "ActorHelmet.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CStalkerOutfit::CStalkerOutfit() {}
CStalkerOutfit::~CStalkerOutfit() {}

SCRIPT_EXPORT(CStalkerOutfit, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CStalkerOutfit, CGameObject>("CStalkerOutfit")
            .def(constructor<>())
    ];
});
