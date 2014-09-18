#pragma once

#include "..\..\Shared\helper.h"

// refs
class CExporter;

class CBoneDef
{
public:
	CBoneDef*	parent;
	string		name;
	INode*		pBone;

	Fmatrix		matInit;
	Fmatrix		matOffset;
public:
				CBoneDef(INode* pNode);
	IC BOOL		isEqual(INode* pNode){ return	pBone==pNode; }
	LPCSTR		GetParentName();
	BOOL		SetInitTM(IPhysiqueExport* pExport, const Matrix3& matMesh);
	void		CalculateLocalMatrix(TimeValue tick, Fmatrix& mat)
	{
		Fmatrix m;
		Helper::ConvertMatrix(pBone->GetNodeTM(tick),m);
		if (parent){
			Fmatrix i_m;
			Helper::ConvertMatrix(parent->pBone->GetNodeTM(tick),i_m);
			i_m.invert();
			mat.mul(i_m,m);
		}else{
			mat.set(m);
		}
	}
};
DEFINE_VECTOR(CBoneDef*,BoneDefVec,BoneDefIt);