#pragma once

class CBlender_light_occq : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: occlusion testing"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_light_occq();
    virtual ~CBlender_light_occq();
};
