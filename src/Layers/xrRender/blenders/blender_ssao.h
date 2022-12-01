#pragma once

#if RENDER == R_R2
class CBlender_SSAO : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: calc SSAO"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_SSAO();
    virtual ~CBlender_SSAO();
};
using CBlender_SSAO_noMSAA = CBlender_SSAO; // XXX: hack to get rid of ifdefs, later should get rid of this hack also.
#else
class CBlender_SSAO_noMSAA : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: calc SSAO"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_SSAO_noMSAA();
    virtual ~CBlender_SSAO_noMSAA();
};

class CBlender_SSAO_MSAA final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_SSAO_MSAA() = default;
    CBlender_SSAO_MSAA(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: calc SSAO"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};
#endif
