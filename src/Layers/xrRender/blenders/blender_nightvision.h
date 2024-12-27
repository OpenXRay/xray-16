#pragma once

class CBlender_nightvision : public IBlender
{
public:
    CBlender_nightvision();
    ~CBlender_nightvision() override = default;

    LPCSTR getComment() override;
    void Compile(CBlender_Compile& C) override;
};
