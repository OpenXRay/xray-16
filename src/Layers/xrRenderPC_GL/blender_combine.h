#pragma once

class CBlender_combine : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: combiner"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_combine();
    virtual ~CBlender_combine();
};

class CBlender_combine_msaa : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: combiner"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_combine_msaa();
    virtual ~CBlender_combine_msaa();

    virtual void SetDefine(LPCSTR Name, LPCSTR Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    LPCSTR Name;
    LPCSTR Definition;
};
