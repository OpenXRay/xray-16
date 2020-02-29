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

class CBlender_accum_direct_msaa : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: accumulate direct light"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }
    const char* Name;
    const char* Definition;

    CBlender_accum_direct_msaa();
    virtual ~CBlender_accum_direct_msaa();
};

class CBlender_accum_direct_volumetric_msaa : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: accumulate direct light"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    CBlender_accum_direct_volumetric_msaa();
    virtual ~CBlender_accum_direct_volumetric_msaa();
    const char* Name;
    const char* Definition;
};

class CBlender_accum_direct_volumetric_sun_msaa : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: accumulate direct light"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    CBlender_accum_direct_volumetric_sun_msaa();
    virtual ~CBlender_accum_direct_volumetric_sun_msaa();
    const char* Name;
    const char* Definition;
};
