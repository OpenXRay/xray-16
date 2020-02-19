#pragma once

class CBlender_accum_spot : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: accumulate spot light"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_spot();
    virtual ~CBlender_accum_spot();
};

class CBlender_accum_spot_msaa : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: accumulate spot light msaa"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

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
    const char* getComment() override { return "INTERNAL: accumulate spot light msaa"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

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
