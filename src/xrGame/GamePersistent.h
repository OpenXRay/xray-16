#ifndef GamePersistentH
#define GamePersistentH
#pragma once

#include "xrEngine/IGame_Persistent.h"
class CMainMenu;
class CUICursor;
class CParticlesObject;
class CUISequencer;
class UICore;
class AnselManager;

class CGamePersistent : public IGame_Persistent, public IEventReceiver
{
protected:
    using inherited = IGame_Persistent;

private:
    // ambient particles
    CParticlesObject* ambient_particles;
    u32 ambient_sound_next_time[20]; // max snd channels
    u32 ambient_effect_next_time;
    u32 ambient_effect_stop_time;

    float ambient_effect_wind_start;
    float ambient_effect_wind_in_time;
    float ambient_effect_wind_end;
    float ambient_effect_wind_out_time;
    bool ambient_effect_wind_on;

    bool m_bPickableDOF;

    AnselManager* ansel;

    CUISequencer* m_intro;
    EVENT eQuickLoad;
    Fvector m_dof[4]; // 0-dest 1-current 2-from 3-original

    fastdelegate::FastDelegate0<> m_intro_event;

    void xr_stdcall start_logo_intro();
    void xr_stdcall update_logo_intro();

    void xr_stdcall game_loaded();
    void xr_stdcall update_game_loaded();

    void xr_stdcall start_game_intro();
    void xr_stdcall update_game_intro();

#ifdef DEBUG
    u32 m_frame_counter;
    u32 m_last_stats_frame;
#endif

    void WeathersUpdate();
    void UpdateDof();

public:
    IReader* pDemoFile;
    u32 uTime2Change;
    EVENT eDemoStart;

    CGamePersistent();
    virtual ~CGamePersistent();

    void PreStart(LPCSTR op) override;
    virtual void Start(LPCSTR op);
    virtual void Disconnect();

    virtual void OnAppActivate();
    virtual void OnAppDeactivate();

    virtual void OnAppStart();
    virtual void OnAppEnd();
    virtual void OnGameStart();
    virtual void OnGameEnd();

    virtual void OnFrame();
    virtual void OnEvent(EVENT E, u64 P1, u64 P2);

    virtual void UpdateGameType();

    virtual void RegisterModel(IRenderVisual* V);
    virtual float MtlTransparent(u32 mtl_idx);
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

    virtual bool OnRenderPPUI_query();
    virtual void OnRenderPPUI_main();
    virtual void OnRenderPPUI_PP();
    virtual void LoadTitle(bool change_tip = false, shared_str map_name = "");
    void SetLoadStageTitle(pcstr ls_title = nullptr) override;

    virtual bool CanBePaused();

    void SetPickableEffectorDOF(bool bSet);
    void SetEffectorDOF(const Fvector& needed_dof);
    void RestoreEffectorDOF();

    virtual void GetCurrentDof(Fvector3& dof);
    virtual void SetBaseDof(const Fvector3& dof);
    virtual void OnSectorChanged(int sector);
    virtual void OnAssetsChanged();
};

IC CGamePersistent& GamePersistent() { return *((CGamePersistent*)g_pGamePersistent); }
#endif // GamePersistentH
