// xrSKIN_BUILD.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "Exporter.h"
#include "MeshExpUtility.h"
#include "..\..\..\editors\ECore\Editor\EditObject.h"
#include "..\..\..\editors\ECore\Editor\EditMesh.h"
#include "Bone.h"
#include "Motion.h"
#include "Envelope.h"

BOOL CExporter::ExportMotion(INode* pNode, LPCSTR fname)
{
	m_Style				= eExportMotion;
	ScanBones			(pNode);
	int i				= U.ip->GetSelNodeCount(); 
	while (i--)			ScanMesh(U.ip->GetSelNode(i));
	if (!Capture())		return FALSE;
	if (m_bHasError)	return FALSE;

	int iFPS			= GetFrameRate();
	int iTPF			= GetTicksPerFrame();
	int iStartTick		= U.ip->GetAnimRange().Start();
	int iEndTick		= U.ip->GetAnimRange().End();

	// build motion
	CSMotion* MOT		= xr_new<CSMotion>();
	MOT->SetParam		(iStartTick/iTPF,iEndTick/iTPF,(float)iFPS);
	string64 nm;		_splitpath(fname,0,0,nm,0);
	MOT->SetName		(nm);

	BoneMotionVec& BMVec= MOT->BoneMotions();
	BMVec.reserve		(m_Bones.size());

	EConsole.ProgressStart((float)m_Bones.size(),"..Exporting per bone motions");
	Fmatrix tmBone;
	for(int boneId = 0; boneId < int(m_Bones.size()); boneId++){
		CBoneDef* bone	= m_Bones[boneId];
		//Log("Bone: ",bone->name.c_str());

		BMVec.push_back(st_BoneMotion());
		st_BoneMotion& BM = BMVec.back();
		BM.SetName(bone->name.c_str());

		BM.envs[ctPositionX] = xr_new<CEnvelope>();
		BM.envs[ctPositionY] = xr_new<CEnvelope>();
		BM.envs[ctPositionZ] = xr_new<CEnvelope>();
		BM.envs[ctRotationH] = xr_new<CEnvelope>();
		BM.envs[ctRotationP] = xr_new<CEnvelope>();
		BM.envs[ctRotationB] = xr_new<CEnvelope>();

		BM.envs[ctPositionX]->behavior[0]=1; BM.envs[ctPositionX]->behavior[1]=1;
		BM.envs[ctPositionY]->behavior[0]=1; BM.envs[ctPositionY]->behavior[1]=1;
		BM.envs[ctPositionZ]->behavior[0]=1; BM.envs[ctPositionZ]->behavior[1]=1;
		BM.envs[ctRotationH]->behavior[0]=1; BM.envs[ctRotationH]->behavior[1]=1;
		BM.envs[ctRotationP]->behavior[0]=1; BM.envs[ctRotationP]->behavior[1]=1;
		BM.envs[ctRotationB]->behavior[0]=1; BM.envs[ctRotationB]->behavior[1]=1;
 
		TimeValue tick;
		st_Key *X,*Y,*Z,*H,*P,*B;
		for(tick = iStartTick; tick < iEndTick; tick+=iTPF)
		{
			X = xr_new<st_Key>();	Y = xr_new<st_Key>();	Z = xr_new<st_Key>();
			H = xr_new<st_Key>();	P = xr_new<st_Key>();	B = xr_new<st_Key>();
			BM.envs[ctPositionX]->keys.push_back(X);	BM.envs[ctPositionY]->keys.push_back(Y);	BM.envs[ctPositionZ]->keys.push_back(Z);
			BM.envs[ctRotationH]->keys.push_back(H);	BM.envs[ctRotationP]->keys.push_back(P);	BM.envs[ctRotationB]->keys.push_back(B);
			// get the bone transformation matrix
			bone->CalculateLocalMatrix(tick,tmBone); 

			float displacedTime = (float)(tick)/(float)(iTPF*iFPS);

			float h,p,b, x,y,z;
			tmBone.getHPB(h,p,b);

			x=tmBone.c.x; y=tmBone.c.y; z=tmBone.c.z;
			X->time = displacedTime;	Y->time = displacedTime;	Z->time = displacedTime;
			H->time = displacedTime;	P->time = displacedTime;	B->time = displacedTime;
			X->shape = 4;	Y->shape = 4;	Z->shape = 4; 
			H->shape = 4;	P->shape = 4;	B->shape = 4;
			X->value = x;	Y->value = y;	Z->value = z;
			H->value =-h;	P->value =-p;	B->value =-b;
		}
		EConsole.ProgressInc();
	}
	EConsole.ProgressEnd();
	MOT->SaveMotion(fname);
	xr_delete(MOT);
	return TRUE;
}
