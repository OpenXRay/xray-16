#pragma once

class CBlender_deffer_flat : public IBlender
{
public:
    const char* getComment() override { return "LEVEL: defer-base-normal"; }
    bool canBeDetailed() override { return TRUE; }
    bool canBeLMAPped() override { return FALSE; }
    bool canUseSteepParallax() override { return TRUE; }

    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
    void Compile(CBlender_Compile& C) override;

    CBlender_deffer_flat();
    virtual ~CBlender_deffer_flat();

private:
    xrP_TOKEN oTessellation;
};
