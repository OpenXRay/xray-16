////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_Abstract.cpp
//	Created 	: 19.09.2002
//  Modified 	: 14.07.2004
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop
#pragma pack(push,4)

#include "xrServer_Objects_Abstract.h"
#include "xrMessages.h"

////////////////////////////////////////////////////////////////////////////
// CSE_Visual
////////////////////////////////////////////////////////////////////////////
CSE_Visual::CSE_Visual		   	(LPCSTR name)
{
	if(name)
	{
		string_path					tmp;
		xr_strcpy						(tmp, name);
		if(strext(tmp)) 
			*strext(tmp)			=0;
		xr_strlwr					(tmp);
		visual_name					= tmp;
	}else
		visual_name					= NULL;

    startup_animation			= "$editor";
	flags.zero					();
}

CSE_Visual::~CSE_Visual			()
{
}

void CSE_Visual::set_visual	   	(LPCSTR name, bool load)
{
	string_path					tmp;
    xr_strcpy						(tmp,name);
    if (strext(tmp))		 	*strext(tmp) = 0;
	xr_strlwr					(tmp);
	visual_name					= tmp; 
}

void CSE_Visual::visual_read   	(NET_Packet &tNetPacket, u16 version)
{
	tNetPacket.r_stringZ		(visual_name);
	if (version>103)
		flags.assign			(tNetPacket.r_u8());
}

void CSE_Visual::visual_write  	(NET_Packet	&tNetPacket)
{
	tNetPacket.w_stringZ		(visual_name);
	tNetPacket.w_u8				(flags.get());
}

void CSE_Visual::OnChangeVisual	(PropValue* sender)
{
	ISE_Abstract* abstract		= smart_cast<ISE_Abstract*>(this); VERIFY(abstract);
	abstract->set_editor_flag	(ISE_Abstract::flVisualChange);
}

void CSE_Visual::OnChangeAnim(PropValue* sender)
{
	ISE_Abstract* abstract		= smart_cast<ISE_Abstract*>(this); VERIFY(abstract);
	abstract->set_editor_flag	(ISE_Abstract::flVisualAnimationChange);
}

#ifndef XRGAME_EXPORTS
void CSE_Visual::FillProps		(LPCSTR pref, PropItemVec &items)
{
	ISE_Abstract* abstract		= smart_cast<ISE_Abstract*>(this); VERIFY(abstract);
	ChooseValue *V 				= PHelper().CreateChoose(items, PrepareKey(pref,abstract->name(),"Model\\Visual"),		&visual_name,		smVisual);
	V->OnChangeEvent.bind		(this,&CSE_Visual::OnChangeVisual);
	V							= PHelper().CreateChoose(items,	PrepareKey(pref,abstract->name(),"Model\\Animation"),	&startup_animation, smSkeletonAnims,0,(void*)*visual_name);
	V->OnChangeEvent.bind		(this,&CSE_Visual::OnChangeAnim);
	PHelper().CreateFlag8		(items, PrepareKey(pref,abstract->name(),"Model\\Obstacle"),	&flags,	flObstacle);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_Animated
////////////////////////////////////////////////////////////////////////////
CSE_Motion::CSE_Motion			(LPCSTR name)
{
	motion_name					= name;
}

CSE_Motion::~CSE_Motion			()
{
}

void CSE_Motion::set_motion		(LPCSTR name)
{
	motion_name					= name;
}

void CSE_Motion::motion_read	(NET_Packet	&tNetPacket)
{
	tNetPacket.r_stringZ		(motion_name);
}

void CSE_Motion::motion_write	(NET_Packet	&tNetPacket)
{
	tNetPacket.w_stringZ			(motion_name);
}

void CSE_Motion::OnChangeMotion	(PropValue* sender)
{
	ISE_Abstract* abstract		= smart_cast<ISE_Abstract*>(this); VERIFY(abstract);
	abstract->set_editor_flag	(ISE_Abstract::flMotionChange);
}

#ifndef XRGAME_EXPORTS
void CSE_Motion::FillProps(		LPCSTR pref, PropItemVec &items)
{
	ISE_Abstract* abstract		= smart_cast<ISE_Abstract*>(this); VERIFY(abstract);
	ChooseValue *V				= PHelper().CreateChoose(items, PrepareKey(pref,abstract->name(),"Motion"),&motion_name, smGameAnim);
	V->OnChangeEvent.bind		(this,&CSE_Motion::OnChangeMotion);
}
#endif // #ifndef XRGAME_EXPORTS

#pragma pack(pop,4)
