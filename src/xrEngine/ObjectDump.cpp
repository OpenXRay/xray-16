#include "stdafx.h"
#include "xrEngine/xr_object.h"

#ifdef DEBUG
#include "ObjectDump.h"
#include "xrCore/dump_string.h"

// XXX: replace std::string with xr_string and optimize

ENGINE_API std::string dbg_object_base_dump_string(const IGameObject* obj)
{
    if (!obj)
        return make_string("object: NULL ptr");
    return make_string("object name: %s, section name: %s, visual name: %s \n", obj->cName().c_str(),
        obj->cNameSect().c_str(), obj->Visual() ? obj->cNameVisual().c_str() : "none");
}

ENGINE_API std::string dbg_object_poses_dump_string(const IGameObject* obj)
{
    if (!obj)
        return std::string();

    u32 ps_size = obj->ps_Size();
    std::string buf("");
    for (u32 i = 0; i < ps_size; ++i)
    {
        const GameObjectSavedPosition& svp = obj->ps_Element(i);
        buf += (make_string(" \n %d, time: %d pos: %s ", i, svp.dwTime, get_string(svp.vPosition).c_str()));
    }

    return make_string("\n XFORM: %s \n position stack : %s \n, ", get_string(obj->XFORM()).c_str(), buf.c_str());
}

ENGINE_API std::string dbg_object_visual_geom_dump_string(const IGameObject* obj)
{
    if (!obj || !obj->Visual())
        return std::string();
    const Fbox& box = obj->BoundingBox();
    Fvector c;
    obj->Center(c);

    return make_string("\n visual box: %s \n visual center: %s \n visual radius: %f ", get_string(box).c_str(),
        get_string(c).c_str(), obj->Radius());
}

/*
 struct
 {
 u32 net_ID : 16;
 u32 bActiveCounter : 8;
 u32 bEnabled : 1;
 u32 bVisible : 1;
 u32 bDestroy : 1;
 u32 net_Local : 1;
 u32 net_Ready : 1;
 u32 net_SV_Update : 1;
 u32 crow : 1;
 u32 bPreDestroy : 1;
 };
 u32 dbg_update_cl;
 #endif
 u32 dwFrame_UpdateCL;
 u32 dwFrame_AsCrow;
 */
ENGINE_API std::string dbg_object_props_dump_string(const IGameObject* obj)
{
    if (!obj)
        return std::string();
    GameObjectProperties props;
    obj->DBGGetProps(props);
    const char* format =
        " "
        "net_ID :%d, bActiveCounter :%d, bEnabled :%s, bVisible :%s, bDestroy :%s, \n "
        "net_Local %s, net_Ready :%s, net_SV_Update :%s, crow :%s, bPreDestroy : %s \n "
        "dbg_update_cl: %d, dwFrame_UpdateCL: %d, dwFrame_AsCrow :%d, Device.dwFrame :%d, Device.dwTimeGlobal: %d \n";
    auto enabled = get_string(bool(!!props.bEnabled));
    auto visible = get_string(bool(!!props.bVisible));
    auto destroy = get_string(bool(!!props.bDestroy));
    auto netLocal = get_string(bool(!!props.net_Local));
    auto netReady = get_string(bool(!!props.net_Ready));
    auto netSvUpdate = get_string(bool(!!props.net_SV_Update));
    auto crow = get_string(bool(!!props.crow));
    auto preDestroy = get_string(bool(!!props.bPreDestroy));
    auto updateFrameDbg = obj->GetDbgUpdateFrame();
    auto updateFrame = obj->GetUpdateFrame();
    auto updateFrameCrow = obj->GetCrowUpdateFrame();
    return make_string(format, props.net_ID, props.bActiveCounter, enabled.c_str(), visible.c_str(), destroy.c_str(), netLocal.c_str(), netReady.c_str(),
        netSvUpdate.c_str(), crow.c_str(), preDestroy.c_str(), updateFrameDbg, updateFrame, updateFrameCrow, Device.dwFrame,
        Device.dwTimeGlobal);
}
ENGINE_API std::string dbg_object_full_dump_string(const IGameObject* obj)
{
    return dbg_object_base_dump_string(obj) + dbg_object_props_dump_string(obj) + dbg_object_poses_dump_string(obj) +
        dbg_object_visual_geom_dump_string(obj);
}
ENGINE_API std::string dbg_object_full_capped_dump_string(const IGameObject* obj)
{
    return std::string("\n object dump: \n") + dbg_object_full_dump_string(obj);
}

#endif // DEBUG
