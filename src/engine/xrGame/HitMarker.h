#ifndef __XR_HITMARKER_H__
#define __XR_HITMARKER_H__
#pragma once

#include "../Include/xrRender/FactoryPtr.h"
class IUIShader;
#include "ui_defs.h"

class CUIStaticItem;
class CLAItem;
class CGrenade;

struct SHitMark
{
	CUIStaticItem*	m_UIStaticItem;
	float			m_StartTime;
	float			m_HitDirection;
	CLAItem*		m_lanim;

					SHitMark		( const ui_shader& sh, const Fvector& dir );
					~SHitMark		();
	bool			IsActive		();
	void			UpdateAnim		();
	void			Draw			( float dir );
};

struct SGrenadeMark
{
	CGrenade*		p_grenade;
	bool			removed_grenade;

	CUIStaticItem*	m_UIStaticItem;
	float			m_LastTime;
	float			m_Angle;
	CLAItem*		m_LightAnim;

					SGrenadeMark( const ui_shader& sh, CGrenade* grn );
					~SGrenadeMark();

	bool			IsActive() const;
	void			Draw( float cam_dir );
	void			Update( float angle );

};

class CHitMarker
{
public:
	FactoryPtr<IUIShader>	hShader2;
	FactoryPtr<IUIShader>	hShader_Grenade;

	typedef xr_deque<SHitMark*>			HITMARKS;
	typedef xr_deque<SGrenadeMark*>		GRENADEMARKS;
	
	HITMARKS				m_HitMarks;
	GRENADEMARKS			m_GrenadeMarks;

public:
							CHitMarker	();
							~CHitMarker	();

	void					Render		();
	void					Hit			( const Fvector& dir );
	bool					AddGrenade_ForMark( CGrenade* grn );
	void					Update_GrenadeView( Fvector& pos_actor );

	void					InitShader( LPCSTR tex_name );
	void					InitShader_Grenade( LPCSTR tex_name );

	void					net_Relcase( CObject* obj );
};

#endif // __XR_HITMARKER_H__
