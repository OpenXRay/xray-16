#pragma once

class CBlender_SSAO_noMSAA : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: calc SSAO"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_SSAO_noMSAA();
    virtual ~CBlender_SSAO_noMSAA();
};

class CBlender_SSAO_MSAA : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: calc SSAO"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_SSAO_MSAA();
    virtual ~CBlender_SSAO_MSAA();

    virtual void SetDefine(LPCSTR Name, LPCSTR Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    LPCSTR Name;
    LPCSTR Definition;
};
