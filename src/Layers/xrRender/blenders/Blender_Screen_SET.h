#pragma once

class CBlender_Screen_SET : public IBlender
{
    xrP_TOKEN oBlend;
    xrP_Integer oAREF;
    xrP_BOOL oZTest;
    xrP_BOOL oZWrite;
    xrP_BOOL oLighting;
    xrP_BOOL oFog;
    xrP_BOOL oClamp;

private:
    void CompileFixed(CBlender_Compile& C);
    void CompileProgrammed(CBlender_Compile& C);

public:
    CBlender_Screen_SET();
    ~CBlender_Screen_SET() override = default;

    LPCSTR getComment() override;

    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;

    void Compile(CBlender_Compile& C) override;
};
