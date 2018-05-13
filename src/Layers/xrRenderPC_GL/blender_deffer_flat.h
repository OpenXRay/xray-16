#pragma once

class CBlender_deffer_flat : public IBlender
{
public:
    LPCSTR getComment() override { return "LEVEL: defer-base-normal"; }
    BOOL canBeDetailed() override { return TRUE; }
    BOOL canBeLMAPped() override { return FALSE; }
    BOOL canUseSteepParallax() override { return TRUE; }

    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
    void Compile(CBlender_Compile& C) override;

    CBlender_deffer_flat();
    virtual ~CBlender_deffer_flat();

private:
    xrP_TOKEN oTessellation;
};
