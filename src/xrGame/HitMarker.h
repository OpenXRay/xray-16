#pragma once
#ifndef __XR_HITMARKER_H__
#define __XR_HITMARKER_H__

#include "Include/xrRender/FactoryPtr.h"
#include "xrUICore/ui_defs.h"
#include "Common/Noncopyable.hpp"
#include "xrCommon/xr_deque.h"

class IUIShader;
class CUIStaticItem;
class CLAItem;
class CGrenade;

struct SHitMark
{
    CUIStaticItem* m_UIStaticItem;
    float m_StartTime;
    float m_HitDirection;
    CLAItem* m_lanim;

    SHitMark(const ui_shader& sh, const Fvector& dir);
    ~SHitMark();
    bool IsActive();
    void UpdateAnim();
    void Draw(float dir);
};

struct SGrenadeMark
{
    CGrenade* p_grenade;
    bool removed_grenade;

    CUIStaticItem* m_UIStaticItem;
    float m_LastTime;
    float m_Angle;
    CLAItem* m_LightAnim;

    SGrenadeMark(const ui_shader& sh, CGrenade* grn);
    ~SGrenadeMark();

    bool IsActive() const;
    void Draw(float cam_dir);
    void Update(float angle);
};

class CHitMarker : private Noncopyable
{
public:
    FactoryPtr<IUIShader> hShader2;
    FactoryPtr<IUIShader> hShader_Grenade;

    typedef xr_deque<SHitMark*> HITMARKS;
    typedef xr_deque<SGrenadeMark*> GRENADEMARKS;

    HITMARKS m_HitMarks;
    GRENADEMARKS m_GrenadeMarks;

public:
    CHitMarker();
    ~CHitMarker();

    void Render();
    void Hit(const Fvector& dir);
    bool AddGrenade_ForMark(CGrenade* grn);
    void Update_GrenadeView(Fvector& pos_actor);

    void InitShader(LPCSTR tex_name);
    void InitShader_Grenade(LPCSTR tex_name);

    void net_Relcase(IGameObject* obj);
};

#endif // __XR_HITMARKER_H__
