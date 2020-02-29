#pragma once

class CBlender_accum_point : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: accumulate point light"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_point();
    virtual ~CBlender_accum_point();
};
