////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects.h"
#include "game_base_space.h"

////////////////////////////////////////////////////////////////////////////
// CSE_Shape
////////////////////////////////////////////////////////////////////////////
CSE_Shape::CSE_Shape						()
{
}

CSE_Shape::~CSE_Shape						()
{
}

void CSE_Shape::cform_read					(NET_Packet	&tNetPacket)
{
	shapes.clear				();
	u8							count;
	tNetPacket.r_u8				(count);
	
	while (count) {
		shape_def				S;
		tNetPacket.r_u8			(S.type);
		switch (S.type) {
			case 0 :
				{
					if(tNetPacket.inistream)
					{
						tNetPacket.r_vec3(S.data.sphere.P);
						tNetPacket.r_float(S.data.sphere.R);
					}else
						tNetPacket.r	(&S.data.sphere,sizeof(S.data.sphere));	

				}break;
			case 1 :	
				tNetPacket.r_matrix(S.data.box);
				break;
		}
		shapes.push_back		(S);
		--count;
	}
}

void CSE_Shape::cform_write					(NET_Packet	&tNetPacket)
{
	tNetPacket.w_u8				(u8(shapes.size()));
	for (u32 i=0; i<shapes.size(); ++i) 
	{
		shape_def				&S = shapes[i];
		tNetPacket.w_u8			(S.type);
		switch (S.type) 
		{
			case 0:
				{
					if(tNetPacket.inistream)
					{
						tNetPacket.w_vec3(S.data.sphere.P);
						tNetPacket.w_float(S.data.sphere.R);
					}else
						tNetPacket.w	(&S.data.sphere,sizeof(S.data.sphere));
				}break;
			case 1:	
				tNetPacket.w_matrix	(S.data.box);
				break;
		}
	}
}

void CSE_Shape::assign_shapes	(CShapeData::shape_def* _shapes, u32 _cnt)
{
	shapes.resize	(_cnt);
	for (u32 k=0; k<_cnt; k++)
		shapes[k]	= _shapes[k];
}

////////////////////////////////////////////////////////////////////////////
// CSE_Spectator
////////////////////////////////////////////////////////////////////////////
CSE_Spectator::CSE_Spectator				(LPCSTR caSection) : CSE_Abstract(caSection)
{
}

CSE_Spectator::~CSE_Spectator				()
{
}

u8	 CSE_Spectator::g_team					()
{
	return 0;
}

void CSE_Spectator::STATE_Read				(NET_Packet	&tNetPacket, u16 size)
{
}

void CSE_Spectator::STATE_Write				(NET_Packet	&tNetPacket)
{
}

void CSE_Spectator::UPDATE_Read				(NET_Packet	&tNetPacket)
{
}

void CSE_Spectator::UPDATE_Write			(NET_Packet	&tNetPacket)
{
}

#ifndef XRGAME_EXPORTS
void CSE_Spectator::FillProps				(LPCSTR pref, PropItemVec& items)
{
  	inherited::FillProps			(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_Temporary
////////////////////////////////////////////////////////////////////////////
CSE_Temporary::CSE_Temporary				(LPCSTR caSection) : CSE_Abstract(caSection)
{
	m_tNodeID					= u32(-1);
}

CSE_Temporary::~CSE_Temporary				()
{
}

void CSE_Temporary::STATE_Read				(NET_Packet	&tNetPacket, u16 size)
{
	tNetPacket.r_u32			(m_tNodeID);
};

void CSE_Temporary::STATE_Write				(NET_Packet	&tNetPacket)
{
	tNetPacket.w_u32			(m_tNodeID);
};

void CSE_Temporary::UPDATE_Read				(NET_Packet	&tNetPacket)
{
};

void CSE_Temporary::UPDATE_Write			(NET_Packet	&tNetPacket)
{
};

#ifndef XRGAME_EXPORTS
void CSE_Temporary::FillProps				(LPCSTR pref, PropItemVec& values)
{
};
#endif // #ifndef XRGAME_EXPORTS

/**
////////////////////////////////////////////////////////////////////////////
// CSE_SpawnGroup
////////////////////////////////////////////////////////////////////////////

CSE_SpawnGroup::CSE_SpawnGroup				(LPCSTR caSection) : CSE_Abstract(caSection)
{
}

CSE_SpawnGroup::~CSE_SpawnGroup				()
{
}

void CSE_SpawnGroup::STATE_Read				(NET_Packet	&tNetPacket, u16 size)
{
	if (m_wVersion < 84)
		tNetPacket.r_float		(m_spawn_probability);

	if (m_wVersion > 80) {
		if (m_wVersion < 84) {
			tNetPacket.r_float	();
			tNetPacket.r_float	();
			m_spawn_flags.assign(tNetPacket.r_u32());
			tNetPacket.r_stringZ(m_spawn_control);
		}
		else {
			if (m_wVersion < 85) {
				tNetPacket.r_u64		(m_min_spawn_interval);
				tNetPacket.r_u64		(m_max_spawn_interval);
			}
		}
	}
}

void CSE_SpawnGroup::STATE_Write			(NET_Packet	&tNetPacket)
{
}

void CSE_SpawnGroup::UPDATE_Read			(NET_Packet	&tNetPacket)
{
}

void CSE_SpawnGroup::UPDATE_Write			(NET_Packet	&tNetPacket)
{
}

#ifndef XRGAME_EXPORTS
void CSE_SpawnGroup::FillProps				(LPCSTR pref, PropItemVec& values)
{
	inherited::FillProps		(pref,values);
	PHelper().CreateFlag32		(values,PrepareKey(pref,*s_name,"Spawn\\spawn single item only"),	&m_spawn_flags,	flSpawnSingleItemOnly);
}
#endif // #ifndef XRGAME_EXPORTS
/**/

////////////////////////////////////////////////////////////////////////////
// CSE_PHSkeleton
////////////////////////////////////////////////////////////////////////////
CSE_PHSkeleton::CSE_PHSkeleton(LPCSTR caSection)
{
	source_id					= u16(-1);
	_flags.zero					();
}

CSE_PHSkeleton::~CSE_PHSkeleton()
{

}

void CSE_PHSkeleton::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	CSE_Visual				*visual = smart_cast<CSE_Visual*>(this);
	R_ASSERT				(visual);
	tNetPacket.r_stringZ	(visual->startup_animation);
	tNetPacket.r_u8			(_flags.flags);
	tNetPacket.r_u16		(source_id);
	if (_flags.test(flSavedData)) {
		data_load(tNetPacket);
	}
}

void CSE_PHSkeleton::STATE_Write		(NET_Packet	&tNetPacket)
{
	CSE_Visual				*visual = smart_cast<CSE_Visual*>(this);
	R_ASSERT				(visual);
	tNetPacket.w_stringZ	(visual->startup_animation);
	tNetPacket.w_u8			(_flags.flags);
	tNetPacket.w_u16		(source_id);
	////////////////////////saving///////////////////////////////////////
	if(_flags.test(flSavedData))
	{
		data_save(tNetPacket);
	}
}

void CSE_PHSkeleton::data_load(NET_Packet &tNetPacket)
{
	saved_bones.net_Load(tNetPacket);
	_flags.set(flSavedData,TRUE);
}

void CSE_PHSkeleton::data_save(NET_Packet &tNetPacket)
{
	saved_bones.net_Save(tNetPacket);
//	this comment is added by Dima (correct me if this is wrong)
//  if we call 2 times in a row StateWrite then we get different results
//	WHY???
//	_flags.set(flSavedData,FALSE);
}

void CSE_PHSkeleton::load(NET_Packet &tNetPacket)
{
	_flags.assign				(tNetPacket.r_u8());
	data_load					(tNetPacket);
	source_id					=u16(-1);//.
}
void CSE_PHSkeleton::UPDATE_Write(NET_Packet &tNetPacket)
{

}

void CSE_PHSkeleton::UPDATE_Read(NET_Packet &tNetPacket)
{

}

#ifndef XRGAME_EXPORTS
void CSE_PHSkeleton::FillProps				(LPCSTR pref, PropItemVec& values)
{
}
#endif // #ifndef XRGAME_EXPORTS

CSE_AbstractVisual::CSE_AbstractVisual(LPCSTR section):inherited1(section),inherited2(section)
{
}

CSE_AbstractVisual::~CSE_AbstractVisual()
{
}

void CSE_AbstractVisual::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	visual_read						(tNetPacket,m_wVersion);
	tNetPacket.r_stringZ			(startup_animation);
}

void CSE_AbstractVisual::STATE_Write	(NET_Packet	&tNetPacket)
{
	visual_write				(tNetPacket);
	tNetPacket.w_stringZ		(startup_animation);
}

#ifndef XRGAME_EXPORTS
void CSE_AbstractVisual::FillProps		(LPCSTR pref, PropItemVec& values)
{
	inherited1::FillProps			(pref,values);
	inherited2::FillProps			(pref,values);
}
#endif // #ifndef XRGAME_EXPORTS

void CSE_AbstractVisual::UPDATE_Read	(NET_Packet	&tNetPacket)
{
}

void CSE_AbstractVisual::UPDATE_Write	(NET_Packet	&tNetPacket)
{
}
LPCSTR	CSE_AbstractVisual::getStartupAnimation		()
{
	return *startup_animation;
}

CSE_Visual* CSE_AbstractVisual::visual					()
{
	return this;
}
