#ifndef	dx103DFluidData_included
#define	dx103DFluidData_included
#pragma once

#include "dx103DFluidEmitters.h"

class dx103DFluidData
{
public:
	enum eVolumePrivateRT
	{
		VP_VELOCITY0 = 0,
		VP_PRESSURE,
		VP_COLOR,	//	Swap with global after update
		VP_NUM_TARGETS
	};

	enum SimulationType
	{
		ST_FOG = 0,
		ST_FIRE,
	};

	struct Settings
	{
		float			m_fHemi;
		float			m_fConfinementScale;
		float			m_fDecay;
		float			m_fGravityBuoyancy;
		SimulationType	m_SimulationType;
	};
public:
	dx103DFluidData();
	~dx103DFluidData();

	void	Load(IReader *data);

	void	SetTexture(eVolumePrivateRT id, ID3DTexture3D *pT) { pT->AddRef(); m_pRTTextures[id]->Release(); m_pRTTextures[id] = pT;}
	void	SetView(eVolumePrivateRT id, ID3DRenderTargetView *pV) { pV->AddRef(); m_pRenderTargetViews[id]->Release(); m_pRenderTargetViews[id] = pV;}

	ID3DTexture3D*			GetTexture(eVolumePrivateRT id) const { m_pRTTextures[id]->AddRef(); return m_pRTTextures[id];}
	ID3DRenderTargetView*		GetView(eVolumePrivateRT id) const { m_pRenderTargetViews[id]->AddRef(); return m_pRenderTargetViews[id];}
	const Fmatrix&				GetTransform() const { return m_Transform;}
	const xr_vector<Fmatrix>&	GetObstaclesList() const { return m_Obstacles;}
	const xr_vector<dx103DFluidEmitters::CEmitter>& GetEmittersList() const { return m_Emitters;}
	const Settings&				GetSettings() const { return m_Settings;}

	//	Allow real-time config reload
#ifdef	DEBUG
	void	ReparseProfile(const xr_string &Profile);
#endif	//	DEBUG

private:
	typedef	dx103DFluidEmitters::CEmitter	CEmitter;

private:

	void	CreateRTTextureAndViews(int rtIndex, D3D_TEXTURE3D_DESC TexDesc);
	void	DestroyRTTextureAndViews(int rtIndex);

	void	ParseProfile(const xr_string &Profile);

private:
	Fmatrix					m_Transform;

	xr_vector<Fmatrix>		m_Obstacles;
	xr_vector<CEmitter>		m_Emitters;

	Settings				m_Settings;

	static	DXGI_FORMAT		m_VPRenderTargetFormats[ VP_NUM_TARGETS ];

	ID3DRenderTargetView	*m_pRenderTargetViews[ VP_NUM_TARGETS ];
	ID3DTexture3D			*m_pRTTextures[ VP_NUM_TARGETS ];
};

#endif	//	dx103DFluidData_included