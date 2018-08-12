#include "StdAfx.h"

#ifdef DEBUG

std::string dbg_object_base_dump_string(const CObject* obj)
{
    if (!obj)
        return make_string("object: NULL ptr");
    return make_string("object name: %s, section name: %s, visual name: %s \n", obj->cName().c_str(),
        obj->cNameSect().c_str(), obj->Visual() ? obj->cNameVisual().c_str() : "none");
}
std::string get_string(const Fvector& v) { return make_string("( %f, %f, %f )", v.x, v.y, v.z); }
std::string get_string(const Fmatrix& dop)
{
    return make_string("\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n", dop.i.x, dop.i.y, dop.i.z, dop._14_,
        dop.j.x, dop.j.y, dop.j.z, dop._24_, dop.k.x, dop.k.y, dop.k.z, dop._34_, dop.c.x, dop.c.y, dop.c.z, dop._44_);
}
std::string get_string(const Fbox& box)
{
    return make_string("[ min: %s - max: %s ]", get_string(box.min).c_str(), get_string(box.max).c_str());
}
std::string dbg_object_poses_dump_string(const CObject* obj)
{
    if (!obj)
        return std::string("");

    u32 ps_size = obj->ps_Size();
    std::string buf("");
    for (u32 i = 0; i < ps_size; ++i)
    {
        const CObject::SavedPosition& svp = obj->ps_Element(i);
        buf += (make_string(" \n %d, time: %d pos: %s  ", i, svp.dwTime, get_string(svp.vPosition).c_str()));
    }

    return make_string("\n XFORM: %s \n position stack : %s \n,  ", get_string(obj->XFORM()).c_str(), buf.c_str());
}

std::string dbg_object_visual_geom_dump_string(const CObject* obj)
{
    if (!obj || !obj->Visual())
        return std::string("");
    const Fbox& box = obj->BoundingBox();
    Fvector c;
    obj->Center(c);

    return make_string("\n visual box: %s \n visual center: %s \n visual radius: %f ", get_string(box).c_str(),
        get_string(c).c_str(), obj->Radius());
}
std::string get_string(bool v) { return v ? std::string("true") : std::string("false"); }
/*
        struct
        {
            u32	net_ID			:	16;
            u32	bActiveCounter	:	8;
            u32	bEnabled		:	1;
            u32	bVisible		:	1;
            u32	bDestroy		:	1;
            u32	net_Local		:	1;
            u32	net_Ready		:	1;
            u32 net_SV_Update	:	1;
            u32 crow			:	1;
            u32	bPreDestroy		:	1;
        };
            u32									dbg_update_cl;
#endif
    u32									dwFrame_UpdateCL;
    u32									dwFrame_AsCrow;
*/
std::string dbg_object_props_dump_string(const CObject* obj)
{
    if (!obj)
        return std::string("");
    CObject::ObjectProperties props;
    obj->DBGGetProps(props);

    return make_string(
               " net_ID :%d, bActiveCounter :%d, bEnabled :%s, bVisible :%s, bDestroy :%s, \n net_Local %s, net_Ready "
               ":%s, "
               "net_SV_Update :%s, crow :%s, bPreDestroy : %s ",
               props.net_ID, props.bActiveCounter, get_string(bool(!!props.bEnabled)).c_str(),
               get_string(bool(!!props.bVisible)).c_str(), get_string(bool(!!props.bDestroy)).c_str(),
               get_string(bool(!!props.net_Local)).c_str(), get_string(bool(!!props.net_Ready)).c_str(),
               get_string(bool(!!props.net_SV_Update)).c_str(), get_string(bool(!!props.crow)).c_str(),
               get_string(bool(!!props.bPreDestroy)).c_str()) +
        make_string(
            "\n dbg_update_cl: %d, dwFrame_UpdateCL: %d, dwFrame_AsCrow :%d, Device.dwFrame :%d, Device.dwTimeGlobal: "
            "%d  \n",
            obj->dbg_update_cl, obj->dwFrame_UpdateCL, obj->dwFrame_AsCrow, Device.dwFrame, Device.dwTimeGlobal);
}
std::string dbg_object_full_dump_string(const CObject* obj)
{
    return dbg_object_base_dump_string(obj) + dbg_object_props_dump_string(obj) + dbg_object_poses_dump_string(obj) +
        dbg_object_visual_geom_dump_string(obj);
}
std::string dbg_object_full_capped_dump_string(const CObject* obj)
{
    return std::string("\n object dump: \n") + dbg_object_full_dump_string(obj);
}
#endif
