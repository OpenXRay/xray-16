#pragma once

namespace xray::render::RENDER_NAMESPACE
{
class CBlender_dof : public IBlender
{
public:
    CBlender_dof();
    ~CBlender_dof() override = default;

    LPCSTR getComment() override;
    void Compile(CBlender_Compile& C) override;
};
} // namespace xray::render::RENDER_NAMESPACE
