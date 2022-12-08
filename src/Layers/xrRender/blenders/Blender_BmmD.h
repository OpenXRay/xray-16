#pragma once

class CBlender_BmmD : public IBlender
{
    string64 oT2_Name; // name of secondary texture
    string64 oT2_xform; // xform for secondary texture
    string64 oR_Name; //. задел на будущее
    string64 oG_Name; //. задел на будущее
    string64 oB_Name; //. задел на будущее
    string64 oA_Name; //. задел на будущее

private:
    void CompileForEditor(CBlender_Compile& C);

public:
    LPCSTR getComment() override;
    BOOL canBeDetailed() override;
    BOOL canBeLMAPped() override;
    BOOL canUseSteepParallax() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;

    void Compile(CBlender_Compile& C) override;

    CBlender_BmmD();
    ~CBlender_BmmD() override = default;
};
