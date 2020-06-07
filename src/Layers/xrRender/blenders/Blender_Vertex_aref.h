#pragma once

class CBlender_Vertex_aref : public IBlender
{
    xrP_Integer oAREF;
    xrP_BOOL oBlend;

private:
    void CompileForEditor(CBlender_Compile& C);

public:
    CBlender_Vertex_aref();
    ~CBlender_Vertex_aref() override = default;

    LPCSTR getComment() override;
    
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
    
    void Compile(CBlender_Compile& C) override;
};
