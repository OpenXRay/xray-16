//----------------------------------------------------
#pragma once
#ifndef MotionH
#define MotionH

#include "xrCore/Animation/Bone.hpp"
#include "xrCore/_std_extensions.h"
#include "xrCommon/xr_vector.h"

// refs
class CEnvelope;
class IWriter;
class IReader;
class motion_marks;

enum EChannelType
{
    ctUnsupported = -1,
    ctPositionX = 0,
    ctPositionY,
    ctPositionZ,
    ctRotationH,
    ctRotationP,
    ctRotationB,
    ctMaxChannel
};

struct st_BoneMotion
{
    enum
    {
        flWorldOrient = 1 << 0,
    };
    shared_str name;
    CEnvelope* envs[ctMaxChannel];
    Flags8 m_Flags;
    st_BoneMotion()
    {
        name = 0;
        m_Flags.zero();
        ZeroMemory(envs, sizeof(CEnvelope*) * ctMaxChannel);
    }
    void SetName(LPCSTR nm) { name = nm; }
};
// vector по костям
using BoneMotionVec = xr_vector<st_BoneMotion>;

//--------------------------------------------------------------------------
class XRCORE_API CCustomMotion
{
protected:
    enum EMotionType
    {
        mtObject = 0,
        mtSkeleton,
        ForceDWORD = u32(-1)
    };
    EMotionType mtype;
    int iFrameStart, iFrameEnd;
    float fFPS;

public:
    shared_str name;

    CCustomMotion();
    CCustomMotion(CCustomMotion* src);
    virtual ~CCustomMotion();

    void SetName(const char* n)
    {
        string256 tmp;
        tmp[0] = 0;
        if (n)
        {
            xr_strcpy(tmp, n);
            xr_strlwr(tmp);
        }
        name = tmp;
    }
    LPCSTR Name() { return name.c_str(); }
    int FrameStart() { return iFrameStart; }
    int FrameEnd() { return iFrameEnd; }
    float FPS() { return fFPS; }
    int Length() { return iFrameEnd - iFrameStart + 1; }
    void SetParam(int s, int e, float fps)
    {
        iFrameStart = s;
        iFrameEnd = e;
        fFPS = fps;
    }

    virtual void Save(IWriter& F);
    virtual bool Load(IReader& F);

    virtual void SaveMotion(const char* buf) = 0;
    virtual bool LoadMotion(const char* buf) = 0;
};

//--------------------------------------------------------------------------
class XRCORE_API COMotion final : public CCustomMotion
{
protected:
    CEnvelope* envs[ctMaxChannel];

public:
    COMotion();
    COMotion(COMotion* src);
    ~COMotion() override;

    void Clear();

    void _Evaluate(float t, Fvector& T, Fvector& R);
    void Save(IWriter& F) override;
    bool Load(IReader& F) override;

    void SaveMotion(const char* buf) override;
    bool LoadMotion(const char* buf) override;

    void FindNearestKey(float t, float& min_k, float& max_k, float eps = EPS_L);
    void CreateKey(float t, const Fvector& P, const Fvector& R);
    void DeleteKey(float t);
    void NormalizeKeys();
    int KeyCount();
    CEnvelope* Envelope(EChannelType et = ctPositionX) { return envs[et]; }
    BOOL ScaleKeys(float from_time, float to_time, float scale_factor);
    BOOL NormalizeKeys(float from_time, float to_time, float speed);
    float GetLength(float* mn = 0, float* mx = 0);
};

//--------------------------------------------------------------------------

enum ESMFlags
{
    esmFX = 1 << 0,
    esmStopAtEnd = 1 << 1,
    esmNoMix = 1 << 2,
    esmSyncPart = 1 << 3,
    esmUseFootSteps = 1 << 4,
    esmRootMover = 1 << 5,
    esmIdle = 1 << 6,
    esmUseWeaponBone = 1 << 7,
};

#include "SkeletonMotions.hpp"

class XRCORE_API CSMotion final : public CCustomMotion
{
protected:
    BoneMotionVec bone_mots;

public:
    u16 m_BoneOrPart;
    float fSpeed;
    float fAccrue;
    float fFalloff;
    float fPower;
    Flags8 m_Flags;

    xr_vector<motion_marks> marks;

    void Clear();

public:
    CSMotion();
    CSMotion(CSMotion* src);
    ~CSMotion() override;

    void _Evaluate(int bone_idx, float t, Fvector& T, Fvector& R);

    void CopyMotion(CSMotion* src);

    st_BoneMotion* FindBoneMotion(shared_str name);
    BoneMotionVec& BoneMotions() { return bone_mots; }
    Flags8 GetMotionFlags(int bone_idx) { return bone_mots[bone_idx].m_Flags; }
    void add_empty_motion(shared_str const& bone_id);

    void Save(IWriter& F) override;
    bool Load(IReader& F) override;

    void SaveMotion(const char* buf) override;
    bool LoadMotion(const char* buf) override;

    void SortBonesBySkeleton(BoneVec& bones);
    void WorldRotate(int boneId, float h, float p, float b);

    void Optimize();
};

struct XRCORE_API SAnimParams
{
    float t_current;
    float tmp;
    float min_t;
    float max_t;
    BOOL bPlay;
    BOOL bWrapped;

public:
    SAnimParams()
    {
        bWrapped = false;
        bPlay = false;
        t_current = 0.f;
        min_t = 0.f;
        max_t = 0.f;
        tmp = 0.f;
    }
    void Set(CCustomMotion* M);
    void Set(float start_frame, float end_frame, float fps);
    float Frame() { return t_current; }
    void Update(float dt, float speed, bool loop);
    void Play()
    {
        bPlay = true;
        t_current = min_t;
        tmp = min_t;
    }
    void Stop()
    {
        bPlay = false;
        t_current = min_t;
        tmp = min_t;
    }
    void Pause(bool val) { bPlay = !val; }
};

class XRCORE_API CClip
{
public:
    struct AnimItem
    {
        shared_str name;
        u16 slot;
        AnimItem() : slot(u16(-1)) {}
        void set(shared_str nm, u16 s)
        {
            name = nm;
            slot = s;
        }
        void clear() { set("", u16(-1)); }
        bool valid() { return !!(name.size() && (slot != u16(-1))); }
        bool equal(const AnimItem& d) const { return name.equal(d.name) && (slot == d.slot); }
    };
    shared_str name;
    AnimItem cycles[4];
    AnimItem fx;

    float fx_power;
    float length;

public:
    virtual void Save(IWriter& F);
    virtual bool Load(IReader& F);
    bool Equal(CClip* c);
};
#endif
