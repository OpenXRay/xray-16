#include "pch_script.h"

#include "StalkerOutfit.h"

void CStalkerOutfit::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CStalkerOutfit, CGameObject>("CStalkerOutfit")
            .def(constructor<>())
    ];
}
