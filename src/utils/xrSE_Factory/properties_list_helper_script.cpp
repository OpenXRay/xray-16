////////////////////////////////////////////////////////////////////////////
//	Module 		: properties_list_helper_script.cpp
//	Created 	: 14.07.2004
//  Modified 	: 14.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Properties list helper script export
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "script_space.h"
#include "script_properties_list_helper.h"
#include "ai_space.h"
#include "xrCore/ModuleLookup.hpp"
#include "xrScriptEngine/script_engine.hpp"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrServerEntities/script_token_list.h"

using namespace luabind;

struct CChooseType
{
};

using TPHelper = IPropHelper&(__stdcall* )();

TPHelper _PHelper = nullptr;
XRay::Module prop_helper_module;
constexpr pcstr prop_helper_library = "xrEPropsB", prop_helper_func = "PHelper";
CScriptPropertiesListHelper* g_property_list_helper = nullptr;

void load_prop_helper()
{
    prop_helper_module = XRay::LoadModule(prop_helper_library);
    if (!prop_helper_module->IsLoaded())
    {
        Msg("! Cannot find library %s", prop_helper_library);
        return;
    }

    _PHelper = (TPHelper)prop_helper_module->GetProcAddress(prop_helper_func);
    if (!_PHelper)
    {
        Msg("! Cannot find entry point of the function %s in the library %s", prop_helper_func, prop_helper_func);
        return;
    }

    g_property_list_helper = new CScriptPropertiesListHelper();
}

IPropHelper& PHelper()
{
    static bool first_time = true;
    if (first_time)
    {
        first_time = false;
        load_prop_helper();
    }
    R_ASSERT3(_PHelper, "Cannot find entry point of the function or Cannot find library", prop_helper_library);
    return (_PHelper());
}

CScriptPropertiesListHelper* properties_helper()
{
    if (!g_property_list_helper)
        ai().script_engine().script_log(
            LuaMessageType::Error, "Editor is not started, therefore prop_helper cannot be accessed!");

    return (g_property_list_helper);
}

SCRIPT_EXPORT(
    CScriptPropertiesListHelper, (), {
        module(luaState)[class_<PropValue>("prop_value"), class_<PropItemVec>("prop_item_vec"),
            class_<CaptionValue>("caption_value"), class_<CanvasValue>("canvas_value"),
            class_<ButtonValue>("button_value"), class_<ChooseValue>("choose_value"), class_<S8Value>("s8_value"),
            class_<S16Value>("s16_value"), class_<S32Value>("s32_value"), class_<U8Value>("u8_value"),
            class_<U16Value>("u16_value"), class_<U32Value>("u32_value"), class_<FloatValue>("u32_value"),
            class_<BOOLValue>("bool_value"), class_<VectorValue>("vector_value"), class_<ColorValue>("color_value"),
            class_<RTextValue>("text_value"), class_<Flag8Value>("flag8_value"), class_<Flag16Value>("flag16_value"),
            class_<Flag32Value>("flag32_value"), class_<Token8Value>("token8_value"),
            class_<Token16Value>("token16_value"), class_<Token32Value>("token32_value"),
            //		class_<RToken8Value>("rtoken8_value"),
            //		class_<RToken16Value>("rtoken16_value"),
            //		class_<RToken32Value>("rtoken32_value"),
            class_<RListValue>("list_value"),

            class_<CChooseType>("choose_type")
                .enum_("choose_type")[value("custom", int(smCustom)), value("sound_source", int(smSoundSource)),
                    value("sound_environment", int(smSoundEnv)), value("library_object", int(smObject)),
                    value("engine_shader", int(smEShader)), value("compiler_shader", int(smCShader)),
                    value("particle_effect", int(smPE)), value("particle_system", int(smParticles)),
                    value("texture", int(smTexture)), value("entity", int(smEntityType)),
                    value("spawn_item", int(smSpawnItem)), value("light_animation", int(smLAnim)),
                    value("visual", int(smVisual)), value("skeleton_animations", int(smSkeletonAnims)),
                    value("skeleton_bones", int(smSkeletonBones)), value("material", int(smGameMaterial)),
                    value("game_animation", int(smGameAnim)), value("game_motion", int(smGameSMotions))],

            class_<CScriptPropertiesListHelper>("properties_list_helper")
                .def("vector_on_after_edit", &CScriptPropertiesListHelper::FvectorRDOnAfterEdit)
                .def("vector_on_before_edit", &CScriptPropertiesListHelper::FvectorRDOnBeforeEdit)
                //			.def("vector_on_draw",			&CScriptPropertiesListHelper::FvectorRDOnDraw)
                .def("float_on_after_edit", &CScriptPropertiesListHelper::floatRDOnAfterEdit, luabind::policy::out_value<3>())
                .def("float_on_before_edit", &CScriptPropertiesListHelper::floatRDOnBeforeEdit, luabind::policy::out_value<3>())
                //			.def("float_on_draw",			&CScriptPropertiesListHelper::floatRDOnDraw)
                .def("name_after_edit", &CScriptPropertiesListHelper::NameAfterEdit, luabind::policy::pure_out_value<3>())
                .def("name_before_edit", &CScriptPropertiesListHelper::NameBeforeEdit, luabind::policy::pure_out_value<3>())
                //			.def("name_on_draw",			&CScriptPropertiesListHelper::NameDraw)

                .def("create_caption", &CScriptPropertiesListHelper::CreateCaption)
                .def("create_canvas", &CScriptPropertiesListHelper::CreateCanvas)
                .def("create_button", &CScriptPropertiesListHelper::CreateButton)

                .def("create_choose",
                    (ChooseValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       u32))(&CScriptPropertiesListHelper::CreateChoose))
                .def("create_choose",
                    (ChooseValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u32,
                                       LPCSTR))(&CScriptPropertiesListHelper::CreateChoose))
                .def("create_choose",
                    (ChooseValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u32,
                                       LPCSTR, LPCSTR))(&CScriptPropertiesListHelper::CreateChoose))
                .def("create_choose",
                    (ChooseValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u32,
                                       LPCSTR, LPCSTR, u32))(&CScriptPropertiesListHelper::CreateChoose))

                //			.def("create_s8", (S8Value *(CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR,
                //luabind::object , LPCSTR
                //))					(&CScriptPropertiesListHelper::CreateS8))
                //			.def("create_s8", (S8Value *(CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR,
                //luabind::object , LPCSTR
                //,  s8))				(&CScriptPropertiesListHelper::CreateS8))
                //			.def("create_s8", (S8Value *(CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR,
                //luabind::object , LPCSTR
                //,  s8,  s8))			(&CScriptPropertiesListHelper::CreateS8))
                //			.def("create_s8", (S8Value *(CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR,
                //luabind::object , LPCSTR
                //,  s8,  s8,  s8))	(&CScriptPropertiesListHelper::CreateS8))

                .def("create_s16", (S16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR))(&CScriptPropertiesListHelper::CreateS16))
                .def("create_s16", (S16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, s16))(&CScriptPropertiesListHelper::CreateS16))
                .def("create_s16", (S16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, s16, s16))(&CScriptPropertiesListHelper::CreateS16))
                .def("create_s16", (S16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, s16, s16, s16))(&CScriptPropertiesListHelper::CreateS16))

                .def("create_s32", (S32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR))(&CScriptPropertiesListHelper::CreateS32))
                .def("create_s32", (S32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, s32))(&CScriptPropertiesListHelper::CreateS32))
                .def("create_s32", (S32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, s32, s32))(&CScriptPropertiesListHelper::CreateS32))
                .def("create_s32", (S32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, s32, s32, s32))(&CScriptPropertiesListHelper::CreateS32))

                .def("create_u8", (U8Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                 LPCSTR))(&CScriptPropertiesListHelper::CreateU8))
                .def("create_u8", (U8Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                 LPCSTR, u8))(&CScriptPropertiesListHelper::CreateU8))
                .def("create_u8", (U8Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                 LPCSTR, u8, u8))(&CScriptPropertiesListHelper::CreateU8))
                .def("create_u8", (U8Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                 LPCSTR, u8, u8, u8))(&CScriptPropertiesListHelper::CreateU8))

                .def("create_u16", (U16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR))(&CScriptPropertiesListHelper::CreateU16))
                .def("create_u16", (U16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, u16))(&CScriptPropertiesListHelper::CreateU16))
                .def("create_u16", (U16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, u16, u16))(&CScriptPropertiesListHelper::CreateU16))
                .def("create_u16", (U16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, u16, u16, u16))(&CScriptPropertiesListHelper::CreateU16))

                .def("create_u32", (U32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR))(&CScriptPropertiesListHelper::CreateU32))
                .def("create_u32", (U32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, u32))(&CScriptPropertiesListHelper::CreateU32))
                .def("create_u32", (U32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, u32, u32))(&CScriptPropertiesListHelper::CreateU32))
                .def("create_u32", (U32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                   LPCSTR, u32, u32, u32))(&CScriptPropertiesListHelper::CreateU32))

                .def("create_float",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR))(
                        &CScriptPropertiesListHelper::CreateFloat))
                .def("create_float",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                      float))(&CScriptPropertiesListHelper::CreateFloat))
                .def("create_float",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, float,
                                      float))(&CScriptPropertiesListHelper::CreateFloat))
                .def("create_float",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, float,
                                      float, float))(&CScriptPropertiesListHelper::CreateFloat))
                .def("create_float",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, float,
                                      float, float, int))(&CScriptPropertiesListHelper::CreateFloat))

                .def("create_bool", &CScriptPropertiesListHelper::CreateBOOL)

                .def("create_vector",
                    (VectorValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR))(
                        &CScriptPropertiesListHelper::CreateVector))
                .def("create_vector",
                    (VectorValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       float))(&CScriptPropertiesListHelper::CreateVector))
                .def("create_vector",
                    (VectorValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       float, float))(&CScriptPropertiesListHelper::CreateVector))
                .def("create_vector",
                    (VectorValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       float, float, float))(&CScriptPropertiesListHelper::CreateVector))
                .def("create_vector",
                    (VectorValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       float, float, float, int))(&CScriptPropertiesListHelper::CreateVector))

                .def("create_flag8",
                    (Flag8Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u8))(
                        &CScriptPropertiesListHelper::CreateFlag8))
                .def("create_flag8",
                    (Flag8Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u8,
                                      LPCSTR))(&CScriptPropertiesListHelper::CreateFlag8))
                .def("create_flag8",
                    (Flag8Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u8,
                                      LPCSTR, LPCSTR))(&CScriptPropertiesListHelper::CreateFlag8))
                .def("create_flag8",
                    (Flag8Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u8,
                                      LPCSTR, LPCSTR, u32))(&CScriptPropertiesListHelper::CreateFlag8))

                .def("create_flag16",
                    (Flag16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       u16))(&CScriptPropertiesListHelper::CreateFlag16))
                .def("create_flag16",
                    (Flag16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u16,
                                       LPCSTR))(&CScriptPropertiesListHelper::CreateFlag16))
                .def("create_flag16",
                    (Flag16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u16,
                                       LPCSTR, LPCSTR))(&CScriptPropertiesListHelper::CreateFlag16))
                .def("create_flag16",
                    (Flag16Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u16,
                                       LPCSTR, LPCSTR, u32))(&CScriptPropertiesListHelper::CreateFlag16))

                .def("create_flag32",
                    (Flag32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       u32))(&CScriptPropertiesListHelper::CreateFlag32))
                .def("create_flag32",
                    (Flag32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u32,
                                       LPCSTR))(&CScriptPropertiesListHelper::CreateFlag32))
                .def("create_flag32",
                    (Flag32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u32,
                                       LPCSTR, LPCSTR))(&CScriptPropertiesListHelper::CreateFlag32))
                .def("create_flag32",
                    (Flag32Value * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, u32,
                                       LPCSTR, LPCSTR, u32))(&CScriptPropertiesListHelper::CreateFlag32))

                .def("create_token8", &CScriptPropertiesListHelper::CreateToken8)
                .def("create_token16", &CScriptPropertiesListHelper::CreateToken16)
                .def("create_token32", &CScriptPropertiesListHelper::CreateToken32)

                //			.def("create_rtoken8",	&CScriptPropertiesListHelper::CreateRToken8)
                //			.def("create_rtoken16",	&CScriptPropertiesListHelper::CreateRToken16)
                //			.def("create_rtoken32",	&CScriptPropertiesListHelper::CreateRToken32)

                .def("create_list", &CScriptPropertiesListHelper::CreateRList)

                .def("create_color", &CScriptPropertiesListHelper::CreateColor)
                .def("create_fcolor", &CScriptPropertiesListHelper::CreateFColor)
                .def("create_vcolor", &CScriptPropertiesListHelper::CreateVColor)

                .def("create_text", &CScriptPropertiesListHelper::CreateRText)

                .def("create_time",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR))(
                        &CScriptPropertiesListHelper::CreateTime))
                .def(
                    "create_time", (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                     LPCSTR, float))(&CScriptPropertiesListHelper::CreateTime))
                .def(
                    "create_time", (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object,
                                                     LPCSTR, float, float))(&CScriptPropertiesListHelper::CreateTime))

                .def("create_angle",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR))(
                        &CScriptPropertiesListHelper::CreateAngle))
                .def("create_angle",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                      float))(&CScriptPropertiesListHelper::CreateAngle))
                .def("create_angle",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, float,
                                      float))(&CScriptPropertiesListHelper::CreateAngle))
                .def("create_angle",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, float,
                                      float, float))(&CScriptPropertiesListHelper::CreateAngle))
                .def("create_angle",
                    (FloatValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR, float,
                                      float, float, int))(&CScriptPropertiesListHelper::CreateAngle))

                .def("create_vangle",
                    (VectorValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR))(
                        &CScriptPropertiesListHelper::CreateAngle3))
                .def("create_vangle",
                    (VectorValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       float))(&CScriptPropertiesListHelper::CreateAngle3))
                .def("create_vangle",
                    (VectorValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       float, float))(&CScriptPropertiesListHelper::CreateAngle3))
                .def("create_vangle",
                    (VectorValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       float, float, float))(&CScriptPropertiesListHelper::CreateAngle3))
                .def("create_vangle",
                    (VectorValue * (CScriptPropertiesListHelper::*)(PropItemVec*, LPCSTR, luabind::object, LPCSTR,
                                       float, float, float, int))(&CScriptPropertiesListHelper::CreateAngle3))

                ,
            def("properties_helper", &properties_helper)];
    });
