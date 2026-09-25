#pragma once

namespace xray::render::fg
{
class CBlender_LmEbB : public IBlender
{
    string64 oT2_Name; // name of secondary texture
    string64 oT2_xform; // xform for secondary texture
    xrP_BOOL oBlend;

public:
    CBlender_LmEbB();
    ~CBlender_LmEbB() override = default;

    LPCSTR getComment() override;
    BOOL canBeLMAPped() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
};
} // namespace xray::render::fg
