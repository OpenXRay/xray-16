#pragma once

class CBlender_combine : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: combiner"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_combine();
    virtual ~CBlender_combine();
};

#if RENDER != R_R2
class CBlender_combine_msaa final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_combine_msaa() = default;
    CBlender_combine_msaa(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: combiner"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};
#endif
