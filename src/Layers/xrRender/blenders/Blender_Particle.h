#pragma once

class CBlender_Particle : public IBlender
{
    xrP_TOKEN oBlend;
    xrP_Integer oAREF;
    xrP_BOOL oClamp;

public:
    CBlender_Particle();
    ~CBlender_Particle() override = default;

    LPCSTR getComment() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;

    void Compile(CBlender_Compile& C) override;
};
