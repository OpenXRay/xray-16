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

class CBlender_accum_reflected_msaa : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: accumulate reflected light"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_reflected_msaa();
    virtual ~CBlender_accum_reflected_msaa();
    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }
    const char* Name;
    const char* Definition;
};
