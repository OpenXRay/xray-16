#pragma once

class CBlender_Blur : public IBlender
{
public:
    CBlender_Blur();
    ~CBlender_Blur() override = default;

    LPCSTR getComment() override;
    BOOL canBeLMAPped() override;
    void Compile(CBlender_Compile& C) override;
};
