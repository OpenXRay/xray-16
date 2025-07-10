#pragma once

namespace xray::render::RENDER_NAMESPACE
{
class CBlender_lut : public IBlender
{
public:
    CBlender_lut();
    ~CBlender_lut() override = default;

    LPCSTR getComment() override;
    void Compile(CBlender_Compile& C) override;
};
}
