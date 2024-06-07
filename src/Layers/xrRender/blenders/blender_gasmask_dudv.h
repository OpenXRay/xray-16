#pragma once

class CBlender_gasmask_dudv : public IBlender
{
public:
    CBlender_gasmask_dudv();
    ~CBlender_gasmask_dudv() override = default;

    LPCSTR getComment() override;
    void Compile(CBlender_Compile& C) override;
};
