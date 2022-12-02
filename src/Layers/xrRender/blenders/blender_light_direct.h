#pragma once

class CBlender_accum_direct : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: accumulate direct light"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_direct();
    virtual ~CBlender_accum_direct();
};

#if RENDER != R_R2
class CBlender_accum_direct_msaa final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_accum_direct_msaa() = default;
    CBlender_accum_direct_msaa(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: accumulate direct light"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

class CBlender_accum_direct_volumetric_msaa final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_accum_direct_volumetric_msaa() = default;
    CBlender_accum_direct_volumetric_msaa(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: accumulate direct light"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

class CBlender_accum_direct_volumetric_sun_msaa final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_accum_direct_volumetric_sun_msaa() = default;
    CBlender_accum_direct_volumetric_sun_msaa(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: accumulate direct light"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};
#endif
