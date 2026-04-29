#pragma once

namespace xray::render::fg
{
class CBlender_Model_EbB : public IBlender
{
public:
    string64 oT2_Name;
    string64 oT2_xform;
    xrP_BOOL oBlend;

private:
    void CompileFFP(CBlender_Compile& C) const;
    void CompileProgrammable(CBlender_Compile& C) const;

public:
    CBlender_Model_EbB();
    ~CBlender_Model_EbB() override = default;

    LPCSTR getComment() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;

    void Compile(CBlender_Compile& C) override;
};
}
