#pragma once

class CBlender_Model_EbB : public IBlender
{
    string64 oT2_Name; // name of secondary texture
    string64 oT2_xform; // xform for secondary texture
    xrP_BOOL oBlend;

private:
    void CompileForEditor(CBlender_Compile& C);

public:
    CBlender_Model_EbB();
    ~CBlender_Model_EbB() override = default;

    LPCSTR getComment() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;

    void Compile(CBlender_Compile& C) override;
};
