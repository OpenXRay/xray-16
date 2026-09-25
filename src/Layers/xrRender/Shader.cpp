// Shader.cpp: implementation of the CShader class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/ResourceManager.h"

namespace xray::render::fg
{
SGeometry::~SGeometry() { RImplementation.Resources->DeleteGeom(this); }

//////////////////////////////////////////////////////////////////////////
void resptrcode_geom::create(u32 FVF, VertexBufferHandle vb, IndexBufferHandle ib)
{
    _set(RImplementation.Resources->CreateGeom(FVF, vb, ib));
}

void resptrcode_geom::create(const VertexElement* decl, VertexBufferHandle vb, IndexBufferHandle ib)
{
    _set(RImplementation.Resources->CreateGeom(decl, vb, ib));
}

void resptrcode_geom::create_logical(const VertexElement* decl)
{
    _set(RImplementation.Resources->CreateLogicalGeom(decl));
}
} // namespace xray::render::fg
