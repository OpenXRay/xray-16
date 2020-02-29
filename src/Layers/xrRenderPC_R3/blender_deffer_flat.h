#pragma once

class CBlender_deffer_flat : public IBlender
{
public:
    virtual const char* getComment() { return "LEVEL: defer-base-normal"; }
    virtual bool canBeDetailed() { return TRUE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual bool canUseSteepParallax() { return TRUE; }
    virtual void Save(IWriter& fs);
    virtual void Load(IReader& fs, u16 version);
    virtual void Compile(CBlender_Compile& C);

    CBlender_deffer_flat();
    virtual ~CBlender_deffer_flat();

private:
    xrP_TOKEN oTessellation;
};
