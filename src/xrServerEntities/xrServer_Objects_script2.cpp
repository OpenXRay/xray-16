#include "pch_script.h"
#include "xrServer_Objects.h"
#include "xrServer_script_macroses.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CSE_PHSkeleton, (), { module(luaState)[class_<CSE_PHSkeleton>("cse_ph_skeleton")]; });

SCRIPT_EXPORT(CSE_AbstractVisual, (CSE_Visual, CSE_Abstract), {
    module(luaState)[luabind_class_abstract2(CSE_AbstractVisual, "CSE_AbstractVisual", CSE_Visual,
        CSE_Abstract).def("getStartupAnimation", &CSE_AbstractVisual::getStartupAnimation)];
});

/**
SCRIPT_EXPORT(CSE_SpawnGroup, (CSE_Abstract),
{
    module(luaState)
    [
        luabind_class_abstract1(
            CSE_SpawnGroup,
            "cse_event",
            CSE_Abstract
        )
    ];
});
**/
