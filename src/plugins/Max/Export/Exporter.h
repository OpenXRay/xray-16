#pragma once

#include "..\..\Shared\helper.h"
#include "..\..\Shared\bonedef.h"
#include "..\..\Shared\face.h"

DEFINE_VECTOR(INode*,INodeVec,INodeIt);

//-----------------------------------------------------------------------------
IC void ERR(LPCSTR s, LPCSTR dop="") 
{ 
	Msg("!Error: %s%s",s,dop);
}
IC int CGINTM(INode* node, int r)
{
	char* msg = 0;
	switch (r) {
	case MATRIX_RETURNED:		return r;
	case NODE_NOT_FOUND:		msg = "NODE_NOT_FOUND"; break;
	case NO_MATRIX_SAVED:		msg = "NO_MATRIX_SAVED"; break;
	case INVALID_MOD_POINTER:	msg = "INVALID_MOD_POINTER"; break;
	}
	if (msg)
		Msg("* '%s': GetInitNodeTM failed (%s)",node->GetName(),msg);
	return r;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
class CExporter  
{
	enum EStyle{
		eExportUndef,
		eExportSkin,
		eExportMotion
	};
	EStyle			m_Style;
	BOOL			m_bHasError;
public:
	BOOL			m_bFindMesh;
	INodeVec		m_AllBones;
	BoneDefVec		m_Bones;
	VertexDefVec	m_Vertices;

	Mtl*			m_MtlMain;
	INode*			m_MeshNode;

	// prepared 
	ExpVertVec		m_ExpVertices;
	ExpFaceVec		m_ExpFaces;

	// global scale
	float			m_fGlobalScale;
private:
	void			ScanBones	(INode *pNode);
	void			ScanMesh	(INode *pNode);
	BOOL			Capture		();
	int				AddVert		(const st_VERT& pTestV)
	{
		for (ExpVertIt vI=m_ExpVertices.begin(); vI!=m_ExpVertices.end(); vI++)
			if ((*vI)->similar(pTestV)) return vI-m_ExpVertices.begin();

		st_VERT* V	= xr_new<st_VERT>();
		*V			= pTestV;
		m_ExpVertices.push_back(V);
		return m_ExpVertices.size()-1;
	}
public:
	//-----------------------------------------------------------------------------
	CVertexDef*		AddVertex	()
	{
		CVertexDef* V = xr_new<CVertexDef>();
		m_Vertices.push_back(V);
		return V;
	}
	//-----------------------------------------------------------------------------
	CBoneDef*		FindBone	(LPCSTR name)
	{
		if (name&&name[0]){
			string nm = Helper::ConvertSpace(string(name));
			for (BoneDefIt it=m_Bones.begin(); it!=m_Bones.end(); it++)
				if ((*it)->name==nm) return *it;
			return 0;
		}else
			return 0;
	}
	void			UpdateParenting()
	{
		for (BoneDefIt it=m_Bones.begin(); it!=m_Bones.end(); it++)
			(*it)->parent = FindBone((*it)->GetParentName());
	}
	int				AddBone(INode* pNode, Matrix3 &matMesh, IPhysiqueExport* pExport);
public:
					CExporter	(){	m_bHasError=FALSE; m_MtlMain=0; m_Style=eExportUndef; m_bFindMesh = FALSE; m_fGlobalScale=1.f; m_MeshNode=0;};
	virtual			~CExporter	();
	BOOL			ExportSkin	(INode *pNode, LPCSTR fname);
	BOOL			ExportMotion(INode *pNode, LPCSTR fname);
};
