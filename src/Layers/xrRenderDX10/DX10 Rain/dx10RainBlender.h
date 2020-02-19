#ifndef dx10RainBlender_included
#define dx10RainBlender_included

class CBlender_rain : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: DX10 rain blender"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

class CBlender_rain_msaa : public IBlender
{
public:
    CBlender_rain_msaa()
    {
        Name = 0;
        Definition = 0;
    }
    virtual const char* getComment() { return "INTERNAL: DX10 MSAA rain blender"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
    virtual void SetDefine(const char* Name, const char* Definition);

    const char* Name;
    const char* Definition;
};

#endif //	dx10RainBlender_included
