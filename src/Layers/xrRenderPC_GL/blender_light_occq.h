#pragma once

class CBlender_light_occq : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: occlusion testing"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_light_occq();
    virtual ~CBlender_light_occq();
};
