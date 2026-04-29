#pragma once

namespace xray::render::RENDER_NAMESPACE
{
class CBlender_light_occq : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: occlusion testing"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_light_occq();
    virtual ~CBlender_light_occq();
};
} // namespace xray::render::RENDER_NAMESPACE
