#include "stdafx.h"
#include "ColorMapManager.h"

ColorMapManager::ColorMapManager()
{
    m_CMap[0] = RImplementation.Resources->_CreateTexture("$user$cmap0");
    m_CMap[1] = RImplementation.Resources->_CreateTexture("$user$cmap1");
}

void ColorMapManager::SetTextures(const shared_str& tex0, const shared_str& tex1)
{
    UpdateTexture(tex0, 0);
    UpdateTexture(tex1, 1);
}

void ColorMapManager::UpdateTexture(const shared_str& strTexName, int iTex)
{
    if (strTexName == m_strCMap[iTex])
        return;

    m_strCMap[iTex] = strTexName;

    if (strTexName.size())
    {
        auto I = m_TexCache.find(strTexName);
        if (I != m_TexCache.end())
        {

#if defined(USE_DX9) || defined(USE_DX11) || defined(USE_DX12)
            ID3DBaseTexture* e0 = I->second->surface_get();
            m_CMap[iTex]->surface_set(e0);
            _RELEASE(e0);
#elif defined(USE_OGL)
            GLuint e0 = I->second->surface_get();
            m_CMap[iTex]->surface_set(GL_TEXTURE_2D, e0);
#else
#    error No graphics API selected or in use!
#endif
        }
        else
        {
            ref_texture tmp;
            tmp.create(strTexName.c_str());

            m_TexCache.emplace(strTexName, tmp);

#if defined(USE_DX9) || defined(USE_DX11) || defined(USE_DX12)
            ID3DBaseTexture* e0 = tmp->surface_get();
            m_CMap[iTex]->surface_set(e0);
            _RELEASE(e0);
#elif defined(USE_OGL)
            GLuint e0 = tmp->surface_get();
            m_CMap[iTex]->surface_set(GL_TEXTURE_2D, e0);
#else
#    error No graphics API selected or in use!
#endif
        }
    }
    else
    {
#if defined(USE_DX9) || defined(USE_DX11) || defined(USE_DX12)
        m_CMap[iTex]->surface_set(nullptr);
#elif defined(USE_OGL)
        m_CMap[iTex]->surface_set(GL_TEXTURE_2D, 0);
#else
#    error No graphics API selected or in use!
#endif
    }
}
