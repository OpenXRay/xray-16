// EffectorShot.h: interface for the CCameraShotEffector class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "CameraEffector.h"
#include "xrEngine/CameraManager.h"
#include "Actor.h"
#include "CameraRecoil.h"

class CWeapon;

class CWeaponShotEffector
{
protected:
    CameraRecoil m_cam_recoil;

    float m_angle_vert;
    float m_angle_horz;

    float m_prev_angle_vert;
    float m_prev_angle_horz;

    float m_delta_vert;
    float m_delta_horz;

    int m_shot_numer;
    bool m_shot_end;
    bool m_first_shot;
    //	float			m_first_shot_pos;

    bool m_actived;
    bool m_single_shot;

private:
    CRandom m_Random;
    s32 m_LastSeed;

public:
    CWeaponShotEffector();
    virtual ~CWeaponShotEffector(){};

    void Initialize(const CameraRecoil& cam_recoil);
    void Reset();

    IC bool IsActive() { return m_actived; }
    //		void	SetActive			(bool Active)		{			m_actived = Active;		}
    IC void StopShoting() { m_shot_end = true; }
    void Update();

    void SetRndSeed(s32 Seed);

    void Shot(CWeapon* weapon);
    void Shot2(float angle);

    void GetDeltaAngle(Fvector& angle);
    void GetLastDelta(Fvector& delta_angle);
    void ChangeHP(float* pitch, float* yaw);

protected:
    void Relax();
};

class CCameraShotEffector : public CWeaponShotEffector, public CEffectorCam
{
protected:
    CActor* m_pActor;

public:
    //-					CCameraShotEffector	(float max_angle, float relax_speed, float max_angle_horz, float
    //step_angle_horz,
    // float angle_frac);
    CCameraShotEffector(const CameraRecoil& cam_recoil);
    virtual ~CCameraShotEffector();

    virtual BOOL ProcessCam(SCamEffectorInfo& info);
    virtual void SetActor(CActor* pActor) { m_pActor = pActor; };
    virtual CCameraShotEffector* cast_effector_shot() { return this; }
    u16 m_WeaponID;
};
