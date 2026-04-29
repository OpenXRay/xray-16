#pragma once

namespace xray::render::fg
{
class CBlender_createminmax : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: DX11 minmax sm blender"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};
} // namespace xray::render::fg
