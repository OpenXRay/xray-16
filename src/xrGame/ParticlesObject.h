#pragma once

#include "xrEngine/PS_instance.h"

extern const Fvector zero_vel;

class CParticlesObject : public CPS_Instance
{
    using inherited = CPS_Instance;

    u32 dwLastTime;
    void Init(LPCSTR p_name, IRender_Sector* S, BOOL bAutoRemove);
    void UpdateSpatial();

protected:
    bool m_bLooped; //флаг, что система зациклена
    bool m_bStopping; //вызвана функция Stop()

protected:
    u32 mt_dt;

protected:
    virtual ~CParticlesObject();

public:
    CParticlesObject(LPCSTR p_name, BOOL bAutoRemove, bool destroy_on_game_load);

    virtual bool shedule_Needed() { return true; };
    virtual float shedule_Scale();
    virtual void shedule_Update(u32 dt);
    void renderable_Render(IRenderable* root) override;
    void PerformAllTheWork(u32 dt);
    void __stdcall PerformAllTheWork_mt();

    Fvector& Position();
    void SetXFORM(const Fmatrix& m);
    IC Fmatrix& XFORM() { return renderable.xform; }
    void UpdateParent(const Fmatrix& m, const Fvector& vel);

    void play_at_pos(const Fvector& pos, BOOL xform = FALSE);
    virtual void Play(bool bHudMode);
    void Stop(BOOL bDefferedStop = TRUE);
    virtual bool Locked() { return mt_dt; }
    bool IsLooped() { return m_bLooped; }
    bool IsAutoRemove();
    bool IsPlaying();
    void SetAutoRemove(bool auto_remove);

    const shared_str Name();

public:
    static CParticlesObject* Create(LPCSTR p_name, BOOL bAutoRemove = TRUE, bool remove_on_game_load = true)
    {
        return new CParticlesObject(p_name, bAutoRemove, remove_on_game_load);
    }
    static void Destroy(CParticlesObject*& p)
    {
        if (p)
        {
            p->PSI_destroy();
            p = 0;
        }
    }
};
