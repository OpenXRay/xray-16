#pragma once

class CBlender_accum_direct : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: accumulate direct light"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_direct();
    virtual ~CBlender_accum_direct();
};
