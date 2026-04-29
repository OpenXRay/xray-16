#pragma once

namespace xray::render::fg
{
class CBlender_msaa : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: DX11 msaa blender"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};
} // namespace xray::render::fg
