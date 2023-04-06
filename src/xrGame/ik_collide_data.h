#pragma once
#include "xrCore/_fbox.h"

class ik_foot_geom
{
public:
    enum e_collide_point
    {
        toe = 0,
        heel,
        side,
        none = -1
    };
    IC void set(const Fvector& toe, const Fvector& heel, const Fvector& side)
    {
        _toe = toe;
        _heel = heel;
        _side = side;
    }
    IC bool is_valid() const
    {
        Fbox test;
        test.vMax.set(FLT_MAX / 2.f, FLT_MAX / 2.f, FLT_MAX / 2.f);
        test.vMin.set(-FLT_MAX / 2.f, -FLT_MAX / 2.f, -FLT_MAX / 2.f);
        return test.contains(pos_toe()) && test.contains(pos_heel()) && test.contains(pos_side());
    }
    ik_foot_geom()
        : _toe(Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX)), _heel(Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX)),
          _side(Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX))
    {
    }
    IC const Fvector& pos_toe() const { return _toe; }
    IC const Fvector& pos_heel() const { return _heel; }
    IC const Fvector& pos_side() const { return _side; }
private:
    Fvector _toe;
    Fvector _heel;
    Fvector _side;
};

struct SIKCollideData
{
    ik_foot_geom::e_collide_point m_collide_point{ ik_foot_geom::toe };
    Fplane m_plane;
    Fvector m_pick_dir;
    bool collided{};
    SIKCollideData() : m_pick_dir({ 0, -1, 0 }) {}
};
