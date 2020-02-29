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

class CBlender_accum_spot_msaa : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: accumulate spot light msaa"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    CBlender_accum_spot_msaa();
    virtual ~CBlender_accum_spot_msaa();
    const char* Name;
    const char* Definition;
};

class CBlender_accum_volumetric_msaa : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: accumulate spot light msaa"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    CBlender_accum_volumetric_msaa();
    virtual ~CBlender_accum_volumetric_msaa();
    const char* Name;
    const char* Definition;
};
