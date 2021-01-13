#ifndef MXSTDMODEL_INCLUDED // -*- C++ -*-
#define MXSTDMODEL_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  MxStdModel

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxStdModel.h,v 1.35 2000/11/28 15:50:14 garland Exp $

 ************************************************************************/

#include "MxBlockModel.h"

typedef MxSizedDynBlock<unsigned int, 6> MxFaceList;
typedef MxSizedDynBlock<unsigned int, 6> MxVertexList;
typedef MxDynBlock<MxEdge> MxEdgeList;

class MxPairContraction
{
public:
    MxPairContraction() {}
    MxPairContraction(const MxPairContraction& c) { *this = c; }
    MxPairContraction& operator=(const MxPairContraction& c);

    MxVertexID v1, v2;
    float dv1[3], dv2[3]; // dv2 is not really necessary

    unsigned int delta_pivot;
    MxFaceList delta_faces;
    MxFaceList dead_faces;
};

class MxFaceContraction
{
public:
    MxFaceID f;
    float dv1[3], dv2[3], dv3[3];

    MxFaceList delta_faces;
    MxFaceList dead_faces;
};

typedef MxPairContraction MxPairExpansion;

// Masks for internal tag bits
#define MX_VALID_FLAG 0x01
#define MX_PROXY_FLAG 0x02
#define MX_TOUCHED_FLAG 0x04
#define MX_LOCK_FLAG 0x08

class MxStdModel : public MxBlockModel
{
private:
    struct vertex_data
    {
        unsigned char mark, tag; // Internal tag bits
        unsigned char user_mark, user_tag; // External tag bits
    };
    struct face_data
    {
        unsigned char mark, tag; // Internal tag bits
        unsigned char user_mark, user_tag; // External tag bits
    };

    MxDynBlock<vertex_data> v_data;
    MxDynBlock<face_data> f_data;
    MxDynBlock<MxFaceList*> face_links;

protected:
    //
    // Accessors for internal tag and mark bits
    unsigned int v_check_tag(MxVertexID i, unsigned int tag) const { return v_data(i).tag & tag; }
    void v_set_tag(MxVertexID i, unsigned int tag) { v_data(i).tag |= tag; }
    void v_unset_tag(MxVertexID i, unsigned int tag) { v_data(i).tag &= ~tag; }
    unsigned char vmark(MxVertexID i) const { return v_data(i).mark; }
    void vmark(MxVertexID i, unsigned char m) { v_data(i).mark = m; }
    unsigned int f_check_tag(MxFaceID i, unsigned int tag) const { return f_data(i).tag & tag; }
    void f_set_tag(MxFaceID i, unsigned int tag) { f_data(i).tag |= tag; }
    void f_unset_tag(MxFaceID i, unsigned int tag) { f_data(i).tag &= ~tag; }
    unsigned char fmark(MxFaceID i) const { return f_data(i).mark; }
    void fmark(MxFaceID i, unsigned char m) { f_data(i).mark = m; }
protected:
    MxVertexID alloc_vertex(float, float, float);
    void free_vertex(MxVertexID);
    void free_face(MxFaceID);
    MxFaceID alloc_face(MxVertexID, MxVertexID, MxVertexID);
    void init_face(MxFaceID);

public:
    MxStdModel(unsigned int nvert, unsigned int nface)
        : MxBlockModel(nvert, nface), v_data(nvert), f_data(nface), face_links(nvert)
    {
    }
    virtual ~MxStdModel();
    MxStdModel* clone();

    ////////////////////////////////////////////////////////////////////////
    //  Tagging and marking
    //
    unsigned int vertex_is_valid(MxVertexID i) const { return v_check_tag(i, MX_VALID_FLAG); }
    void vertex_mark_valid(MxVertexID i) { v_set_tag(i, MX_VALID_FLAG); }
    void vertex_mark_invalid(MxVertexID i) { v_unset_tag(i, MX_VALID_FLAG); }
    unsigned int vertex_is_locked(MxVertexID i) const { return v_check_tag(i, MX_LOCK_FLAG); }
    void vertex_mark_locked(MxVertexID i) { v_set_tag(i, MX_LOCK_FLAG); }
    void vertex_mark_unlocked(MxVertexID i) { v_unset_tag(i, MX_LOCK_FLAG); }
    unsigned int face_is_valid(MxFaceID i) const { return f_check_tag(i, MX_VALID_FLAG); }
    void face_mark_valid(MxFaceID i) { f_set_tag(i, MX_VALID_FLAG); }
    void face_mark_invalid(MxFaceID i) { f_unset_tag(i, MX_VALID_FLAG); }
    unsigned int face_is_locked(MxFaceID i) const { return f_check_tag(i, MX_LOCK_FLAG); }
    void face_mark_locked(MxFaceID i) { f_set_tag(i, MX_LOCK_FLAG); }
    void face_mark_unlocked(MxFaceID i) { f_unset_tag(i, MX_LOCK_FLAG); }
    unsigned int vertex_is_proxy(MxVertexID i) const { return v_check_tag(i, MX_PROXY_FLAG); }
    void vertex_mark_proxy(MxVertexID i) { v_set_tag(i, MX_PROXY_FLAG); }
    void vertex_mark_nonproxy(MxVertexID i) { v_unset_tag(i, MX_PROXY_FLAG); }
    //
    // Accessors for external tag and mark bits
    unsigned int vertex_check_tag(MxVertexID i, unsigned int tag) const { return v_data(i).user_tag & tag; }
    void vertex_set_tag(MxVertexID i, unsigned int tag) { v_data(i).user_tag |= tag; }
    void vertex_unset_tag(MxVertexID i, unsigned int tag) { v_data(i).user_tag &= ~tag; }
    unsigned char vertex_mark(MxVertexID i) { return v_data(i).user_mark; }
    void vertex_mark(MxVertexID i, unsigned char m) { v_data(i).user_mark = m; }
    unsigned int face_check_tag(MxFaceID i, unsigned int tag) const { return f_data(i).user_tag & tag; }
    void face_set_tag(MxFaceID i, unsigned int tag) { f_data(i).user_tag |= tag; }
    void face_unset_tag(MxFaceID i, unsigned int tag) { f_data(i).user_tag &= ~tag; }
    unsigned char face_mark(MxFaceID i) { return f_data(i).user_mark; }
    void face_mark(MxFaceID i, unsigned char m) { f_data(i).user_mark = m; }
    ////////////////////////////////////////////////////////////////////////
    //  Vertex proxy management and proxy-aware accessors
    //
    MxVertexID add_proxy_vertex(MxVertexID);
    MxVertexID& vertex_proxy_parent(MxVertexID);
    MxVertexID resolve_proxies(MxVertexID v);
    float* vertex_position(MxVertexID v);

    ////////////////////////////////////////////////////////////////////////
    //  Neighborhood collection and management
    //
    void mark_neighborhood(MxVertexID, unsigned short mark = 0);
    void collect_unmarked_neighbors(MxVertexID, MxFaceList& faces);
    void mark_neighborhood_delta(MxVertexID, short delta);
    void partition_marked_neighbors(MxVertexID, unsigned short pivot, MxFaceList& below, MxFaceList& above);

    void mark_corners(const MxFaceList& faces, unsigned short mark = 0);
    void collect_unmarked_corners(const MxFaceList& faces, MxVertexList& verts);

    void collect_edge_neighbors(MxVertexID, MxVertexID, MxFaceList&);
    void collect_vertex_star(MxVertexID v, MxVertexList& verts);

    MxFaceList& neighbors(MxVertexID v) { return *face_links(v); }
    const MxFaceList& neighbors(MxVertexID v) const { return *face_links(v); }
    void collect_neighborhood(MxVertexID v, int depth, MxFaceList& faces);

    void compute_vertex_normal(MxVertexID v, float*);
    void synthesize_normals(unsigned int start = 0);

    ////////////////////////////////////////////////////////////////////////
    // Primitive transformation operations
    //
    void remap_vertex(MxVertexID from, MxVertexID to);
    MxVertexID split_edge(MxVertexID v1, MxVertexID v2, float x, float y, float z);
    MxVertexID split_edge(MxVertexID v1, MxVertexID v2);

    void flip_edge(MxVertexID v1, MxVertexID v2);

    // split_face3
    void split_face4(MxFaceID f, MxVertexID* newverts = NULL);

    void unlink_face(MxFaceID f);

    ////////////////////////////////////////////////////////////////////////
    // Contraction and related operations
    //
    void compact_vertices();
    void remove_degeneracy(MxFaceList&);

    // Pair contraction interface
    void compute_contraction(MxVertexID, MxVertexID, MxPairContraction*, const float* vnew = NULL);
    void apply_contraction(const MxPairContraction&);
    void apply_expansion(const MxPairExpansion&);
    void contract(MxVertexID v1, MxVertexID v2, const float*, MxPairContraction*);

    // Triple contraction interface
    void compute_contraction(MxFaceID, MxFaceContraction*);
    void contract(MxVertexID v1, MxVertexID v2, MxVertexID v3, const float* vnew, MxFaceList& changed);

    // Generalized contraction interface
    void contract(MxVertexID v1, const MxVertexList& rest, const float* vnew, MxFaceList& changed);
};

extern void mx_render_model(MxStdModel&);
extern void mx_draw_mesh(MxStdModel&, double* color = NULL);
extern void mx_draw_wireframe(MxStdModel&, double* color = NULL);
extern void mx_draw_boundaries(MxStdModel&);
extern void mx_draw_pointcloud(MxStdModel&, double* color = NULL);

// MXSTDMODEL_INCLUDED
#endif
