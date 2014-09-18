//----------------------------------------------------
// file: CEditableObject.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "..\..\..\editors\ECore\Editor\EditObject.h"
#include "..\..\..\editors\ECore\Editor\EditMesh.h"
#include "Bone.h"
#include "Exporter.h"
#include "..\..\Shared\GameMaterial.h"

//----------------------------------------------------------------------------
// Material parsing
//----------------------------------------------------------------------------
BOOL CEditableObject::ExtractTexName(Texmap *src, LPSTR dest)
{
	if( src->ClassID() != Class_ID(BMTEX_CLASS_ID,0) )
		return FALSE;
	BitmapTex *bmap = (BitmapTex*)src;
	_splitpath( bmap->GetMapName(), 0, 0, dest, 0 );
	EFS.AppendFolderToName(dest,1,TRUE);
	return TRUE;
}

//----------------------------------------------------------------------------
BOOL CEditableObject::ParseStdMaterial(StdMat* src, CSurface* dest)
{
	R_ASSERT(src);
	R_ASSERT((src->ClassID()==Class_ID(DMTL_CLASS_ID,0))||(src->ClassID()==XRAYMTL_CLASS_ID));
	Msg("- Processing material '%s' ...", src->GetName() );
	// ------- texture (if exist)
	string1024 tname;
	if( src->MapEnabled( ID_AM ) ){
		if( src->GetSubTexmap( ID_AM ) ){
			if (!ExtractTexName( src->GetSubTexmap( ID_AM ), tname )) return FALSE;
			dest->SetTexture(tname);
		}else{
			return FALSE;
		}
	}else if( src->MapEnabled( ID_DI ) ){
		if( src->GetSubTexmap( ID_DI ) ){
			if (!ExtractTexName( src->GetSubTexmap( ID_DI ), tname )) return FALSE;
			dest->SetTexture(tname);
		}else{
			return FALSE;
		}
	}else{
		return FALSE;
	}
	dest->m_Flags.set(CSurface::sf2Sided,!!src->GetTwoSided());
	if (src->GetTwoSided()) ELog.Msg(mtInformation,"  - material 2-sided");
	dest->SetName(GenerateSurfaceName(src->GetName()));
	dest->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|(1<<D3DFVF_TEXCOUNT_SHIFT));
	dest->SetVMap("Texture");
	dest->SetShader("default");
	dest->SetShaderXRLC("default");
	return TRUE;
}

BOOL CEditableObject::ParseMultiMaterial(MultiMtl* src, u32 mid, CSurface* dest)
{
	R_ASSERT(src);
	R_ASSERT(src->ClassID()==Class_ID(MULTI_CLASS_ID,0));
	Mtl* M = src->GetSubMtl(mid);
	if (M){
		if (M->ClassID()==Class_ID(DMTL_CLASS_ID,0)){
			StdMat *smtl = (StdMat*)src->GetSubMtl(mid);
			if (!ParseStdMaterial(smtl,dest)){
				ELog.Msg(mtError,"'%s' -> bad submaterial",src->GetName());
				return FALSE;
			}
			return TRUE;
		}else if (M->ClassID()==XRAYMTL_CLASS_ID){
			XRayMtl *smtl = (XRayMtl*)src->GetSubMtl(mid);
			if (!ParseXRayMaterial(smtl,mid,dest)){
				ELog.Msg(mtError,"'%s' -> bad submaterial",src->GetName());
				return FALSE;
			}
		} 
		return TRUE;
	}else{
		ELog.Msg(mtError,"'%s' -> can't extract multi-material items.",src->GetName());
		return FALSE;
	}
}

BOOL CEditableObject::ParseXRayMaterial(XRayMtl* src, u32 mid, CSurface* dest)
{
	R_ASSERT(src);
	R_ASSERT(src->ClassID()==XRAYMTL_CLASS_ID);
	if (!ParseStdMaterial((StdMat*)src,dest)){
		ELog.Msg(mtError,"'%s' -> bad material",src->GetName());
		return FALSE;
	}
	LPCSTR e_shader		= src->GetEShaderName();
	LPCSTR c_shader		= src->GetCShaderName();
	LPCSTR g_mtl		= src->GetGameMtlName();
	dest->SetShader		(e_shader?e_shader:"default");
	dest->SetShaderXRLC	(c_shader?c_shader:"default"); 
	dest->SetGameMtl	(g_mtl?g_mtl:"default");
	if (e_shader&&c_shader&&g_mtl){
		ELog.Msg(mtInformation," -Found S.T.A.L.K.E.R. shaders [E:'%s', C:'%s', M:'%s']", dest->_ShaderName(), dest->_ShaderXRLCName(), dest->_GameMtlName() );
	}else{
		if (!e_shader)		ELog.Msg(mtError," *Empty engine shader! Set 'DEFAULT'.");
		if (!c_shader)		ELog.Msg(mtError," *Empty compiler shader! Set 'DEFAULT'.");
		if (!g_mtl)			ELog.Msg(mtError," *Empty game material! Set 'DEFAULT'.");
	}
	return TRUE;
}

CSurface* CEditableObject::CreateSurface(Mtl* mtl, u32 mid)
{
	if (!mtl){ ELog.Msg(mtError,"Empty material..."); return 0;	}
	for (SurfaceIt s_it=m_Surfaces.begin(); s_it!=m_Surfaces.end(); s_it++)
		if (((*s_it)->mid==mid)&&((*s_it)->mtl==mtl)) return *s_it;
	CSurface* S		= xr_new<CSurface>();
	S->mid			= mid;
	S->mtl			= mtl;
	Class_ID cls_id = mtl->ClassID();
	BOOL bRes		= FALSE;
	if (cls_id==Class_ID(MULTI_CLASS_ID,0))		bRes=ParseMultiMaterial	((MultiMtl*)mtl,mid,S);
	else if (cls_id==Class_ID(DMTL_CLASS_ID,0))	bRes=ParseStdMaterial	((StdMat*)mtl,S);
	else if (cls_id==XRAYMTL_CLASS_ID)			bRes=ParseXRayMaterial	((XRayMtl*)mtl,mid,S);
	else{
		ELog.Msg	(mtError,"'%s' -> unsuported material class...",mtl->GetName());
		xr_delete		(S);
		return 0;
	}
	if (!bRes){
		ELog.Msg	(mtError,"'%s' -> can't parse material",mtl->GetName());
		return		 0;
	}
	m_Surfaces.push_back(S);
	return S;
}
//----------------------------------------------------------------------------
// Skeleton functions
//----------------------------------------------------------------------------
bool CEditableObject::ImportMAXSkeleton(CExporter* E)
{
	bool bResult				= true;
	CEditableMesh* MESH			= xr_new<CEditableMesh>(this);
	m_Meshes.push_back(MESH);
	// import mesh
	if (!MESH->Convert(E))		return FALSE;
	// BONES
	m_Bones.reserve(E->m_Bones.size());
	for (int B=0; B!=E->m_Bones.size(); B++){
		m_Bones.push_back(xr_new<CBone>());
		CBone* BONE				= m_Bones.back(); 
		CBoneDef* bone			= E->m_Bones[B];
		CBoneDef* parent_bone	= bone->parent;

		Fvector offset,rotate;
		float length= 0.1f;

		Fmatrix m;
		if (parent_bone)	m.mul(parent_bone->matOffset,bone->matInit);
		else				m.set(bone->matInit);

		m.getXYZi			(rotate);
		offset.set			(m.c);

		BONE->SetWMap		(bone->name.c_str());
		BONE->SetName		(bone->name.c_str());
		BONE->SetParentName	(Helper::ConvertSpace(string(bone->pBone->GetParentNode()->GetName())).c_str());
		BONE->SetRestParams	(length,offset,rotate);
	}

	// DEFAULT BONE PART
	m_BoneParts.push_back(SBonePart());
	SBonePart& BP = m_BoneParts.back();
	BP.alias = "default";
	for (int b_i=0; b_i<(int)m_Bones.size(); b_i++)
		BP.bones.push_back(Bones()[b_i]->Name());

	m_objectFlags.set(CEditableObject::eoDynamic,TRUE);

	if ((0==GetVertexCount())||(0==GetFaceCount())){ 
		bResult = false;
	}else{
		ELog.Msg(mtInformation,"Model '%s' contains: %d points, %d faces, %d bones", E->m_MeshNode->GetName(), GetVertexCount(), GetFaceCount(), Bones().size());
	}

	return bResult;
}
