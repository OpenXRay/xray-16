#pragma once

class CBlender_ShTex : public IBlender
{
public:
    CBlender_ShTex();

    LPCSTR getComment() override;
    BOOL canBeLMAPped() override;

    void Compile (CBlender_Compile& C) override;
};
