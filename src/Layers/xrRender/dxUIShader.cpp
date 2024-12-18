#include "stdafx.h"
#include "dxUIShader.h"

xr_unordered_map<std::string, ref_shader> g_UIShadersCache;

static ref_shader& GetCachedUIShader(const char* sh, const char* tex)
{
    std::string key{ tex ? tex : "" };
    key += "_";
    key += sh;

    if (const auto it = g_UIShadersCache.find(key); it != g_UIShadersCache.end())
    {
        return it->second;
    }
    else
    {
        auto& shader = g_UIShadersCache[key];
        shader.create(sh, tex);
        return shader;
    }
}

void dxUIShader::Copy(IUIShader& _in) { *this = *((dxUIShader*)&_in); }
void dxUIShader::create(LPCSTR sh, LPCSTR tex, bool no_cache)
{
    if (no_cache)
    {
        hShader.create(sh, tex);
    }
    else
    {
        hShader = GetCachedUIShader(sh, tex);
    }
}
//void dxUIShader::destroy() { hShader.destroy(); }
