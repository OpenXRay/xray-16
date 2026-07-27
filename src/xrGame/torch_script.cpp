#include "pch_script.h"

#include "Torch.h"
#include "PDA.h"
#include "SimpleDetector.h"
#include "EliteDetector.h"
#include "AdvancedDetector.h"

void CTorch::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CTorch, CGameObject>("CTorch")
            .def(constructor<>())
            .def("torch_set_color_r", &CTorch::torch_set_color_r)
            .def("torch_set_color_g", &CTorch::torch_set_color_g)
            .def("torch_set_color_b", &CTorch::torch_set_color_b)
            .def("torch2_set_color_r", &CTorch::torch2_set_color_r)
            .def("torch2_set_color_g", &CTorch::torch2_set_color_g)
            .def("torch2_set_color_b", &CTorch::torch2_set_color_b)
            .def("torch2_set_offset_x", &CTorch::torch2_set_offset_x)
            .def("torch2_set_offset_y", &CTorch::torch2_set_offset_y)
            .def("torch2_set_radius", &CTorch::torch2_set_radius)
            .def("torch2_set_range", &CTorch::torch2_set_range)
            .def("torch_set_color_a", &CTorch::torch_set_color_a)
            .def("torch_set_offset_y", &CTorch::torch_set_offset_y)
            .def("torch_set_offset_z", &CTorch::torch_set_offset_z)
            .def("torch_set_radius", &CTorch::torch_set_radius)
            .def("torch_set_range", &CTorch::torch_set_range)
            .def("torch_set_inertion", &CTorch::torch_set_inertion)
            .def("torch_set_animation", &CTorch::torch_set_animation)
            .def("torch_set_texture", &CTorch::torch_set_texture)
            .def("torch_switch_spot", &CTorch::torch_switch_spot)
            .def("enable_torch", &CTorch::enable_torch)
            .def("enable_torch2", &CTorch::enable_torch2)
            .def("torch_enabled", &CTorch::torch_enabled),
        class_<CPda, CGameObject>("CPda")
            .def(constructor<>()),
        class_<CScientificDetector, CGameObject>("CScientificDetector")
            .def(constructor<>()),
        class_<CEliteDetector, CGameObject>("CEliteDetector")
            .def(constructor<>()),
        class_<CAdvancedDetector, CGameObject>("CAdvancedDetector")
            .def(constructor<>()),
        class_<CSimpleDetector, CGameObject>("CSimpleDetector")
            .def(constructor<>())
    ];
}
