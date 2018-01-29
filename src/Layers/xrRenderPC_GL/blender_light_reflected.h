#pragma once

class CBlender_accum_reflected : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: accumulate reflected light"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_reflected();
    virtual ~CBlender_accum_reflected();
};

class CBlender_accum_reflected_msaa : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: accumulate reflected light"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_accum_reflected_msaa();
    virtual ~CBlender_accum_reflected_msaa();

    virtual void SetDefine(LPCSTR Name, LPCSTR Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    LPCSTR Name;
    LPCSTR Definition;
};
