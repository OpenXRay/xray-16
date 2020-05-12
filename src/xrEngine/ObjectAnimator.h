#ifndef ObjectAnimatorH
#define ObjectAnimatorH
#pragma once

#include "xrCore/Animation/Motion.hpp"

// refs
class ENGINE_API CObjectAnimator
{
    using MotionVec = xr_vector<COMotion*>;

protected:
    bool bLoop;

    shared_str m_Name;

    Fmatrix m_XFORM;
    SAnimParams m_MParam;
    MotionVec m_Motions;
    float m_Speed;

    COMotion* m_Current;
    void LoadMotions(pcstr fname);
    void SetActiveMotion(COMotion* mot);
    COMotion* FindMotionByName(pcstr name);

public:
    CObjectAnimator();
    virtual ~CObjectAnimator();

    void Clear();
    void Load(pcstr name);
    IC pcstr Name() const { return *m_Name; }
    float& Speed() { return m_Speed; }
    COMotion* Play(bool bLoop, pcstr name = 0);
    void Pause(bool val) { return m_MParam.Pause(val); }
    void Stop();
    IC bool IsPlaying() const { return m_MParam.bPlay; }
    IC const Fmatrix& XFORM() const { return m_XFORM; }
    float GetLength() const;
    // Update
    void Update(float dt);
    void DrawPath();
};

#endif // ObjectAnimatorH
