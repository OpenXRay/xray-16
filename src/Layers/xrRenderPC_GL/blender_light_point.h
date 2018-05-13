#pragma once

class CBlender_accum_point : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: accumulate point light"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_point();
    virtual ~CBlender_accum_point();
};

class CBlender_accum_point_msaa : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: accumulate point light msaa"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_point_msaa();
    virtual ~CBlender_accum_point_msaa();

    virtual void SetDefine(LPCSTR Name, LPCSTR Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    LPCSTR Name;
    LPCSTR Definition;
};
