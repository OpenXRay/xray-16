#pragma once

class CBlender_luminance : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: luminance estimate"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_luminance();
    virtual ~CBlender_luminance();
};
