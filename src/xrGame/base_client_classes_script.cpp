////////////////////////////////////////////////////////////////////////////
//	Module 		: base_client_classes_script.cpp
//	Created 	: 20.12.2004
//  Modified 	: 20.12.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay base client classes script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "base_client_classes_wrappers.h"
#include "xrEngine/Feel_Sound.h"
#include "Include/xrRender/RenderVisual.h"
#include "Include/xrRender/Kinematics.h"
#include "ai/stalker/ai_stalker.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(IFactoryObject, (), {
    module(luaState)[
        // 'DLL_Pure' is preserved to maintain backward compatibility with mod scripts
        class_<IFactoryObject, no_bases, default_holder, FactoryObjectWrapper>("DLL_Pure")
            .def(constructor<>())
            .def("_construct", &IFactoryObject::_construct, &FactoryObjectWrapper::_construct_static)];
});

/*
SCRIPT_EXPORT(ISpatial, (),
{
    module(luaState)
    [
        class_<ISpatial, no_bases, default_holder, CISpatialWrapper>("ISpatial")
            .def(constructor<>())
            .def("spatial_register",	&ISpatial::spatial_register,	&CISpatialWrapper::spatial_register_static)
            .def("spatial_unregister",	&ISpatial::spatial_unregister,	&CISpatialWrapper::spatial_unregister_static)
            .def("spatial_move",		&ISpatial::spatial_move,		&CISpatialWrapper::spatial_move_static)
            .def("spatial_sector_point",&ISpatial::spatial_sector_point,&CISpatialWrapper::spatial_sector_point_static)
            .def("dcast_GameObject",	&ISpatial::dcast_GameObject,	&CISpatialWrapper::dcast_CObject_static)
            .def("dcast_FeelSound",		&ISpatial::dcast_FeelSound,		&CISpatialWrapper::dcast_FeelSound_static)
            .def("dcast_Renderable",	&ISpatial::dcast_Renderable,	&CISpatialWrapper::dcast_Renderable_static)
            .def("dcast_Light",			&ISpatial::dcast_Light,			&CISpatialWrapper::dcast_Light_static)
    ];
});
*/

SCRIPT_EXPORT(ISheduled, (), {
    module(luaState)[class_<ISheduled, no_bases, default_holder, CISheduledWrapper>("ISheduled")
        //			.def(constructor<>())
        //			.def("shedule_Scale",		&ISheduled::shedule_Scale, &CISheduledWrapper::shedule_Scale_static)
        //			.def("shedule_Update",		&ISheduled::shedule_Update,
        //&CISheduledWrapper::shedule_Update_static)
    ];
});

SCRIPT_EXPORT(IRenderable, (), {
    module(luaState)[class_<IRenderable, no_bases, default_holder, CIRenderableWrapper>("IRenderable")
        //			.def(constructor<>())
        //			.def("renderable_Render",&IRenderable::renderable_Render,&CIRenderableWrapper::renderable_Render_static)
        //			.def("renderable_ShadowGenerate",&IRenderable::renderable_ShadowGenerate,&CIRenderableWrapper::renderable_ShadowGenerate_static)
        //			.def("renderable_ShadowReceive",&IRenderable::renderable_ShadowReceive,&CIRenderableWrapper::renderable_ShadowReceive_static)
    ];
});

SCRIPT_EXPORT(ICollidable, (), { module(luaState)[class_<ICollidable>("ICollidable")]; });

SCRIPT_EXPORT(CGameObject, (IFactoryObject, ISheduled, ICollidable, IRenderable), {
    module(luaState)[
        //		class_<IGameObject,bases<IFactoryObject,ISheduled,ICollidable,IRenderable>,CObjectWrapper>("IGameObject")
        //			.def(constructor<>())
        //			.def("_construct",			&IGameObject::_construct,&CObjectWrapper::_construct_static)
        /*
                    .def("spatial_register",	&IGameObject::spatial_register,
           &CObjectWrapper::spatial_register_static)
                    .def("spatial_unregister",	&IGameObject::spatial_unregister,
           &CObjectWrapper::spatial_unregister_static)
                    .def("spatial_move",		&IGameObject::spatial_move,		&CObjectWrapper::spatial_move_static)
                    .def("spatial_sector_point",&IGameObject::spatial_sector_point,&CObjectWrapper::spatial_sector_point_static)
                    .def("dcast_FeelSound",		&IGameObject::dcast_FeelSound,
           &CObjectWrapper::dcast_FeelSound_static)
                    .def("dcast_Light",			&IGameObject::dcast_Light,			&CObjectWrapper::dcast_Light_static)
        */
        //			.def("shedule_Scale",		&IGameObject::shedule_Scale,
        //&CObjectWrapper::shedule_Scale_static)
        //			.def("shedule_Update",		&IGameObject::shedule_Update,
        //&CObjectWrapper::shedule_Update_static)

        //			.def("renderable_Render"
        //,&IGameObject::renderable_Render,&CObjectWrapper::renderable_Render_static)
        //			.def("renderable_ShadowGenerate",&IGameObject::renderable_ShadowGenerate,&CObjectWrapper::renderable_ShadowGenerate_static)
        //			.def("renderable_ShadowReceive",&IGameObject::renderable_ShadowReceive,&CObjectWrapper::renderable_ShadowReceive_static)
        //			.def("Visual",					&IGameObject::Visual)

        class_<CGameObject, bases<IFactoryObject, ISheduled, ICollidable, IRenderable>, default_holder,
            CGameObjectWrapper>("CGameObject")
            .def(constructor<>())
            .def("_construct", &CGameObject::_construct, &CGameObjectWrapper::_construct_static)
            .def("Visual", &CGameObject::Visual)
            /*
                        .def("spatial_register",	&CGameObject::spatial_register,
               &CGameObjectWrapper::spatial_register_static)
                        .def("spatial_unregister",	&CGameObject::spatial_unregister,
               &CGameObjectWrapper::spatial_unregister_static)
                        .def("spatial_move",		&CGameObject::spatial_move,
               &CGameObjectWrapper::spatial_move_static)
                        .def("spatial_sector_point",&CGameObject::spatial_sector_point,&CGameObjectWrapper::spatial_sector_point_static)
                        .def("dcast_FeelSound",		&CGameObject::dcast_FeelSound,
               &CGameObjectWrapper::dcast_FeelSound_static)
                        .def("dcast_Light",			&CGameObject::dcast_Light,
               &CGameObjectWrapper::dcast_Light_static)
            */
            //			.def("shedule_Scale",		&CGameObject::shedule_Scale,
            //&CGameObjectWrapper::shedule_Scale_static)
            //			.def("shedule_Update",		&CGameObject::shedule_Update,
            //&CGameObjectWrapper::shedule_Update_static)

            //			.def("renderable_Render"
            //,&CGameObject::renderable_Render,&CGameObjectWrapper::renderable_Render_static)
            //			.def("renderable_ShadowGenerate",&CGameObject::renderable_ShadowGenerate,&CGameObjectWrapper::renderable_ShadowGenerate_static)
            //			.def("renderable_ShadowReceive",&CGameObject::renderable_ShadowReceive,&CGameObjectWrapper::renderable_ShadowReceive_static)

            .def("net_Export", &CGameObject::net_Export, &CGameObjectWrapper::net_Export_static)
            .def("net_Import", &CGameObject::net_Import, &CGameObjectWrapper::net_Import_static)
            .def("net_Spawn", &CGameObject::net_Spawn, &CGameObjectWrapper::net_Spawn_static)

            .def("use", &CGameObject::use, &CGameObjectWrapper::use_static)

            //			.def("setVisible",			&CGameObject::setVisible)
            .def("getVisible", &CGameObject::getVisible)
            .def("getEnabled", &CGameObject::getEnabled)
        //			.def("setEnabled",			&CGameObject::setEnabled)

        //		,class_<CPhysicsShellHolder,CGameObject>("CPhysicsShellHolder")
        //			.def(constructor<>())

        //		,class_<CEntity,CPhysicsShellHolder,CEntityWrapper>("CEntity")
        //			.def(constructor<>())
        //			.def("HitSignal",&CEntity::HitSignal,&CEntityWrapper::HitSignal_static)
        //			.def("HitImpulse",&CEntity::HitImpulse,&CEntityWrapper::HitImpulse_static)

        //		,class_<CEntityAlive,CEntity>("CEntityAlive")
        //			.def(constructor<>())

        //		,class_<CCustomMonster,CEntityAlive>("CCustomMonster")
        //			.def(constructor<>())

        //		,class_<CAI_Stalker,CCustomMonster>("CAI_Stalker")
    ];
});

SCRIPT_EXPORT(IRenderVisual, (), {
    module(luaState)[class_<IRenderVisual>("IRender_Visual")
                         //			.def(constructor<>())
                         .def("dcast_PKinematicsAnimated", &IRenderVisual::dcast_PKinematicsAnimated)];
});

void IKinematicsAnimated_PlayCycle(IKinematicsAnimated* sa, LPCSTR anim) { sa->PlayCycle(anim); }
SCRIPT_EXPORT(IKinematicsAnimated, (), {
    module(
        luaState)[class_<IKinematicsAnimated>("IKinematicsAnimated").def("PlayCycle", &IKinematicsAnimated_PlayCycle)];
});

SCRIPT_EXPORT(CBlend, (), {
    module(luaState)[class_<CBlend>("CBlend")
        //			.def(constructor<>())
    ];
});

/*
SCRIPT_EXPORT(IKinematics, (FHierrarhyVisual),
{
    module(luaState)
    [
        class_<IKinematics, FHierrarhyVisual>("IKinematics")
        //			.def(constructor<>())
    ];
});

SCRIPT_EXPORT(FHierrarhyVisual, (IRenderVisual),
{
    module(luaState)
    [
        class_<FHierrarhyVisual, IRenderVisual>("FHierrarhyVisual")
        //			.def(constructor<>())
    ];
});
*/
