#pragma once

class CBlender_accum_direct_mask : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: mask direct light"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_direct_mask();
    virtual ~CBlender_accum_direct_mask();
};

#if RENDER != R_R2
class CBlender_accum_direct_mask_msaa final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_accum_direct_mask_msaa() = default;
    CBlender_accum_direct_mask_msaa(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: mask direct light msaa"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};
#endif
