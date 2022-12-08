#ifndef MXBLOCKMODEL_INCLUDED // -*- C++ -*-
#define MXBLOCKMODEL_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  MxBlockModel

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxBlockModel.h,v 1.24 2000/02/03 17:32:21 garland Exp $

 ************************************************************************/

#include "MxDynBlock.h"
#include "MxGeoPrims.h"

#define MX_UNBOUND 0x0
#define MX_PERFACE 0x1
#define MX_PERVERTEX 0x2
#define MX_MAX_BINDING 0x2

#define MX_NORMAL_MASK 0x3
#define MX_COLOR_MASK (0x3 << 2)
#define MX_TEXTURE_MASK (0x3 << 4)
#define MX_ALL_MASK (MX_NORMAL_MASK | MX_COLOR_MASK | MX_TEXTURE_MASK)

class MxBlockModel
{
private:
    unsigned char cbinding, nbinding, tbinding;

    unsigned int flags;

    // Required blocks
    MxDynBlock<MxVertex> vertices;
    MxDynBlock<MxFace> faces;

    // Optional blocks
    MxDynBlock<MxNormal>* normals;
    MxDynBlock<MxColor>* colors;
    MxDynBlock<MxTexCoord>* tcoords;
    // prop_block *properties;  // Indirect block for arbitrary properties

    // Optional texture map
    char* tex_name;

protected:
    virtual MxVertexID alloc_vertex(float, float, float);
    virtual void init_vertex(MxVertexID) {}
    virtual void free_vertex(MxVertexID) {}
    virtual MxFaceID alloc_face(MxVertexID, MxVertexID, MxVertexID);
    virtual void init_face(MxFaceID) {}
    virtual void free_face(MxFaceID) {}
public:
    unsigned int binding_mask;

public:
    MxBlockModel(int nvert, int nface) : vertices(nvert), faces(nface)
    {
        colors = NULL;
        normals = NULL;
        tcoords = NULL;
        cbinding = nbinding = tbinding = MX_UNBOUND;
        binding_mask = MX_ALL_MASK;
        tex_name = NULL;
    }
    virtual ~MxBlockModel()
    {
        if (normals)
            xr_delete(normals);
        if (colors)
            xr_delete(colors);
        if (tcoords)
            xr_delete(tcoords);
        if (tex_name)
            xr_free(tex_name);
    }

    MxBlockModel* clone(MxBlockModel* into = NULL);

    unsigned int vert_count() const { return vertices.length(); }
    unsigned int face_count() const { return faces.length(); }
    unsigned int color_count() const { return (colors ? colors->length() : 0); }
    unsigned int normal_count() const { return (normals ? normals->length() : 0); }
    unsigned int texcoord_count() const { return (tcoords ? tcoords->length() : 0); }
    MxVertexID add_vertex(float, float, float);
    MxFaceID add_face(unsigned int, unsigned int, unsigned int, bool will_link = true);
    unsigned int add_color(float, float, float);
    unsigned int add_normal(float, float, float);
    unsigned int add_texcoord(float, float);

    MxVertexID add_vertex(float* v) { return add_vertex(v[0], v[1], v[2]); }
    MxFaceID add_face(unsigned int* f) { return add_face(f[0], f[1], f[2]); }
    void remove_vertex(MxVertexID v);
    void remove_face(MxFaceID f);

    MxVertex& vertex(unsigned int i) { return vertices(i); }
    MxFace& face(unsigned int i) { return faces(i); }
    MxVertex& corner(MxFaceID f, short i) { return vertex(face(f)[i]); }
    MxColor& color(unsigned int i)
    {
        VERIFY(colors);
        return (*colors)(i);
    }
    MxNormal& normal(unsigned int i)
    {
        VERIFY(normals);
        return (*normals)(i);
    }
    MxTexCoord& texcoord(unsigned int i)
    {
        VERIFY(tcoords);
        return (*tcoords)(i);
    }

    int color_binding() { return (cbinding & (binding_mask >> 2)); }
    int normal_binding() { return (nbinding & binding_mask); }
    int texcoord_binding() { return (tbinding & (binding_mask >> 4)); }
    void color_binding(unsigned char b);
    void normal_binding(unsigned char b);
    void texcoord_binding(unsigned char b);

    const char* binding_name(int);
    int parse_binding(const char*);

    const char* texmap_name() const { return tex_name; }
    unsigned int add_texmap(const char* name);

    void compute_face_normal(MxFaceID, double*, bool will_unitize = true);
    void compute_face_normal(MxFaceID, float*, bool will_unitize = true);
    void compute_face_plane(MxFaceID, float*, bool will_unitize = true);
    double compute_face_area(MxFaceID);
    double compute_face_perimeter(MxFaceID, bool* edge_flags = NULL);

    double compute_corner_angle(MxFaceID, unsigned int);
};

// MXBLOCKMODEL_INCLUDED
#endif
