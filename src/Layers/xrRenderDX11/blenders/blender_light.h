#pragma once

namespace xray::render::RENDER_NAMESPACE
{
class CBlender_LIGHT : public IBlender
{
public:
    CBlender_LIGHT();

    LPCSTR getComment() override;
    BOOL canBeLMAPped() override;

    void Compile(CBlender_Compile& C) override;
};
} // namespace xray::render::RENDER_NAMESPACE
