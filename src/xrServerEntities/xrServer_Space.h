////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects space
////////////////////////////////////////////////////////////////////////////

#ifndef xrServer_SpaceH
#define xrServer_SpaceH

#include "script_export_space.h"

#ifndef XRGAME_EXPORTS
#	define SERVER_ENTITY_EDITOR_METHODS					virtual void FillProps(LPCSTR pref, PropItemVec& values);
#else // #ifdef XRGAME_EXPORTS
#	define SERVER_ENTITY_EDITOR_METHODS
#endif // #ifndef XRGAME_EXPORTS

#define SERVER_ENTITY_SCRIPT_METHODS					DECLARE_SCRIPT_REGISTER_FUNCTION
#define SERVER_ENTITY_DECLARE_BEGIN0(__A)				class __A	{ public: SERVER_ENTITY_SCRIPT_METHODS
#define SERVER_ENTITY_DECLARE_BEGIN(__A,__B)			class __A : public __B	{ typedef __B inherited; public: SERVER_ENTITY_SCRIPT_METHODS
#define SERVER_ENTITY_DECLARE_BEGIN2(__A,__B,__C)		class __A : public __B, public __C	{ typedef __B inherited1; typedef __C inherited2; public: SERVER_ENTITY_SCRIPT_METHODS
#define SERVER_ENTITY_DECLARE_BEGIN3(__A,__B,__C,__D)	class __A : public __B, public __C, public __D	{ typedef __B inherited1; typedef __C inherited2; typedef __D inherited3; public: SERVER_ENTITY_SCRIPT_METHODS

#define	SERVER_ENTITY_DECLARE_END \
public:\
	virtual void 			UPDATE_Read		(NET_Packet& P); \
	virtual void 			UPDATE_Write	(NET_Packet& P); \
	virtual void 			STATE_Read		(NET_Packet& P, u16 size); \
	virtual void 			STATE_Write		(NET_Packet& P); \
	SERVER_ENTITY_EDITOR_METHODS \
};

struct	SRotation
{
	float  yaw, pitch, roll;
	SRotation() { yaw=pitch=roll=0; }
	SRotation(float y, float p, float r) { yaw=y;pitch=p;roll=r; }
};

enum EPOType {
	epotBox,
	epotFixedChain,
    epotFreeChain,
    epotSkeleton
};

DEFINE_VECTOR	(u32,						DWORD_VECTOR,					DWORD_IT);
DEFINE_VECTOR	(bool,						BOOL_VECTOR,					BOOL_IT);
DEFINE_VECTOR	(float,						FLOAT_VECTOR,					FLOAT_IT);
DEFINE_VECTOR	(LPSTR,						LPSTR_VECTOR,					LPSTR_IT);
DEFINE_VECTOR	(Fvector,					FVECTOR_VECTOR,					FVECTOR_IT);

#ifdef XRGAME_EXPORTS
#	define DECLARE_ENTITY_DESTROY
#endif

#ifdef XRSE_FACTORY_EXPORTS
#	define DECLARE_ENTITY_DESTROY
#endif

#ifdef DECLARE_ENTITY_DESTROY
template <class T> void	F_entity_Destroy	(T *&P)
{
	xr_delete	(P);
};
#endif

#endif