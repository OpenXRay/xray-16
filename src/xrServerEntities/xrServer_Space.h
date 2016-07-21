////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects space
////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef xrServer_SpaceH
#define xrServer_SpaceH
#include "xrCore/_types.h"
#include "xrCore/_vector3d.h"
#include "xrCommon/xr_vector.h"

#ifndef XRGAME_EXPORTS
#define SERVER_ENTITY_EDITOR_METHODS virtual void FillProps(LPCSTR pref, PropItemVec& values);
#else // #ifdef XRGAME_EXPORTS
#define SERVER_ENTITY_EDITOR_METHODS
#endif // #ifndef XRGAME_EXPORTS

enum EPOType
{
    epotBox,
    epotFixedChain,
    epotFreeChain,
    epotSkeleton
};

DEFINE_VECTOR(u32, DWORD_VECTOR, DWORD_IT);
DEFINE_VECTOR(bool, BOOL_VECTOR, BOOL_IT);
DEFINE_VECTOR(float, FLOAT_VECTOR, FLOAT_IT);
DEFINE_VECTOR(LPSTR, LPSTR_VECTOR, LPSTR_IT);
DEFINE_VECTOR(Fvector, FVECTOR_VECTOR, FVECTOR_IT);

#ifdef XRGAME_EXPORTS
#define DECLARE_ENTITY_DESTROY
#endif

#ifdef XRSE_FACTORY_EXPORTS
#define DECLARE_ENTITY_DESTROY
#endif

#ifdef DECLARE_ENTITY_DESTROY
template <class T>
void F_entity_Destroy(T*& P)
{
    xr_delete(P);
};
#endif

#endif
