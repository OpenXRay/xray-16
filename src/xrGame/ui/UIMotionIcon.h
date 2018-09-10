#pragma once
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/ProgressBar/UIProgressShape.h"

class CUIMotionIcon : public CUIWindow
{
    typedef CUIWindow inherited;

public:
private:
    CUIProgressShape m_luminosity_progress;
    CUIProgressShape m_noise_progress;

    struct _npc_visibility
    {
        u16 id;
        float value;
        bool operator==(const u16& _id) { return id == _id; }
        bool operator<(const _npc_visibility& m) const { return (value < m.value); }
    };
    xr_vector<_npc_visibility> m_npc_visibility;
    bool m_bchanged;
    float m_luminosity;
    float cur_pos;

public:
    virtual ~CUIMotionIcon();
    CUIMotionIcon();
    virtual void Update();
    virtual void Draw();
    void Init(Frect const& rect);
    void SetNoise(float Pos);
    void SetLuminosity(float Pos);
    void SetActorVisibility(u16 who_id, float value);
    void ResetVisibility();
};
