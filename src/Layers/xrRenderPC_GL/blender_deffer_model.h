#pragma once

class CBlender_deffer_model : public IBlender
{
public:
    xrP_Integer oAREF;
    xrP_BOOL oBlend;
public:
    LPCSTR getComment() override { return "LEVEL: deffer-model-flat"; }
    BOOL canBeDetailed() override { return TRUE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
    void Compile(CBlender_Compile& C) override;

    CBlender_deffer_model();
    virtual ~CBlender_deffer_model();

private:
    xrP_TOKEN oTessellation;
};
