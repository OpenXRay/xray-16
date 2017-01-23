#ifndef	dx10ShaderResourceStateCache_included
#define	dx10ShaderResourceStateCache_included
#pragma once

class dx10ShaderResourceStateCache
{
public:
	dx10ShaderResourceStateCache();

	void	ResetDeviceState();

	void	Apply();

	void	SetPSResource( u32 uiSlot, ID3DShaderResourceView	*pRes );
	void	SetGSResource( u32 uiSlot, ID3DShaderResourceView	*pRes );
	void	SetVSResource( u32 uiSlot, ID3DShaderResourceView	*pRes );
#ifdef USE_DX11
	void	SetDSResource( u32 uiSlot, ID3DShaderResourceView	*pRes );
	void	SetHSResource( u32 uiSlot, ID3DShaderResourceView	*pRes );
	void	SetCSResource( u32 uiSlot, ID3DShaderResourceView	*pRes );
#endif

private:
	ID3DShaderResourceView	*m_PSViews[CBackend::mtMaxPixelShaderTextures];
	ID3DShaderResourceView	*m_GSViews[CBackend::mtMaxGeometryShaderTextures];
	ID3DShaderResourceView	*m_VSViews[CBackend::mtMaxVertexShaderTextures];
#ifdef USE_DX11
	ID3DShaderResourceView	*m_HSViews[CBackend::mtMaxHullShaderTextures];
	ID3DShaderResourceView	*m_DSViews[CBackend::mtMaxDomainShaderTextures];
	ID3DShaderResourceView	*m_CSViews[CBackend::mtMaxComputeShaderTextures];
#endif

	u32		m_uiMinPSView;
	u32		m_uiMaxPSView;

	u32		m_uiMinGSView;
	u32		m_uiMaxGSView;

	u32		m_uiMinVSView;
	u32		m_uiMaxVSView;

#ifdef USE_DX11
	u32		m_uiMinHSView;
	u32		m_uiMaxHSView;

	u32		m_uiMinDSView;
	u32		m_uiMaxDSView;

	u32		m_uiMinCSView;
	u32		m_uiMaxCSView;
#endif

	bool	m_bUpdatePSViews;
	bool	m_bUpdateGSViews;
	bool	m_bUpdateVSViews;
#ifdef USE_DX11
	bool	m_bUpdateHSViews;
	bool	m_bUpdateDSViews;
	bool	m_bUpdateCSViews;
#endif
};

extern	dx10ShaderResourceStateCache	SRVSManager;

#endif	//	dx10ShaderResourceStateCache_included