#pragma once

class CBlender_accum_spot : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: accumulate spot light"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_spot();
    virtual ~CBlender_accum_spot();
};
