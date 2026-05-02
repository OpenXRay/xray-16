#include "stdafx.h"
#include "ColorMapManager.h"

namespace xray::render::fg
{
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
        ref_texture src;
        auto I = m_TexCache.find(strTexName);
        if (I != m_TexCache.end())
        {
            src = I->second;
        }
        else
        {
            src.create(strTexName.c_str());
            m_TexCache.emplace(strTexName, src);
        }
        if (!src->flags.bLoaded)
            src->Load();
        m_CMap[iTex]->surface_set(nvrhi::TextureHandle(src->surface_get_native()));
    }
    else
    {
        m_CMap[iTex]->surface_set(nvrhi::TextureHandle());
    }
}
} // namespace xray::render::fg
