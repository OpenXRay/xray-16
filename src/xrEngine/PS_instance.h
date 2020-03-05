#pragma once
#ifndef _CPS_Instance_H_
#define _CPS_Instance_H_

#include "xrCDB/ISpatial.h"
#include "ISheduled.h"
#include "IRenderable.h"
#include "xrCore/_bitwise.h"

class ENGINE_API CPS_Instance : public SpatialBase, public ScheduledBase, public RenderableBase
{
    friend class IGame_Persistent;

private:
    bool m_destroy_on_game_load;

protected:
    int m_iLifeTime;
    bool m_bAutoRemove;
    bool m_bDead;

protected:
    virtual ~CPS_Instance();
    virtual void PSI_internal_delete();

public:
    CPS_Instance(bool destroy_on_game_load);

    IC const bool& destroy_on_game_load() const { return m_destroy_on_game_load; }
    virtual void PSI_destroy();
    IC bool PSI_alive() { return m_iLifeTime > 0; }
    IC bool PSI_IsAutomatic() { return m_bAutoRemove; }
    IC void PSI_SetLifeTime(float life_time) { m_iLifeTime = iFloor(life_time * 1000); }
    virtual void Play(bool bHudMode) = 0;
    virtual bool Locked() { return false; }
    virtual shared_str shedule_Name() const { return shared_str("particle_instance"); };
    virtual void shedule_Update(u32 dt);
    virtual IRenderable* dcast_Renderable() { return this; }
};

#endif
