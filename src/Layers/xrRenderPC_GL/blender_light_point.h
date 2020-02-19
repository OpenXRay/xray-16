#pragma once

class CBlender_accum_point : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: accumulate point light"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_point();
    virtual ~CBlender_accum_point();
};

class CBlender_accum_point_msaa : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: accumulate point light msaa"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_point_msaa();
    virtual ~CBlender_accum_point_msaa();

    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    const char* Name;
    const char* Definition;
};
