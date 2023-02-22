// Rain.h: interface for the CRain class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Include/xrRender/FactoryPtr.h"
#include "Include/xrRender/LensFlareRender.h"
#include "Include/xrRender/ThunderboltDescRender.h"
#include "Include/xrRender/ThunderboltRender.h"

// refs
class ENGINE_API IRender_DetailModel;
class ENGINE_API CLAItem;
class ENGINE_API CEnvDescriptorMixer;

struct ENGINE_API SThunderboltDesc
{
    // geom
    // IRender_DetailModel* l_model;
    FactoryPtr<IThunderboltDescRender> m_pRender;
    // sound
    ref_sound snd;
    // gradient
    struct SFlare
    {
        float fOpacity{};
        Fvector2 fRadius{};
        shared_str shader;
        shared_str texture;

        FactoryPtr<IFlareRender> m_pFlare;

        SFlare() = default;
        SFlare(float opacity, Fvector2 radius, pcstr sh, pcstr tex)
            : fOpacity(opacity), fRadius(radius), shader(sh), texture(tex)
        {
            m_pFlare->CreateShader(shader.c_str(), texture.c_str());
        }
        ~SFlare()
        {
            m_pFlare->DestroyShader();
        }
    };
    SFlare* m_GradientTop;
    SFlare* m_GradientCenter;
    shared_str name;
    CLAItem* color_anim;

public:
    SThunderboltDesc(const CInifile& pIni, shared_str const& sect);
    ~SThunderboltDesc();
    static SFlare* create_gradient(pcstr gradient_name, const CInifile& config, shared_str const& sect);
};

struct ENGINE_API SThunderboltCollection
{
    using DescVec = xr_vector<SThunderboltDesc*>;
    DescVec palette;
    shared_str section;

    SThunderboltCollection(shared_str sect, CInifile const* pIni, CInifile const* thunderbolts);
    ~SThunderboltCollection();

    SThunderboltDesc* GetRandomDesc()
    {
        VERIFY(palette.size() > 0);
        return palette[Random.randI(palette.size())];
    }
};

#define THUNDERBOLT_CACHE_SIZE 8
//
class ENGINE_API CEffect_Thunderbolt
{
    friend class dxThunderboltRender;

protected:
    xr_vector<SThunderboltCollection*> collections;
    SThunderboltDesc* current;

private:
    Fmatrix current_xform;
    Fvector3 current_direction;

    FactoryPtr<IThunderboltRender> m_pRender;
    // ref_geom hGeom_model;
    // states
    enum EState
    {
        stIdle,
        stWorking
    };
    EState state;

    // ref_geom hGeom_gradient;

    Fvector lightning_center;
    float lightning_size;
    float lightning_phase;

    float life_time;
    float current_time;
    float next_lightning_time;
    bool bEnabled;

    CInifile* m_thunderbolt_collections_config{};
    CInifile* m_thunderbolts_config{};

public:
    static constexpr float MAX_DIST_FACTOR = 0.95f;

    // params
    Fvector2 p_var_alt;
    float p_var_long;
    float p_min_dist;
    float p_tilt;
    float p_second_prop;
    float p_sky_color;
    float p_sun_color;
    float p_fog_color;

private:
    static bool RayPick(const Fvector& s, const Fvector& d, float& range);
    void Bolt(const CEnvDescriptorMixer& currentEnv);

public:
    CEffect_Thunderbolt();
    ~CEffect_Thunderbolt();

    void OnFrame(CEnvDescriptorMixer& currentEnv);
    void Render();

    SThunderboltCollection* AppendDef(shared_str sect);

    [[nodiscard]]
    auto& GetCollections() { return collections; }
};
