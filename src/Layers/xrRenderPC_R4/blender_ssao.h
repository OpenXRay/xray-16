#pragma once

class CBlender_SSAO_noMSAA : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: calc SSAO"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_SSAO_noMSAA();
    virtual ~CBlender_SSAO_noMSAA();
};

class CBlender_SSAO_MSAA : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: calc SSAO"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

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
