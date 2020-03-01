#pragma once

class CBlender_accum_spot : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: accumulate spot light"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_spot();
    virtual ~CBlender_accum_spot();
};

class CBlender_accum_spot_msaa : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: accumulate spot light msaa"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    virtual void SetDefine(LPCSTR Name, LPCSTR Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    CBlender_accum_spot_msaa();
    virtual ~CBlender_accum_spot_msaa();
    LPCSTR Name;
    LPCSTR Definition;
};

class CBlender_accum_volumetric_msaa : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: accumulate spot light msaa"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    virtual void SetDefine(LPCSTR Name, LPCSTR Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    CBlender_accum_volumetric_msaa();
    virtual ~CBlender_accum_volumetric_msaa();
    LPCSTR Name;
    LPCSTR Definition;
};
