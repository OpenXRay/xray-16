#pragma once

class CBlender_LaEmB : public IBlender
{
    string64 oT2_Name;  // name of secondary texture
    string64 oT2_xform; // xform for secondary texture
    string64 oT2_const;

private:
    void compile_L(CBlender_Compile& C);
    void compile_Lc(CBlender_Compile& C);
    void compile_2(CBlender_Compile& C);
    void compile_2c(CBlender_Compile& C);
    void compile_3(CBlender_Compile& C);
    void compile_3c(CBlender_Compile& C);
    void compile_ED(CBlender_Compile& C);
    void compile_EDc(CBlender_Compile& C);

public:
    CBlender_LaEmB();
    ~CBlender_LaEmB() override = default;

    LPCSTR getComment() override;
    BOOL canBeLMAPped() override;
    
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version)  override;
    
    void Compile(CBlender_Compile& C) override;
};
