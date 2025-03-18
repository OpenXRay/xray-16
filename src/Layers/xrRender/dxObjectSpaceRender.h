#pragma once

#ifdef DEBUG
#include "Include/xrRender/ObjectSpaceRender.h"
#include "xrEngine/xr_collide_form.h"

namespace xray::render::RENDER_NAMESPACE
{
class dxObjectSpaceRender : public IObjectSpaceRender
{
public:
    dxObjectSpaceRender();
    virtual ~dxObjectSpaceRender();
    virtual void Copy(IObjectSpaceRender& _in);

    virtual void dbgRender();
    virtual void dbgAddSphere(const Fsphere& sphere, u32 colour);
    virtual void dbgReserveSphere(size_t count);
    virtual void SetShader();

private:
    ref_shader m_shDebug;
    clQueryCollision q_debug; // MT: dangerous
    xr_vector<std::pair<Fsphere, u32>> dbg_S; // MT: dangerous
};

} // namespace xray::render::RENDER_NAMESPACE
#endif // DEBUG
