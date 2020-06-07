#pragma once


class CBlender_default : public IBlender
{
    xrP_TOKEN oTessellation;

private:
    void CompileForEditor(CBlender_Compile& C);

public:
    CBlender_default();
    ~CBlender_default() override = default;

    LPCSTR getComment() override;
    BOOL canBeDetailed() override;
    BOOL canBeLMAPped() override;

    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;

    void Compile(CBlender_Compile& C) override;
};
