#pragma once

class CBlender_Detail_Still : public IBlender
{
public:
    xrP_BOOL oBlend;

public:
    LPCSTR getComment() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;

    void Compile(CBlender_Compile& C) override;

    CBlender_Detail_Still();
    ~CBlender_Detail_Still() override = default;
};
