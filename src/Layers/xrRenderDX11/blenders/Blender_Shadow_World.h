#pragma once

namespace xray::render::RENDER_NAMESPACE
{
class CBlender_ShWorld : public IBlender
{
public:
    CBlender_ShWorld();
    ~CBlender_ShWorld() override = default;

    LPCSTR getComment() override;

    void Compile(CBlender_Compile& C) override;
};
} // namespace xray::render::RENDER_NAMESPACE
