#pragma once

class CBlender_SSAO_noMSAA : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: calc SSAO"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_SSAO_noMSAA();
    virtual ~CBlender_SSAO_noMSAA();
};

class CBlender_SSAO_MSAA : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: calc SSAO"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_SSAO_MSAA();
    virtual ~CBlender_SSAO_MSAA();

    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    const char* Name;
    const char* Definition;
};
