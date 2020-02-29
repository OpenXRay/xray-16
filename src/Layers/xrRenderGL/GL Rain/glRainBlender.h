#ifndef	dx10RainBlender_included
#define	dx10RainBlender_included

class CBlender_rain : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: DX10 rain blender"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;
};

class CBlender_rain_msaa : public IBlender
{
public:
    CBlender_rain_msaa()
    {
        Name = nullptr;
        Definition = nullptr;
    }

    const char* getComment() override { return "INTERNAL: DX10 MSAA rain blender"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;
    virtual void SetDefine(const char* Name, const char* Definition);

    const char* Name;
    const char* Definition;
};

#endif	//	dx10RainBlender_included
