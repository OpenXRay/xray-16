#pragma once

namespace xray::render::RENDER_NAMESPACE
{
class CBlender_Screen_GRAY : public IBlender
{
public:
    CBlender_Screen_GRAY();
    ~CBlender_Screen_GRAY() override = default;

    LPCSTR getComment() override;

    void Compile(CBlender_Compile& C) override;
};
} // namespace xray::render::RENDER_NAMESPACE
