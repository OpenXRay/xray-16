#include "pch_script.h"
#include "game_sv_deathmatch.h"
#include "xrServer_script_macroses.h"
#include "xrServer.h"

template <typename T>
struct CWrapperBase : T, luabind::wrap_base
{
    using inherited = T;
    using self_type = CWrapperBase<T>;
    DEFINE_LUA_WRAPPER_CONST_METHOD_0(type_name, pcstr)
    //	DEFINE_LUA_WRAPPER_METHOD_1(Money_SetStart, void, u32)
};

SCRIPT_EXPORT(game_sv_Deathmatch, (game_sv_GameState),
{
    using WrapType = CWrapperBase<game_sv_Deathmatch>;
    using namespace luabind;

    module(luaState)
    [
        class_<game_sv_Deathmatch, game_sv_GameState, default_holder, WrapType>("game_sv_Deathmatch")
            .def(constructor<>())
            .def("GetTeamData", &game_sv_Deathmatch::GetTeamData)

            .def("type_name", &game_sv_Deathmatch::type_name, &WrapType::type_name_static)
        //			.def("Money_SetStart",		&CWrapperBase<game_sv_Deathmatch>::Money_SetStart,		&CWrapperBase<game_sv_Deathmatch>::Money_SetStart_static)
    ];
});
