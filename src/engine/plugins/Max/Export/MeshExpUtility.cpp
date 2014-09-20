// file: MeshExpUtility.cpp

#include "stdafx.h"
#pragma hdrstop

#include "MeshExpUtility.h"
#include "..\..\..\xrCore\FileSystem.h"
#include "MeshExpUtility.rh"
#include "..\..\..\editors\ECore\Editor\EditObject.h"
#include "..\..\..\editors\ECore\Editor\EditMesh.h"
//-------------------------------------------------------------------
//  Dialog Handler for Utility


static BOOL CALLBACK DefaultDlgProc(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {

		case WM_INITDIALOG:
			U.Init(hWnd);
			break;

		case WM_DESTROY:
			U.Destroy(hWnd);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_CLOSE:
					U.iu->CloseUtility();
					break;
				case IDC_EXPORT_AS_OBJECT:
					U.ExportObject();
					break;
				case IDC_EXPORT_AS_LWO:
					U.ExportLWO();
					break;
				case IDC_SKIN_EXPORT:
					U.ExportSkin();
					break;
				case IDC_SKIN_KEYS_EXPORT:
					U.ExportSkinKeys();
					break;
			}
			break;


		default:
			return FALSE;
	}
	return TRUE;
}

//-------------------------------------------------------------------
//     Utility implimentations

MeshExpUtility::MeshExpUtility()
{
	iu = 0;
	ip = 0;	
	hPanel = 0;

	m_ObjectFlipFaces			= false;
	m_SkinFlipFaces				= false;
	m_SkinAllowDummy			= false;
}

MeshExpUtility::~MeshExpUtility()
{
}


void MeshExpUtility::BeginEditParams(Interface *ip,IUtil *iu) 
{
	this->iu = iu;
	this->ip = ip;
	EConsole.Init( hInstance, 0 );

	hPanel = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_MWND), DefaultDlgProc, "S.T.A.L.K.E.R. Export", 0);
}
	
void MeshExpUtility::EndEditParams(Interface *ip,IUtil *iu) 
{
	m_ObjectFlipFaces			= IsDlgButtonChecked( hPanel, IDC_OBJECT_FLIPFACES );
	m_SkinFlipFaces				= IsDlgButtonChecked( hPanel, IDC_SKIN_FLIPFACES );
	m_SkinAllowDummy			= IsDlgButtonChecked( hPanel, IDC_SKIN_ALLOW_DUMMY );

	EConsole.Clear();
	
	this->iu = 0;
	this->ip = 0;
	ip->DeleteRollupPage(hPanel);
	hPanel = 0;
}

void MeshExpUtility::SelectionSetChanged(Interface *ip,IUtil *iu)
{
	RefreshExportList();
	UpdateSelectionListBox();
}

void MeshExpUtility::Init(HWND hWnd)
{
	hPanel = hWnd;

	hItemList = GetDlgItem(hWnd, IDC_OBJLIST);

	CheckDlgButton( hPanel, IDC_OBJECT_FLIPFACES,				m_ObjectFlipFaces );
	CheckDlgButton( hPanel, IDC_SKIN_FLIPFACES,					m_SkinFlipFaces );
	CheckDlgButton( hPanel, IDC_SKIN_ALLOW_DUMMY,				m_SkinAllowDummy );

	RefreshExportList();
	UpdateSelectionListBox();
}

void MeshExpUtility::Destroy(HWND hWnd)
{
}

void MeshExpUtility::RefreshExportList(){
	m_Items.clear();
	ExportItem item;
	int i = ip->GetSelNodeCount();
	while( i-- ){
		item.pNode = ip->GetSelNode(i);
		m_Items.push_back( item );
	}
}

void MeshExpUtility::UpdateSelectionListBox()
{
	SendMessage( hItemList, LB_RESETCONTENT, 0, 0 );
	ExportItemIt it = m_Items.begin();
	for(;it!=m_Items.end();it++)
		SendMessage( hItemList, LB_ADDSTRING,
			0,(LPARAM) it->pNode->GetName() );
}
//-------------------------------------------------------------------

BOOL MeshExpUtility::BuildObject(CEditableObject*& exp_obj, LPCSTR m_ExportName)
{
	bool bResult = true;

	if (m_ExportName[0]==0) return false;

	ELog.Msg(mtInformation,"Building object..." );
	char fname[256]; _splitpath( m_ExportName, 0, 0, fname, 0 );
	exp_obj = xr_new<CEditableObject>(fname);	
	exp_obj->SetVersionToCurrent(TRUE,TRUE);

	ExportItemIt it = m_Items.begin();
	for(;it!=m_Items.end();it++){
		CEditableMesh *submesh = xr_new<CEditableMesh>(exp_obj);
		ELog.Msg(mtInformation,"Converting node '%s'...", it->pNode->GetName());
		if( submesh->Convert(it->pNode) ){
			// transform
			Matrix3 mMatrix;
			mMatrix = it->pNode->GetNodeTM(0)*Inverse(it->pNode->GetParentNode()->GetNodeTM(0));

			Point3	r1	= mMatrix.GetRow(0);
			Point3	r2	= mMatrix.GetRow(1);
			Point3	r3	= mMatrix.GetRow(2);
			Point3	r4	= mMatrix.GetRow(3);
			Fmatrix m;	m.identity();
			m.i.set(r1.x, r1.z, r1.y);
			m.j.set(r2.x, r2.z, r2.y);
			m.k.set(r3.x, r3.z, r3.y);
			m.c.set(r4.x, r4.z, r4.y);
			
			submesh->Transform( m );
			// flip faces
			Fvector v; v.crossproduct(m.j, m.k);
			if(v.dotproduct(m.i)<0.f)	submesh->FlipFaces();
			if(m_ObjectFlipFaces)		submesh->FlipFaces();
			submesh->RecomputeBBox();
			// append mesh
			submesh->SetName			(it->pNode->GetName());
			exp_obj->m_Meshes.push_back	(submesh);
		}else{
			ELog.Msg(mtError,"! can't convert", it->pNode->GetName());
			xr_delete(submesh);
			bResult = false;
			break;
		}
	}

	if (bResult){
		exp_obj->UpdateBox		();
		exp_obj->VerifyMeshNames();
		ELog.Msg				(mtInformation,"Object '%s' contains: %d points, %d faces",
								exp_obj->GetName(), exp_obj->GetVertexCount(), exp_obj->GetFaceCount());
	}else{
		xr_delete(exp_obj);
	}
//-------------------------------------------------------------------
	return bResult;
}
//-------------------------------------------------------------------

BOOL MeshExpUtility::SaveAsObject(const char* m_ExportName)
{
	BOOL bResult = TRUE;

	if (m_ExportName[0]==0) return FALSE;

	ELog.Msg(mtInformation,"Exporting..." );
	ELog.Msg(mtInformation,"-------------------------------------------------------" );
	CEditableObject* exp_obj=0;	
	if (bResult=BuildObject(exp_obj,m_ExportName)){
		ELog.Msg				(mtInformation,"Saving object...");
		for (SurfaceIt s_it=exp_obj->FirstSurface(); s_it!=exp_obj->LastSurface(); s_it++){
			LPSTR t=(LPSTR)(*s_it)->_Texture();
			if (strext(t)) *strext(t)=0;
		}
		exp_obj->Optimize		();
		exp_obj->SaveObject		(m_ExportName);
	}

	xr_delete(exp_obj);

	return bResult;
}
//-------------------------------------------------------------------

BOOL MeshExpUtility::SaveAsLWO(const char* m_ExportName)
{
	BOOL bResult = TRUE;

	if (m_ExportName[0]==0) return FALSE;

	ELog.Msg(mtInformation,"Exporting..." );
	ELog.Msg(mtInformation,"-------------------------------------------------------" );
	CEditableObject* exp_obj=0;	
	if (bResult=BuildObject(exp_obj,m_ExportName)){
		ELog.Msg				(mtInformation,"Object saved...");
		exp_obj->Optimize		();
		exp_obj->ExportLWO		(m_ExportName);
	}

	xr_delete(exp_obj);

	return bResult;
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
//     registry access
const char *g_rOptionsKey		= "SOFTWARE\\NerGoSoft\\S.T.A.L.K.E.R. Map Editor\\2.0\\Options";
const char *g_rPathValue		= "MainPath";
const char *g_rObjFlipFacesVal	= "Object Flip Faces";
const char *g_rObjSuppressSMVal	= "Object Suppress SM";
const char *g_rNoOptimizeVal	= "Object No Optimize";
const char *g_rSkinSuppressSMVal= "Skin Suppress SM";
const char *g_rSkinProgressive	= "Skin Progressive";

//-------------------------------------------------------------------------------------------------
void MeshExpUtility::ExportObject()
{
	BOOL bResult = FALSE;

	if( m_Items.empty() ){
		ELog.Msg(mtError,"Nothing selected" );
		ELog.Msg(mtError,"-------------------------------------------------------" );
		return; 
	}

	m_ObjectFlipFaces			= IsDlgButtonChecked( hPanel, IDC_OBJECT_FLIPFACES );

	string_path				m_ExportName;
	m_ExportName[0]=0;
	if( !EFS.GetSaveName("$import$",m_ExportName,0,0) )
	{
		ELog.Msg(mtInformation,"Export cancelled" );
		ELog.Msg(mtInformation,"-------------------------------------------------------" );
		return;
	}
	EConsole.StayOnTop(TRUE);
	if (strext(m_ExportName)) *strext(m_ExportName)=0;
	strcat(m_ExportName,".object");
	bResult						= SaveAsObject(m_ExportName);

	ELog.Msg					(mtInformation,bResult?". Export succesfully completed.":"! Export failed.");
	ELog.Msg					(mtInformation,"-------------------------------------------------------" );
	EConsole.StayOnTop(FALSE);
}
//-------------------------------------------------------------------------------------------------

void MeshExpUtility::ExportLWO()
{
	BOOL bResult = FALSE;

	if( m_Items.empty() ){
		ELog.Msg(mtError,"Nothing selected" );
		ELog.Msg(mtError,"-------------------------------------------------------" );
		return; 
	}

	m_ObjectFlipFaces			= IsDlgButtonChecked( hPanel, IDC_OBJECT_FLIPFACES );

	string_path m_ExportName;
	m_ExportName[0]=0;
	if( !EFS.GetSaveName("$import$",m_ExportName,0,1) ){
		ELog.Msg(mtInformation,"Export cancelled" );
		ELog.Msg(mtInformation,"-------------------------------------------------------" );
		return;
	}
	if (strext(m_ExportName)) *strext(m_ExportName)=0;
	strcat(m_ExportName,".lwo");
	EConsole.StayOnTop(TRUE);
	bResult						= SaveAsLWO(m_ExportName);

	ELog.Msg					(mtInformation,bResult?". Export succesfully completed.":"! Export failed.");
	ELog.Msg					(mtInformation,"-------------------------------------------------------" );
	EConsole.StayOnTop(FALSE);
}
//-------------------------------------------------------------------------------------------------

void MeshExpUtility::ExportSkin()
{
	BOOL bResult = true;

	if( m_Items.empty() ){
		ELog.Msg(mtError,"Nothing selected." );
		ELog.Msg(mtError,"-------------------------------------------------------" );
		return; 
	}
	if( m_Items.size()>1 ){
		ELog.Msg(mtInformation,"More than one object selected." );
		ELog.Msg(mtInformation,"-------------------------------------------------------" );
		return; 
	}

	m_SkinFlipFaces				= IsDlgButtonChecked( hPanel, IDC_SKIN_FLIPFACES );
	m_SkinAllowDummy			= IsDlgButtonChecked( hPanel, IDC_SKIN_ALLOW_DUMMY );

	string_path m_ExportName;
	m_ExportName[0]=0;
	if( !EFS.GetSaveName("$import$",m_ExportName,0,0) ){
		ELog.Msg(mtInformation,"Export cancelled" );
		ELog.Msg(mtInformation,"-------------------------------------------------------" );
		return;
	}

	EConsole.StayOnTop(TRUE);
	bResult						= SaveAsSkin(m_ExportName);
	ELog.Msg					(mtInformation,bResult?". Export succesfully completed.":"! Export failed.");
	ELog.Msg					(mtInformation,"-------------------------------------------------------" );
	EConsole.StayOnTop(FALSE);
}

void MeshExpUtility::ExportSkinKeys()
{
	BOOL bResult = true;

	if( m_Items.empty() ){
		ELog.Msg(mtError,"Nothing selected." );
		ELog.Msg(mtError,"-------------------------------------------------------" );
		return; 
	}
	if( m_Items.size()>1 ){
		ELog.Msg(mtInformation,"More than one object selected." );
		ELog.Msg(mtInformation,"-------------------------------------------------------" );
		return; 
	}
	m_SkinFlipFaces				= IsDlgButtonChecked( hPanel, IDC_SKIN_FLIPFACES );
	m_SkinAllowDummy			= IsDlgButtonChecked( hPanel, IDC_SKIN_ALLOW_DUMMY );

	string_path m_ExportName;
	m_ExportName[0]=0;
	if( !EFS.GetSaveName("$smotion$",m_ExportName,0) ){
		ELog.Msg(mtInformation,"Export cancelled" );
		ELog.Msg(mtInformation,"-------------------------------------------------------" );
		return;
	}

	EConsole.StayOnTop(TRUE);
	bResult						= SaveSkinKeys(m_ExportName);
	ELog.Msg					(mtInformation,bResult?". Export succesfully completed.":"! Export failed.");
	ELog.Msg					(mtInformation,"-------------------------------------------------------" );
	EConsole.StayOnTop(FALSE);
}

