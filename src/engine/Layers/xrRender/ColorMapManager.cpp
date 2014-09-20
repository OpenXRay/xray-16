#include "stdafx.h"
#include "ColorMapManager.h"

#include "dxRenderDeviceRender.h"


ColorMapManager::ColorMapManager()
{
	m_CMap[0]	= dxRenderDeviceRender::Instance().Resources->_CreateTexture("$user$cmap0");
	m_CMap[1]	= dxRenderDeviceRender::Instance().Resources->_CreateTexture("$user$cmap1");
}

void ColorMapManager::SetTextures(const shared_str &tex0, const shared_str &tex1)
{ 
	UpdateTexture(tex0, 0); 
	UpdateTexture(tex1, 1);
}


void ColorMapManager::UpdateTexture(const shared_str &strTexName, int iTex)
{
	if ( strTexName==m_strCMap[iTex] ) return;

	m_strCMap[iTex] = strTexName;

	if (strTexName.size())
	{
		map_TexIt I = m_TexCache.find(strTexName);
		if (I!=m_TexCache.end())
		{
			ID3DBaseTexture*	e0	= I->second->surface_get();
			m_CMap[iTex]->surface_set(e0);
			_RELEASE(e0);
		}
		else
		{
			ref_texture	tmp;
			tmp.create(strTexName.c_str());

			m_TexCache.insert	(mk_pair(strTexName,tmp));

			ID3DBaseTexture*	e0	= tmp->surface_get();
			m_CMap[iTex]->surface_set(e0);
			_RELEASE(e0);
		}
	}
	else
	{
		m_CMap[iTex]->surface_set(0);
	}

	
}