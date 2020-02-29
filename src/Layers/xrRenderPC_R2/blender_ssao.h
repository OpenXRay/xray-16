#pragma once

class CBlender_SSAO : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: calc SSAO"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_SSAO();
    virtual ~CBlender_SSAO();
};
