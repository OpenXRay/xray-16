#pragma once

class CBlender_Tree : public IBlender
{
    xrP_BOOL oBlend;
    xrP_BOOL oNotAnTree;

private:
    void CompileForEditor(CBlender_Compile& C);

public:
    CBlender_Tree();
    ~CBlender_Tree() override = default;

    LPCSTR getComment() override;
    BOOL canBeDetailed() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;

    void Compile(CBlender_Compile& C) override;
};
