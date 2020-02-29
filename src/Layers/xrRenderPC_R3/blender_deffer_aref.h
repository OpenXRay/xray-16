#pragma once

class CBlender_deffer_aref : public IBlender
{
public:
    xrP_Integer oAREF;
    xrP_BOOL oBlend;
    bool lmapped;

public:
    virtual const char* getComment() { return "LEVEL: defer-base-aref"; }
    virtual bool canBeDetailed() { return TRUE; }
    virtual bool canBeLMAPped() { return lmapped; }
    virtual bool canUseSteepParallax() { return TRUE; }
    virtual void Save(IWriter& fs);
    virtual void Load(IReader& fs, u16 version);
    virtual void Compile(CBlender_Compile& C);

    CBlender_deffer_aref(bool _lmapped = false);
    virtual ~CBlender_deffer_aref();
};
