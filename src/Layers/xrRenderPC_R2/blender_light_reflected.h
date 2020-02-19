#pragma once

class CBlender_accum_reflected : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: accumulate reflected light"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_reflected();
    virtual ~CBlender_accum_reflected();
};
