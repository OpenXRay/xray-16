#pragma once

namespace xray::render::RENDER_NAMESPACE
{
class CBlender_Blur : public IBlender
{
public:
    CBlender_Blur();
    ~CBlender_Blur() override = default;

    LPCSTR getComment() override;
    void Compile(CBlender_Compile& C) override;
};
} // namespace xray::render::RENDER_NAMESPACE
