//---------------------------------------------------------------------------
#ifndef SkeletonMotionsH
#define SkeletonMotionsH

#include "Bone.hpp"
#include "SkeletonMotionDefs.hpp"
#include "xrCore/_quaternion.h"
#include "xrCore/_vector3d.h"

class CBlend;

// callback
typedef void (*PlayCallback)(CBlend* P);

//*** Key frame definition ************************************************************************
enum
{
    flTKeyPresent = (1 << 0),
    flRKeyAbsent = (1 << 1),
    flTKey16IsBit = (1 << 2),
};
#pragma pack(push, 2)
struct CKeyQR
{
    s16 x, y, z, w; // rotation
};
struct CKeyQT8
{
    s8 x1, y1, z1;
};
struct CKeyQT16
{
    s16 x1, y1, z1;
};
#pragma pack(pop)


class XRCORE_API motion_marks
{
public:
    typedef std::pair<float, float> interval;
#ifdef _EDITOR
public:
#else
private:
#endif
    typedef xr_vector<interval> STORAGE;
    typedef STORAGE::iterator ITERATOR;
    typedef STORAGE::const_iterator C_ITERATOR;

    STORAGE intervals;

public:
    shared_str name;
    void Load(IReader*);
    void Save(IWriter*) const;
    const xr_vector<interval>& Intervals() const { return intervals; }
    void SetIntervals(xr_vector<interval> values) { intervals = std::move(values); }
    [[nodiscard]] bool is_empty() const { return intervals.empty(); }
    [[nodiscard]] const interval* pick_mark(float const& t) const;
    [[nodiscard]] bool is_mark_between(float const& t0, float const& t1) const;
    [[nodiscard]] float time_to_next_mark(float time) const;
};

const float fQuantizerRangeExt = 1.5f;
class XRCORE_API CMotionDef
{
public:
    u16 bone_or_part;
    u16 motion;
    u16 speed; // quantized: 0..10
    u16 power; // quantized: 0..10
    u16 accrue; // quantized: 0..10
    u16 falloff; // quantized: 0..10
    u16 flags;
    xr_vector<motion_marks> marks;

    IC float Dequantize(u16 V) const { return float(V) / 655.35f; }
    IC u16 Quantize(float V) const
    {
        s32 t = iFloor(V * 655.35f);
        clamp(t, 0, 65535);
        return u16(t);
    }

    [[nodiscard]] u32 mem_usage() const { return sizeof(*this); }
    ICF float Accrue() const { return fQuantizerRangeExt * Dequantize(accrue); }
    ICF float Falloff() const { return fQuantizerRangeExt * Dequantize(falloff); }
    ICF float Speed() const { return Dequantize(speed); }
    ICF float Power() const { return Dequantize(power); }
    bool StopAtEnd() const;
};
struct accel_str_pred
{
    IC bool operator()(const shared_str& x, const shared_str& y) const { return xr_strcmp(x, y) < 0; }
};
typedef xr_map<shared_str, u16, accel_str_pred> accel_map;


// partition
class XRCORE_API CPartDef
{
public:
    shared_str Name;
    xr_vector<u32> bones;
    CPartDef() : Name(0){};

    [[nodiscard]]
    u32 mem_usage() const { return sizeof(*this) + bones.size() * sizeof(u32) + sizeof(Name); }
};
class XRCORE_API CPartition
{
    CPartDef P[MAX_PARTS];

public:
    IC CPartDef& operator[](u16 id) { return P[id]; }
    IC const CPartDef& part(u16 id) const { return P[id]; }
    [[nodiscard]] u16 part_id(const shared_str& name) const;
    [[nodiscard]] u32 mem_usage() const { return P[0].mem_usage() * MAX_PARTS; }

    [[nodiscard]] u8 count() const
    {
        u8 ret = 0;
        for (u8 i = 0; i < MAX_PARTS; ++i)
            if (P[i].Name.size())
                ret++;
        return ret;
    };
};

struct MotionMetadata
{
    shared_str name;
    CMotionDef definition;
    float duration = 0.f;
};

struct MotionLibraryMetadata
{
    shared_str source;
    accel_map motions;
    accel_map cycles;
    accel_map effects;
    CPartition partition;
    xr_vector<MotionMetadata> clips;
};
//---------------------------------------------------------------------------
#endif
