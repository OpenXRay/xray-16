#pragma once

namespace xray::render::fg
{
class CBlender_Tree : public IBlender
{
public:
    xrP_BOOL oBlend;
    xrP_BOOL oNotAnTree;

public:
    CBlender_Tree();
    ~CBlender_Tree() override = default;

    LPCSTR getComment() override;
    BOOL canBeDetailed() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
};
} // namespace xray::render::fg
