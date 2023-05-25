#pragma once

namespace ELight
{
    enum EFlags
    {
        flAffectStatic = (1 << 0),
        flAffectDynamic = (1 << 1),
        flProcedural = (1 << 2),
        flBreaking = (1 << 3),
        flPointFuzzy = (1 << 4),
        flCastShadow = (1 << 5),
    };

    enum EType
    {
        ltPoint = D3DLIGHT_POINT,
        ltSpot = D3DLIGHT_SPOT,
        ltDirect = D3DLIGHT_DIRECTIONAL,
        ltMaxCount,
        lt_max_type = u32(-1),
    };
};

#define LCONTROL_HEMI "$hemi" // hemisphere
#define LCONTROL_SUN "$sun" // sun
#define LCONTROL_STATIC "$static" // all other static lights

// refs
class CLAItem;

class CLight : public CCustomObject
{
protected:
    typedef CCustomObject inherited;

public:
    // flags
    Flags32 m_Flags;

    // light
    ELight::EType m_Type; /* Type of light source */
    Fcolor m_Color;       /* Diffuse color of light */
    float m_Brightness;
    float m_Range;        /* Cutoff range */
    float m_Attenuation0; /* Constant attenuation */
    float m_Attenuation1; /* Linear attenuation */
    float m_Attenuation2; /* Quadratic attenuation */
    float m_Cone;         /* Outer angle of spotlight cone */

    float m_VirtualSize;

    CLAItem *m_pAnimRef;

    // spot light
    shared_str m_FalloffTex;

    // controller
    u32 m_LControl;

    // build options
    BOOL m_UseInD3D;

    // fuzzy
    struct SFuzzyData
    {
        enum EShapeType
        {
            fstSphere,
            fstBox,
            fstForceU8 = u8(-1)
        };
        EShapeType m_ShapeType;
        float m_SphereRadius;
        Fvector m_BoxDimension;
        s16 m_PointCount;
        FvectorVec m_Positions;
        SFuzzyData()
        {
            m_ShapeType = fstSphere;
            m_SphereRadius = 0.1f;
            m_BoxDimension.set(0.1f, 0.1f, 0.1f);
            m_PointCount = 1;
            m_Positions.resize(m_PointCount, Fvector().set(0, 0, 0));
        }
        void Generate(Fvector &p)
        {
            switch (m_ShapeType)
            {
            case fstSphere:
                p.random_point(m_SphereRadius);
                break;
            case fstBox:
                p.random_point(m_BoxDimension);
                break;
            }
        }
        void SaveStream(IWriter &F);
        void LoadStream(IReader &F);
        void SaveLTX(CInifile &ini, LPCSTR sect_name);
        void LoadLTX(CInifile &ini, LPCSTR sect_name);
    };
    SFuzzyData *m_FuzzyData;

    virtual void OnUpdateTransform();
    void OnTypeChange(PropValue *value);

    void OnFuzzyFlagChange(PropValue *value);
    void OnFuzzyDataChange(PropValue *value);
    void OnFuzzyTypeChange(PropValue *value);

    void OnFuzzyGenerateClick(ButtonValue *value, bool &bModif, bool &bSafe);
    void OnAutoClick(ButtonValue *value, bool &bModif, bool &bSafe);
    void OnNeedUpdate(PropValue *value);

    void OnPointDataChange(PropValue *value);
    void OnAttenuationDraw(CanvasValue *sender);
    void OnPointDataTestEqual(CanvasValue *a, CanvasValue *b, bool &res);

protected:
    //    virtual Fvector& GetPosition	()						{ return m_D3D.position; 	}
    //    virtual void 	SetPosition		(const Fvector& pos)	{ m_D3D.position.set(pos);	UpdateTransform();}
public:
    CLight(LPVOID data, LPCSTR name);
    void Construct(LPVOID data);
    virtual ~CLight();
    void CopyFrom(CLight *src);
    virtual bool CanAttach() { return true; }

    // pick functions
    virtual bool RayPick(float &distance, const Fvector &start, const Fvector &direction,
                         SRayPickInfo *pinf = NULL);
    virtual bool FrustumPick(const CFrustum &frustum);

    // change position/orientation methods
    virtual void Scale(Fvector &amount) { ; }

    // placement functions
    virtual bool GetBox(Fbox &box);

    // file system function
    virtual bool LoadStream(IReader &);
    virtual bool LoadLTX(CInifile &ini, LPCSTR sect_name);
    virtual void SaveStream(IWriter &);
    virtual void SaveLTX(CInifile &ini, LPCSTR sect_name);

    virtual void FillProp(LPCSTR pref, PropItemVec &items);
    void FillSunProp(LPCSTR pref, PropItemVec &items);
    void FillPointProp(LPCSTR pref, PropItemVec &items);
    void FillSpotProp(LPCSTR pref, PropItemVec &items);
    void FillAttProp(LPCSTR pref, PropItemVec &items);
    virtual bool GetSummaryInfo(SSceneSummary *inf);

    // render utility function
    void AffectD3D(BOOL flag);

    virtual void Render(int priority, bool strictB2F);
    virtual void OnFrame();
    void Update();

    LPCSTR GetLControlName();

    // events
    virtual void OnShowHint(AStringVec &dest);
};
