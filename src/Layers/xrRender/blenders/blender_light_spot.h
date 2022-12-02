#pragma once

class CBlender_accum_spot : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: accumulate spot light"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_spot();
    virtual ~CBlender_accum_spot();
};

#if RENDER != R_R2
class CBlender_accum_spot_msaa final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_accum_spot_msaa() = default;
    CBlender_accum_spot_msaa(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: accumulate spot light msaa"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

class CBlender_accum_volumetric_msaa final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_accum_volumetric_msaa() = default;
    CBlender_accum_volumetric_msaa(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: accumulate spot light msaa"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};
#endif
