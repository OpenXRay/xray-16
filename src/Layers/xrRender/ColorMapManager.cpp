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
#ifdef USE_OGL
            GLuint e0 = I->second->surface_get();
            m_CMap[iTex]->surface_set(GL_TEXTURE_2D, e0);
#else
            ID3DBaseTexture* e0 = I->second->surface_get();
            m_CMap[iTex]->surface_set(e0);
            _RELEASE(e0);
#endif // USE_OGL
        }
        else
        {
            ref_texture tmp;
            tmp.create(strTexName.c_str());

            m_TexCache.insert(std::make_pair(strTexName, tmp));

#ifdef USE_OGL
            GLuint e0 = tmp->surface_get();
            m_CMap[iTex]->surface_set(GL_TEXTURE_2D, e0);
#else
            ID3DBaseTexture* e0 = tmp->surface_get();
            m_CMap[iTex]->surface_set(e0);
            _RELEASE(e0);
#endif // USE_OGL
        }
    }
    else
    {
#ifdef USE_OGL
        m_CMap[iTex]->surface_set(GL_TEXTURE_2D, 0);
#else
        m_CMap[iTex]->surface_set(nullptr);
#endif // USE_OGL
    }
}
