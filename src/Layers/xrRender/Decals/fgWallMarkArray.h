#pragma once

#include "Include/xrRender/WallMarkArray.h"

namespace xray::render::fg::decals {

class fgWallMarkArray : public IWallMarkArray
{
public:
    ~fgWallMarkArray() override = default;
    void Copy(IWallMarkArray& _in) override;
    void AppendMark(LPCSTR s_textures) override;
    void clear() override;
    bool empty() override;
    wm_shader GenerateWallmark() override;

    u32 GenerateBindlessMaterialID(shared_str* outTextureName = nullptr);

private:
    u32 TryRegisterMaterial(u32 index);

    xr_vector<u32> m_materialIDs;
    xr_vector<shared_str> m_textureNames;
};

} // namespace xray::render::fg::decals
