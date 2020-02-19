#ifndef dx11HDAOCSBlender_included
#define dx11HDAOCSBlender_included

class CBlender_CS_HDAO : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: DX11 CS for HDAO"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

class CBlender_CS_HDAO_MSAA : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: DX11 CS for HDAO"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

#endif
