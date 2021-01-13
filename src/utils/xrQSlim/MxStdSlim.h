#ifndef MXSTDSLIM_INCLUDED // -*- C++ -*-
#define MXSTDSLIM_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  Core simplification interface.  The MxStdSlim class defines the
  interface which all simplification classes conform to.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxStdSlim.h,v 1.4 1998/11/19 01:57:34 garland Exp $

 ************************************************************************/

#include "MxStdModel.h"
#include "MxHeap.h"

#define MX_PLACE_ENDPOINTS 0
#define MX_PLACE_ENDORMID 1
#define MX_PLACE_LINE 2
#define MX_PLACE_OPTIMAL 3

#define MX_WEIGHT_UNIFORM 0
#define MX_WEIGHT_AREA 1
#define MX_WEIGHT_ANGLE 2
#define MX_WEIGHT_AVERAGE 3
#define MX_WEIGHT_AREA_AVG 4
#define MX_WEIGHT_RAWNORMALS 5

#define EDGE_BASE_ERROR 1.f

class MxStdSlim
{
protected:
    MxStdModel* m;
    MxHeap heap;

public:
    unsigned int valid_verts;
    unsigned int valid_faces;
    bool is_initialized;

    int placement_policy;
    int weighting_policy;
    bool will_join_only;

    double boundary_weight;
    double compactness_ratio;
    double meshing_penalty;
    double local_validity_threshold;
    unsigned int vertex_degree_limit;

public:
    MxStdSlim(MxStdModel* m0);

    virtual void initialize() = 0;
    virtual bool decimate(unsigned int, float max_error, void* cb_params = 0) = 0;

    MxStdModel& model() { return *m; }
public:
    void (*contraction_callback)(const MxPairContraction&, float, void*);
};

// MXSTDSLIM_INCLUDED
#endif
