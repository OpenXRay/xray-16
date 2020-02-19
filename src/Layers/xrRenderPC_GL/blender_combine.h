#pragma once

class CBlender_combine : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: combiner"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_combine();
    virtual ~CBlender_combine();
};

class CBlender_combine_msaa : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: combiner"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_combine_msaa();
    virtual ~CBlender_combine_msaa();

    virtual void SetDefine(const char* Name, const char* Definition)
    {
        this->Name = Name;
        this->Definition = Definition;
    }

    const char* Name;
    const char* Definition;
};
