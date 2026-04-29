#pragma once

namespace xray::render::fg
{
class CBlender_Detail_GPU : public IBlender
{
public:
    CBlender_Detail_GPU();
    ~CBlender_Detail_GPU() override = default;

    LPCSTR getComment() override;
    void Compile(CBlender_Compile& C) override;
};
} // namespace xray::render::fg
