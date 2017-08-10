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

using DWORD_VECTOR = xr_vector<u32>;
using BOOL_VECTOR = xr_vector<bool>;
using FLOAT_VECTOR = xr_vector<float>;
using LPSTR_VECTOR = xr_vector<LPSTR>;
using FVECTOR_VECTOR = xr_vector<Fvector>;

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
