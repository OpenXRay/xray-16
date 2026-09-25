#pragma once

namespace xray::render::fg
{
class CBlender_Vertex_aref : public IBlender
{
public:
    xrP_Integer oAREF;
    xrP_BOOL oBlend;

public:
    CBlender_Vertex_aref();
    ~CBlender_Vertex_aref() override = default;

    LPCSTR getComment() override;

    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
};
} // namespace xray::render::fg
