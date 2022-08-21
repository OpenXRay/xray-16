#pragma once

class CBlender_accum_point : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: accumulate point light"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_point();
    virtual ~CBlender_accum_point();
};

#if RENDER != R_R2
class CBlender_accum_point_msaa final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_accum_point_msaa() = default;
    CBlender_accum_point_msaa(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: accumulate point light msaa"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};
#endif
