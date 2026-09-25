#pragma once

namespace xray::render::fg
{
class CBlender_Detail_Still : public IBlender
{
public:
    xrP_BOOL oBlend;

public:
    CBlender_Detail_Still();
    ~CBlender_Detail_Still() override = default;

    LPCSTR getComment() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
};
} // namespace xray::render::fg
