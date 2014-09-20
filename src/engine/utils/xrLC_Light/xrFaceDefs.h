#pragma once

struct DataVertex;
template <typename DataVertexType> struct Tface;
typedef	Tface<DataVertex>	Face;

template <typename DataVertexType> struct Tvertex;
typedef	Tvertex<DataVertex>	Vertex;

typedef xr_vector<Vertex*>			vecVertex;
typedef vecVertex::iterator			vecVertexIt;

typedef xr_vector<Face*>			vecFace;
typedef vecFace::iterator			vecFaceIt;
typedef vecFace::const_iterator		vecFaceCit;

typedef xr_vector<vecFace*>			vec2Face;
typedef vec2Face::iterator			splitIt;


typedef vecFace						vecAdj;
typedef vecAdj::iterator			vecAdjIt;

