#pragma once

struct DataVertex;
template <typename DataVertexType>
struct Tface;
using Face = Tface<DataVertex>;

template <typename DataVertexType>
struct Tvertex;
using Vertex = Tvertex<DataVertex>;

using vecVertex =  xr_vector<Vertex*>;
using vecVertexIt = vecVertex::iterator;

using vecFace = xr_vector<Face*> ;
using vecFaceIt = vecFace::iterator ;
using vecFaceCit = vecFace::const_iterator;

using vec2Face = xr_vector<vecFace*>;
using splitIt = vec2Face::iterator;

using vecAdj = vecFace;
using vecAdjIt = vecAdj::iterator;
