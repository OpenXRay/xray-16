#pragma once

class CBlender_accum_reflected : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: accumulate reflected light"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_reflected();
    virtual ~CBlender_accum_reflected();
};

#if RENDER != R_R2
class CBlender_accum_reflected_msaa final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_accum_reflected_msaa() = default;
    CBlender_accum_reflected_msaa(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: accumulate reflected light"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};
#endif
