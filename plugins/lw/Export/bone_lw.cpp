#include "stdafx.h"
#include "..\..\Shared\bone.h"
#include "..\..\Shared\envelope.h"

#ifdef _LW_EXPORT
extern "C"	LWItemInfo		*g_iteminfo;
extern "C"	LWChannelInfo	*g_chinfo;
extern "C"	LWBoneInfo		*g_boneinfo;
extern "C"	LWEnvelopeFuncs	*g_envf;

void CBone::ParseBone(LWItemID bone){
	LWItemID P = g_iteminfo->parent(bone);
	if (g_iteminfo->type(P)==LWI_BONE) SetParentName(g_iteminfo->name(P));

	LWDVector vec;
	g_boneinfo->restParam(bone,LWIP_POSITION,vec);
	rest_offset.set((float)vec[0],(float)vec[1],(float)vec[2]);
	g_boneinfo->restParam(bone,LWIP_ROTATION,vec);
	rest_rotate.set((float)vec[1],(float)vec[0],(float)vec[2]);

	rest_length = (float)g_boneinfo->restLength(bone);

	SetWMap(g_boneinfo->weightMap(bone));
}
#endif
