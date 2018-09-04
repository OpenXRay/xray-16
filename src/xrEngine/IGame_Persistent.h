#pragma once
#ifndef IGame_PersistentH
#define IGame_PersistentH

#include "xrServerEntities/gametype_chooser.h"
#include "xrCommon/xr_set.h"
#include "xrCommon/xr_vector.h"
#include "xrCore/xr_trims.h"
#include "pure.h"
#ifndef _EDITOR
#include "Environment.h"
#include "EngineAPI.h"
#include "IGame_ObjectPool.h"
#endif
#include "ShadersExternalData.h" //--#SM+#--

class IRenderVisual;
class IMainMenu;
class ENGINE_API CPS_Instance;
//-----------------------------------------------------------------------------------------------------------
class ENGINE_API IGame_Persistent :
#ifndef _EDITOR
    public FactoryObjectBase,
#endif
    public pureAppStart,
    public pureAppEnd,
    public pureAppActivate,
    public pureAppDeactivate,
    public pureFrame
{
public:
    struct ParticleStatistics
    {
        u32 Starting;
        u32 Active;
        u32 Destroying;

        ParticleStatistics() { FrameStart(); }
        void FrameStart()
        {
            Starting = 0;
            Active = 0;
            Destroying = 0;
        }

        void FrameEnd() {}
    };
    union params
    {
        struct
        {
            string256 m_game_or_spawn;
            string256 m_game_type;
            string256 m_alife;
            string256 m_new_or_load;
            EGameIDs m_e_game_type;
        };
        string256 m_params[4];
        params() { reset(); }
        void reset()
        {
            for (int i = 0; i < 4; ++i)
                xr_strcpy(m_params[i], "");
        }
        void parse_cmd_line(LPCSTR cmd_line)
        {
            reset();
            int n = _min(4, _GetItemCount(cmd_line, '/'));
            for (int i = 0; i < n; ++i)
            {
                _GetItem(cmd_line, i, m_params[i], '/');
                xr_strlwr(m_params[i]);
            }
        }
    };
    params m_game_params;

public:
    xr_set<CPS_Instance*> ps_active;
    xr_vector<CPS_Instance*> ps_destroy;
    xr_vector<CPS_Instance*> ps_needtoplay;

public:
    void destroy_particles(const bool& all_particles);

public:
    virtual void PreStart(LPCSTR op);
    virtual void Start(LPCSTR op);
    virtual void Disconnect();
#ifndef _EDITOR
    IGame_ObjectPool ObjectPool;
    CEnvironment* pEnvironment;
    CEnvironment& Environment() { return *pEnvironment; };
    void Prefetch();
#endif
    IMainMenu* m_pMainMenu;
    static bool IsMainMenuActive();

    ParticleStatistics stats;

    ShadersExternalData* m_pGShaderConstants; //--#SM+#--

    const ParticleStatistics& GetStats() { return stats; }
    virtual bool OnRenderPPUI_query() { return FALSE; }; // should return true if we want to have second function called
    virtual void OnRenderPPUI_main(){};
    virtual void OnRenderPPUI_PP(){};

    virtual void OnAppStart();
    virtual void OnAppEnd();
    virtual void OnAppActivate();
    virtual void OnAppDeactivate();
    virtual void OnFrame();

    // вызывается только когда изменяется тип игры
    virtual void OnGameStart();
    virtual void OnGameEnd();

    virtual void UpdateGameType(){};
    virtual void GetCurrentDof(Fvector3& dof) { dof.set(-1.4f, 0.0f, 250.f); };
    virtual void SetBaseDof(const Fvector3& /*dof*/) {};
    virtual void OnSectorChanged(int /*sector*/) {};
    virtual void OnAssetsChanged();

    virtual void RegisterModel(IRenderVisual* V)
#ifndef _EDITOR
        = 0;
#else
    {
    }
#endif
    virtual float MtlTransparent(u32 mtl_idx)
#ifndef _EDITOR
        = 0;
#else
    {
        return 1.f;
    }
#endif

    IGame_Persistent();
    virtual ~IGame_Persistent();

    ICF u32 GameType() { return m_game_params.m_e_game_type; };
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);
    virtual void LoadTitle(bool /*change_tip*/ = false, shared_str /*map_name*/ = "") {}
    virtual void SetLoadStageTitle(pcstr /*ls_title*/) {}
    virtual bool CanBePaused() { return true; }
};

class IMainMenu
{
public:
    virtual ~IMainMenu(){};
    virtual void Activate(bool bActive) = 0;
    virtual bool IsActive() const = 0;
    virtual bool CanSkipSceneRendering() = 0;
    virtual void DestroyInternal(bool bForce) = 0;
};

extern ENGINE_API IGame_Persistent* g_pGamePersistent;
#endif // IGame_PersistentH
