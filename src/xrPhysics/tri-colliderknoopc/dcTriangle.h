#pragma once

// XXX: maybe update ODE to a newer version
// The warning happens on lines 23-24 when assigning to -dInfinity
#pragma warning(disable : 4756)

struct Triangle
{
    // dReal* v0;
    // dReal* v1;
    // dReal* v2;
    dVector3 side0;
    dVector3 side1;
    dVector3 norm;
    dReal dist;
    dReal pos;
    dReal depth;
    CDB::TRI* T;
    Triangle()
    {
        T = NULL;
#ifdef DEBUG
        depth = -dInfinity;
        dist = -dInfinity;
#endif
    }
};

#pragma warning(pop)
