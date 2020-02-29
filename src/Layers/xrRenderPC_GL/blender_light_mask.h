#pragma once

class CBlender_accum_direct_mask : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: mask direct light"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_direct_mask();
    virtual ~CBlender_accum_direct_mask();
};

class CBlender_accum_direct_mask_msaa : public IBlender
{
public:

    const char* getComment() override { return "INTERNAL: mask direct light msaa"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_direct_mask_msaa();
    virtual ~CBlender_accum_direct_mask_msaa();

    const char* Name;
    const char* Definition;
};
