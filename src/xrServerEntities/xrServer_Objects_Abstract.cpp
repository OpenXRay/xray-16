////////////////////////////////////////////////////////////////////////////
//  Module      : xrServer_Objects_Abstract.cpp
//  Created     : 19.09.2002
//  Modified    : 14.07.2004
//  Author      : Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//  Description : Server objects
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#pragma hdrstop
#pragma pack(push, 4)

#include "xrServer_Objects_Abstract.h"
#include "xrMessages.h"

////////////////////////////////////////////////////////////////////////////
// CSE_Visual
////////////////////////////////////////////////////////////////////////////
CSE_Visual::CSE_Visual(LPCSTR name)
{
    if (name)
    {
        string_path tmp;
        xr_strcpy(tmp, name);
        if (strext(tmp))
            *strext(tmp) = 0;
        xr_strlwr(tmp);
        visual_name = tmp;
    }
    else
        visual_name = nullptr;

    startup_animation = "$editor";
    flags.zero();
}

CSE_Visual::~CSE_Visual() {}
void CSE_Visual::set_visual(LPCSTR name, bool load)
{
    string_path tmp;
    xr_strcpy(tmp, name);
    if (strext(tmp))
        *strext(tmp) = 0;
    xr_strlwr(tmp);
    visual_name = tmp;
}

void CSE_Visual::visual_read(NET_Packet& tNetPacket, u16 version)
{
    tNetPacket.r_stringZ(visual_name);
    if (version > 103)
        flags.assign(tNetPacket.r_u8());
}

void CSE_Visual::visual_write(NET_Packet& tNetPacket)
{
    tNetPacket.w_stringZ(visual_name);
    tNetPacket.w_u8(flags.get());
}

void CSE_Visual::OnChangeVisual(PropValue* sender)
{
    IServerEntity* abstract = smart_cast<IServerEntity*>(this);
    VERIFY(abstract);
    abstract->set_editor_flag(IServerEntity::flVisualChange);
}

void CSE_Visual::OnChangeAnim(PropValue* sender)
{
    IServerEntity* abstract = smart_cast<IServerEntity*>(this);
    VERIFY(abstract);
    abstract->set_editor_flag(IServerEntity::flVisualAnimationChange);
}

#ifndef XRGAME_EXPORTS
void CSE_Visual::FillProps(LPCSTR pref, PropItemVec& items)
{
    IServerEntity* abstract = smart_cast<IServerEntity*>(this);
    VERIFY(abstract);
    ChooseValue* V =
        PHelper().CreateChoose(items, PrepareKey(pref, abstract->name(), "Model" DELIMITER "Visual"), &visual_name, smVisual);
    V->OnChangeEvent.bind(this, &CSE_Visual::OnChangeVisual);
    V = PHelper().CreateChoose(items, PrepareKey(pref, abstract->name(), "Model" DELIMITER "Animation"), &startup_animation,
        smSkeletonAnims, nullptr, (void*)*visual_name);
    V->OnChangeEvent.bind(this, &CSE_Visual::OnChangeAnim);
    PHelper().CreateFlag8(items, PrepareKey(pref, abstract->name(), "Model" DELIMITER "Obstacle"), &flags, flObstacle);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_Animated
////////////////////////////////////////////////////////////////////////////
CSE_Motion::CSE_Motion(LPCSTR name) { motion_name = name; }
CSE_Motion::~CSE_Motion() {}
void CSE_Motion::set_motion(LPCSTR name) { motion_name = name; }
void CSE_Motion::motion_read(NET_Packet& tNetPacket) { tNetPacket.r_stringZ(motion_name); }
void CSE_Motion::motion_write(NET_Packet& tNetPacket) { tNetPacket.w_stringZ(motion_name); }
void CSE_Motion::OnChangeMotion(PropValue* sender)
{
    IServerEntity* abstract = smart_cast<IServerEntity*>(this);
    VERIFY(abstract);
    abstract->set_editor_flag(IServerEntity::flMotionChange);
}

#ifndef XRGAME_EXPORTS
void CSE_Motion::FillProps(LPCSTR pref, PropItemVec& items)
{
    IServerEntity* abstract = smart_cast<IServerEntity*>(this);
    VERIFY(abstract);
    ChooseValue* V =
        PHelper().CreateChoose(items, PrepareKey(pref, abstract->name(), "Motion"), &motion_name, smGameAnim);
    V->OnChangeEvent.bind(this, &CSE_Motion::OnChangeMotion);
}
#endif // #ifndef XRGAME_EXPORTS

#pragma pack(pop, 4)
