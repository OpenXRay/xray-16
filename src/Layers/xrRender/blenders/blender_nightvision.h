#pragma once

namespace xray::render::RENDER_NAMESPACE
{
class CBlender_nightvision : public IBlender
{
public:
    CBlender_nightvision();
    ~CBlender_nightvision() override = default;

    LPCSTR getComment() override;
    void Compile(CBlender_Compile& C) override;
};
} // namespace xray::render::RENDER_NAMESPACE
