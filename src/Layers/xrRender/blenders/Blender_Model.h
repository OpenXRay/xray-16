#pragma once

namespace xray::render::fg
{
class CBlender_Model : public IBlender
{
public:
    xrP_TOKEN oTessellation;
    xrP_Integer oAREF;
    xrP_BOOL oBlend;

public:
    CBlender_Model();
    ~CBlender_Model() override = default;

    LPCSTR getComment() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
};
} // namespace xray::render::fg
