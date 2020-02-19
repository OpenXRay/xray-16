#pragma once

class CBlender_deffer_model : public IBlender
{
public:
    xrP_Integer oAREF;
    xrP_BOOL oBlend;

public:
    virtual const char* getComment() { return "LEVEL: deffer-model-flat"; }
    virtual bool canBeDetailed() { return TRUE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Save(IWriter& fs);
    virtual void Load(IReader& fs, u16 version);
    virtual void Compile(CBlender_Compile& C);

    CBlender_deffer_model();
    virtual ~CBlender_deffer_model();

private:
    xrP_TOKEN oTessellation;
};
