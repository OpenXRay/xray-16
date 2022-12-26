#include "pch_script.h"
#include "pseudodog.h"
#include "psy_dog.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CAI_PseudoDog, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CAI_PseudoDog, CGameObject>("CAI_PseudoDog")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CPsyDog, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CPsyDog, CGameObject>("CPsyDog")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CPsyDogPhantom, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CPsyDogPhantom, CGameObject>("CPsyDogPhantom")
            .def(constructor<>())
    ];
});
