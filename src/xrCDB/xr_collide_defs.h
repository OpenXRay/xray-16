#pragma once

#include <algorithm>

#include "xrCDB.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_matrix.h"
#include "xrCommon/math_funcs_inline.h"

class IGameObject;
namespace collide
{
struct tri
{
    Fvector e10;
    float e10s;
    Fvector e21;
    float e21s;
    Fvector e02;
    float e02s;
    Fvector p[3];
    Fvector N;
    float d;
};

struct elipsoid
{
    Fmatrix mL2W; // convertion from sphere(000,1) to real space
    Fmatrix mW2L; // convertion from real space to sphere(000,1)
};

struct ray_cache
{
    // previous state
    Fvector start;
    Fvector dir;
    float range;
    bool result;

    // cached vertices
    Fvector verts[3];

    ray_cache()
    {
        start.set(0, 0, 0);
        dir.set(0, 0, 0);
        range = 0;
        result = FALSE;
        verts[0].set(0, 0, 0);
        verts[1].set(0, 0, 0);
        verts[2].set(0, 0, 0);
    }

    void set(const Fvector& _start, const Fvector& _dir, const float _range, const bool _result)
    {
        start = _start;
        dir = _dir;
        range = _range;
        result = _result;
    }

    bool similar(const Fvector& _start, const Fvector& _dir, const float _range)
    {
        if (!_start.similar(start))
            return false;
        if (!fsimilar(1.f, dir.dotproduct(_dir)))
            return false;
        if (!fsimilar(_range, range))
            return FALSE;
        return TRUE;
    }
};

enum rq_target
{
    rqtNone = (0),
    rqtObject = (1 << 0),
    rqtStatic = (1 << 1),
    rqtShape = (1 << 2),
    rqtObstacle = (1 << 3),
    rqtBoth = (rqtObject | rqtStatic),
    rqtDyn = (rqtObject | rqtShape | rqtObstacle)
};

struct ray_defs
{
    Fvector start;
    Fvector dir;
    float range;
    u32 flags;
    rq_target tgt;

    ray_defs(const Fvector& _start, const Fvector& _dir, float _range, u32 _flags, rq_target _tgt)
    {
        start = _start;
        dir = _dir;
        range = _range;
        flags = _flags;
        tgt = _tgt;
    }
};
struct rq_result
{
    IGameObject* O; // if NULL - static
    float range; // range to intersection
    int element; // номер кости/номер треугольника

    IC rq_result& set(IGameObject* _O, float _range, int _element)
    {
        O = _O;
        range = _range;
        element = _element;
        return *this;
    }

    IC bool set_if_less(CDB::RESULT* I)
    {
        if (I->range < range)
        {
            set(0, I->range, I->id);
            return true;
        }
        else
            return false;
    }
    IC bool set_if_less(rq_result* R)
    {
        if (R->range < range)
        {
            set(R->O, R->range, R->element);
            return TRUE;
        }
        else
            return false;
    }
    IC bool set_if_less(IGameObject* _who, float _range, int _element)
    {
        if (_range < range)
        {
            set(_who, _range, _element);
            return true;
        }
        else
            return false;
    }
    IC bool valid() { return (element >= 0); }
};

using rqVec = xr_vector<rq_result>;

class rq_results
{
protected:
    rqVec results;
    static bool r_sort_pred(const rq_result& a, const rq_result& b) { return a.range < b.range; }

public:
    IC bool append_result(IGameObject* _who, float _range, int _element, bool bNearest)
    {
        if (bNearest && !results.empty())
        {
            rq_result& R = results.back();
            if (_range < R.range)
            {
                R.O = _who;
                R.range = _range;
                R.element = _element;
                return TRUE;
            }
            return FALSE;
        }
        rq_result& rq = results.emplace_back(rq_result());
        rq.range = _range;
        rq.element = _element;
        rq.O = _who;
        return TRUE;
    }

    IC void append_result(rq_result& res)
    {
        if (0 == results.capacity())
            results.reserve(8);
        results.push_back(res);
    }

    IC int r_count() { return results.size(); }
    IC rq_result* r_begin() { return &*results.begin(); }
    //IC rq_result* r_end() { return &*results.end(); }
    IC rqVec* r_get() { return &results; }
    IC void r_clear() { results.clear(); }
    IC void r_sort() { std::sort(results.begin(), results.end(), r_sort_pred); }
    IC rqVec& r_results() { return results; }
};

typedef bool rq_callback(rq_result& result, LPVOID user_data);
typedef bool test_callback(const ray_defs& rd, IGameObject* object, LPVOID user_data);
}
