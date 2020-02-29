#pragma once

class CBlender_accum_direct_mask : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: mask direct light"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_direct_mask();
    virtual ~CBlender_accum_direct_mask();
};

class CBlender_accum_direct_mask_msaa : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: mask direct light msaa"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_direct_mask_msaa();
    virtual ~CBlender_accum_direct_mask_msaa();

    const char* Name;
    const char* Definition;
};
