#include "pch_script.h"
#include "UIGame_custom_script.h"
#include "xrServer_script_macroses.h"
#include "ui/UIMultiTextStatic.h"

template <typename T>
struct CUIGameCustomWrapperBase : public T, public luabind::wrap_base
{
    typedef T inherited;
    typedef CUIGameCustomWrapperBase<T> self_type;

    DEFINE_LUA_WRAPPER_METHOD_V0(Init)
    DEFINE_LUA_WRAPPER_METHOD_V1(SetClGame, game_cl_GameState*)
};

void UIGame_custom_script::script_register(lua_State* L)
{
    using namespace luabind;

    using BaseType = UIGame_custom_script;
    using WrapType = CUIGameCustomWrapperBase<UIGame_custom_script>;

    module(L)
    [
        class_<UIGame_custom_script, CUIGameCustom, WrapType>("UIGame_custom_script")
            .def(constructor<>())
            .def("Init", &BaseType::Init, &WrapType::Init_static)
            .def("SetClGame", &BaseType::SetClGame, &WrapType::SetClGame_static)
    ];
}
