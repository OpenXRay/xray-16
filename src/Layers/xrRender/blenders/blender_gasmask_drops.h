#pragma once

class CBlender_gasmask_drops : public IBlender
{
public:
    CBlender_gasmask_drops();
    ~CBlender_gasmask_drops() override = default;

    LPCSTR getComment() override;
    void Compile(CBlender_Compile& C) override;
};
