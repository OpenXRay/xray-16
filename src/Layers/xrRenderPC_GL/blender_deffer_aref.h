#pragma once

class CBlender_deffer_aref : public IBlender
{
public:
    xrP_Integer oAREF;
    xrP_BOOL oBlend;
    bool lmapped;
public:
    LPCSTR getComment() override { return "LEVEL: defer-base-aref"; }
    BOOL canBeDetailed() override { return TRUE; }
    BOOL canBeLMAPped() override { return lmapped; }
    BOOL canUseSteepParallax() override { return TRUE; }

    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
    void Compile(CBlender_Compile& C) override;

    CBlender_deffer_aref(bool _lmapped = false);
    virtual ~CBlender_deffer_aref();
};
