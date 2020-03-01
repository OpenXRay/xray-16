#pragma once
#ifndef __SPECTATOR_H__
#define __SPECTATOR_H__

#include "xrEngine/Feel_Touch.h"
#include "xrEngine/IInputReceiver.h"

#include "Entity.h"
#include "Actor_Flags.h"

// refs
class CActor;

class CSpectator : public CGameObject, public IInputReceiver
{
private:
    typedef CGameObject inherited;
    CTimer m_timer; // for pause case (in demo mode)
    float m_fTimeDelta;

protected:
public:
    enum EActorCameras
    {
        eacFreeFly = 0,
        eacFirstEye,
        eacLookAt,
        eacFreeLook,
        eacFixedLookAt,
        eacMaxCam
    };

private:
    // Cameras
    CCameraBase* cameras[eacMaxCam];
    EActorCameras cam_active;

    int look_idx;

    //------------------------------
    void cam_Set(EActorCameras style);
    void cam_Update(CActor* A = 0);

    CActor* m_pActorToLookAt;
    bool SelectNextPlayerToLook(bool const search_next);

    void FirstEye_ToPlayer(IGameObject* pObject);

    static const float cam_inert_value;
    float prev_cam_inert_value;
    shared_str m_last_player_name;
    EActorCameras m_last_camera;

public:
    CSpectator();
    virtual ~CSpectator();

    virtual void IR_OnMouseMove(int x, int y);
    virtual void IR_OnKeyboardPress(int dik);
    virtual void IR_OnKeyboardRelease(int dik);
    virtual void IR_OnKeyboardHold(int dik);
    virtual void shedule_Update(u32 T);
    virtual void UpdateCL();
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();

    virtual void Center(Fvector& C) const { C.set(Position()); }
    virtual float Radius() const { return EPS; }
    //	virtual const Fbox&		BoundingBox				()				const	{ VERIFY2(renderable.visual,*cName());
    //return
    // renderable.visual->vis.box;									}
    virtual CGameObject* cast_game_object() { return this; }
    virtual IInputReceiver* cast_input_receiver() { return this; }
    virtual void net_Relcase(IGameObject* O);
    void GetSpectatorString(string1024& pStr);

    virtual void On_SetEntity();
    virtual void On_LostEntity();

    EActorCameras GetActiveCam() const { return cam_active; }

    CCameraBase* cam_Active() { return cameras[cam_active]; }
    CCameraBase* cam_FirstEye() { return cameras[eacFirstEye]; }
};

#endif // __SPECTATOR_H__
