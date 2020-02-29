#pragma once

class CBlender_luminance : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: luminance estimate"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_luminance();
    virtual ~CBlender_luminance();
};
