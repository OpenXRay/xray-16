#pragma once

class CBlender_combine : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: combiner"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

    CBlender_combine();
    virtual ~CBlender_combine();
};

class CBlender_combine_msaa : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: combiner"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);

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
