#pragma once

class CBlender_accum_direct_mask : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: mask direct light"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_direct_mask();
    virtual ~CBlender_accum_direct_mask();
};

class CBlender_accum_direct_mask_msaa : public IBlender
{
public:

    LPCSTR getComment() override { return "INTERNAL: mask direct light msaa"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    virtual void SetDefine(LPCSTR Name, LPCSTR Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_direct_mask_msaa();
    virtual ~CBlender_accum_direct_mask_msaa();

    LPCSTR Name;
    LPCSTR Definition;
};
