#include "stdafx.h"
#pragma hdrstop

#include "EDetailModel.h"
#include "Library.h"
#include "EditObject.h"
#include "EditMesh.h"
#include "ImageManager.h"

//------------------------------------------------------------------------------
#define DETOBJ_CHUNK_VERSION		0x1000
#define DETOBJ_CHUNK_REFERENCE 		0x0101                               
#define DETOBJ_CHUNK_SCALE_LIMITS	0x0102
#define DETOBJ_CHUNK_DENSITY_FACTOR	0x0103
#define DETOBJ_CHUNK_FLAGS			0x0104

#define DETOBJ_VERSION 				0x0001
//------------------------------------------------------------------------------
void EDetail::EVertexIn::remapUV(const fvfVertexIn& src, const Fvector2& offs, const Fvector2& scale, bool bRotate)
{
    P.set		(src.P);
    ImageLib.MergedTextureRemapUV(u,v,src.u,src.v, offs, scale, bRotate);
}

EDetail::EDetail()
{
	shader				= 0;
	m_Flags.zero		();
	m_pRefs				= 0;
    m_fMinScale			= 0.5f;
    m_fMaxScale   		= 2.f;
    m_fDensityFactor	= 1.f;
    m_sRefs				= "";
	vertices			= 0;
	number_vertices		= 0;
	indices				= 0;
	number_indices		= 0;
}

EDetail::~EDetail()
{
	Unload();
}

void EDetail::Unload()
{
	CDetail::Unload		();
    Lib.RemoveEditObject(m_pRefs);
    OnDeviceDestroy		();
}

LPCSTR EDetail::GetName	()
{
    return m_pRefs?m_pRefs->GetName():m_sRefs.c_str();
}

LPCSTR EDetail::GetTextureName()
{
	VERIFY(m_pRefs);
    CSurface* surf		= *m_pRefs->FirstSurface(); VERIFY(surf);
    return surf->_Texture();
}

void EDetail::DefferedLoad()
{
}

void EDetail::OnDeviceCreate()
{
	if (!m_pRefs)		return;
    CSurface* surf		= *m_pRefs->FirstSurface();
    VERIFY				(surf);
    AnsiString	s_name	= surf->_ShaderName();
    AnsiString	t_name	= surf->_Texture();
	shader.create		(s_name.c_str(),t_name.c_str());
}

void EDetail::OnDeviceDestroy()
{
	shader.destroy();
}

u16 EDetail::_AddVert(const Fvector& p, float u, float v)
{
	EVertexIn V(p,u,v);
    for (u16 k=0; k<(u16)number_vertices; k++)
    	if (V.similar((EVertexIn&)vertices[k])) return k;
    number_vertices++;
    vertices = (fvfVertexIn*)xr_realloc(vertices,number_vertices*sizeof(fvfVertexIn));
    vertices[number_vertices-1] = V;
    return u16(number_vertices-1);
}

IC BOOL isDegenerated(u16 v[3])
{
	return (v[0]==v[1] || v[0]==v[2] || v[1]==v[2]);
};
IC BOOL isEqual(U16Vec& ind, u16 v[3])
{
	for (U16It it=ind.begin(); it!=ind.end(); it+=3){
        // Test for 6 variations
        if ((*(it+0)==v[0]) && (*(it+1)==v[1]) && (*(it+2)==v[2])) return true;
        if ((*(it+0)==v[0]) && (*(it+2)==v[1]) && (*(it+1)==v[2])) return true;
        if ((*(it+2)==v[0]) && (*(it+0)==v[1]) && (*(it+1)==v[2])) return true;
        if ((*(it+2)==v[0]) && (*(it+1)==v[1]) && (*(it+0)==v[2])) return true;
        if ((*(it+1)==v[0]) && (*(it+0)==v[1]) && (*(it+2)==v[2])) return true;
        if ((*(it+1)==v[0]) && (*(it+2)==v[1]) && (*(it+0)==v[2])) return true;
    }
    return false;
}

bool EDetail::Update	(LPCSTR name)
{
	m_sRefs				= name;
    // update link
    CEditableObject* R	= Lib.CreateEditObject(name);
    if (!R){
 		ELog.Msg		(mtError,"Can't load detail object '%s'.", name);
        return false;
    }
    if(R->SurfaceCount()!=1){
    	ELog.Msg		(mtError,"Object must contain 1 material.");
	    Lib.RemoveEditObject(R);
    	return false;
    }
	if(R->MeshCount()==0){
    	ELog.Msg		(mtError,"Object must contain 1 mesh.");
	    Lib.RemoveEditObject(R);
    	return false;
    }

    Lib.RemoveEditObject(m_pRefs);
    m_pRefs				= R;

    // fill geometry
    CEditableMesh* M	= *m_pRefs->FirstMesh();
    U16Vec inds;

    // fill vertices
    bv_bb.invalidate();
    u32 idx			= 0;
    for (u32 f_id=0; f_id<M->GetFCount(); f_id++)
    {
        st_Face& F 	= M->GetFaces()[f_id];
    	u16 ind[3];
    	for (int k=0; k<3; k++,idx++){
            Fvector& P  = M->GetVertices()[F.pv[k].pindex];
            st_VMapPt&vm= M->GetVMRefs()[F.pv[k].vmref].pts[0];
            Fvector2& uv= M->GetVMaps()[vm.vmap_index]->getUV(vm.index);
        	ind[k]		= _AddVert	(P,uv.x,uv.y);
	        bv_bb.modify(vertices[ind[k]].P);
        }
        if (isDegenerated(ind))	continue;
        if (isEqual(inds,ind))	continue;
        inds.push_back(ind[0]);
        inds.push_back(ind[1]);
        inds.push_back(ind[2]);
    }
	number_indices 		= inds.size();
	indices				= (u16*)xr_malloc(number_indices*sizeof(u16));
    Memory.mem_copy		(indices,inds.begin(),number_indices*sizeof(u16));

	bv_bb.getsphere		(bv_sphere.P,bv_sphere.R);

    OnDeviceCreate		();

    return true;
}

bool EDetail::Load(IReader& F)
{
	// check version
    R_ASSERT			(F.find_chunk(DETOBJ_CHUNK_VERSION));
    u32 version		= F.r_u32();
    if (version!=DETOBJ_VERSION){
    	ELog.Msg(mtError,"EDetail: unsupported version.");
        return false;
    }

	// references
	string256 buf;
    R_ASSERT			(F.find_chunk(DETOBJ_CHUNK_REFERENCE));
    F.r_stringZ			(buf,sizeof(buf));

    // scale
    R_ASSERT			(F.find_chunk(DETOBJ_CHUNK_SCALE_LIMITS));
    m_fMinScale			= F.r_float(); if (fis_zero(m_fMinScale))	m_fMinScale = 0.1f;
	m_fMaxScale			= F.r_float(); if (m_fMaxScale<m_fMinScale)	m_fMaxScale = m_fMinScale;

	// density factor
    if (F.find_chunk(DETOBJ_CHUNK_DENSITY_FACTOR))
	    m_fDensityFactor= F.r_float();

    if (F.find_chunk(DETOBJ_CHUNK_FLAGS))
    	m_Flags.assign	(F.r_u32());

    // update object
    return 				Update(buf);
}

void EDetail::Save(IWriter& F)
{
	// version
	F.open_chunk		(DETOBJ_CHUNK_VERSION);
    F.w_u32				(DETOBJ_VERSION);
    F.close_chunk		();

    // reference
	F.open_chunk		(DETOBJ_CHUNK_REFERENCE);
    F.w_stringZ			(m_sRefs.c_str());
    F.close_chunk		();

	// scale
	F.open_chunk		(DETOBJ_CHUNK_SCALE_LIMITS);
    F.w_float			(m_fMinScale);
    F.w_float			(m_fMaxScale);
    F.close_chunk		();

	// density factor
	F.open_chunk		(DETOBJ_CHUNK_DENSITY_FACTOR);
    F.w_float			(m_fDensityFactor);
    F.close_chunk		();

    // flags
	F.open_chunk		(DETOBJ_CHUNK_FLAGS);
    F.w_u32				(m_Flags.get());
    F.close_chunk		();
}

void EDetail::Export(IWriter& F, LPCSTR tex_name, const Fvector2& offs, const Fvector2& scale, bool rot)
{
	R_ASSERT			(m_pRefs);
    CSurface* surf		= *m_pRefs->FirstSurface();
	R_ASSERT			(surf);
    // write data
	F.w_stringZ			(surf->_ShaderName());
	F.w_stringZ			(tex_name);//surf->_Texture());

    F.w_u32				(m_Flags.get());
    F.w_float			(m_fMinScale);
    F.w_float			(m_fMaxScale);

    F.w_u32				(number_vertices);
    F.w_u32				(number_indices);

    // remap UV
    EVertexIn* rm_vertices = xr_alloc<EVertexIn>(number_vertices);
    for (u32 k=0; k<number_vertices; k++) rm_vertices[k].remapUV(vertices[k],offs,scale,rot);
    
    F.w					(rm_vertices, 	number_vertices*sizeof(fvfVertexIn));
    F.w					(indices, 		number_indices*sizeof(WORD));

    xr_free				(rm_vertices);
}

void EDetail::Export(LPCSTR name)
{
    CSurface* surf		= *m_pRefs->FirstSurface();
	R_ASSERT			(surf);
    IWriter* F 			= FS.w_open(name);
    if (F){
        Fvector2 offs	= {0,0};
        Fvector2 scale	= {1,1};
        Export			(*F,surf->_Texture(),offs,scale,false);
        FS.w_close		(F);
    }else{
        Log				("!Can't export detail:",name);
    }
}

