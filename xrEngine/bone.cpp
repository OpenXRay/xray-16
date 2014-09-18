#include "stdafx.h"
#pragma hdrstop

#include "bone.h"
#include "gamemtllib.h"


u16	CBone::get_game_mtl_idx	( )			const	
{
	return GMLib.GetMaterialIdx(game_mtl.c_str());
}

static const Fobb	dummy = Fobb().identity();
const	Fobb&		CBone::	get_obb				( )			const
{
	return dummy;
}
//////////////////////////////////////////////////////////////////////////
// BoneInstance methods



void	ENGINE_API	CBoneInstance::set_param	(u32 idx, float data)
{
	VERIFY		(idx<MAX_BONE_PARAMS);
	param[idx]	= data;
}
float	ENGINE_API	CBoneInstance::get_param	(u32 idx)
{
	VERIFY		(idx<MAX_BONE_PARAMS);
	return		param[idx];
}

#ifdef	DEBUG
void ENGINE_API	CBoneData::DebugQuery		(BoneDebug& L)
{
	for (u32 i=0; i<children.size(); i++)
	{
		L.push_back(SelfID);
		L.push_back(children[i]->SelfID);
		children[i]->DebugQuery(L);
	}
}
#endif

void ENGINE_API	CBoneData::CalculateM2B(const Fmatrix& parent)
{
	// Build matrix
	m2b_transform.mul_43	(parent,bind_transform);

	// Calculate children
	for (xr_vector<CBoneData*>::iterator C=children.begin(); C!=children.end(); C++)
		(*C)->CalculateM2B	(m2b_transform);

	m2b_transform.invert	();            
}

u16	CBoneData::GetNumChildren	( )const
{
	return (u16)children.size();
}
IBoneData&	CBoneData::GetChild		( u16 id )
{
	return *children[id];
}
const IBoneData&	CBoneData::GetChild		( u16 id )const
{
	return *children[id];
}
