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

#ifndef MASTER_GOLD
#define SERVER_ENTITY_EDITOR_METHODS virtual void FillProps(LPCSTR pref, PropItemVec& values);
#else
#define SERVER_ENTITY_EDITOR_METHODS
#endif

enum EPOType
{
    epotBox,
    epotFixedChain,
    epotFreeChain,
    epotSkeleton
};

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
