//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "UI_ToolsCustom.h"
#include "EditObject.h"
#include "EditMesh.h"
#include "ChoseForm.h"
#include "ui_main.h"
#include "PropertiesList.h"               
#include "motion.h"
#include "bone.h"
#include "library.h"
#include "fmesh.h"
#include "folderlib.h"
#include "d3dutils.h"

//------------------------------------------------------------------------------
CToolCustom* Tools=0;
//------------------------------------------------------------------------------
#define CHECK_SNAP(R,A,C){ R+=A; if(fabsf(R)>=C){ A=snapto(R,C); R=0; }else{A=0;}}

CToolCustom::CToolCustom()
{
    m_bReady			= false;
    m_Action			= etaSelect;
    m_Settings.assign	(etfNormalAlign|etfGSnap|etfOSnap|etfMTSnap|etfVSnap|etfASnap|etfMSnap);
    m_Axis				= etAxisZX;
    fFogness			= 0.9f;
    dwFogColor			= 0xffffffff;
	m_pAxisMoveObject	= NULL;
    m_axis_xform		= Fidentity;
}
//---------------------------------------------------------------------------

CToolCustom::~CToolCustom()
{
}
//---------------------------------------------------------------------------

bool CToolCustom::OnCreate()
{
    m_bReady 		= true;

	SetAction		(etaSelect);

    return true;
}

void CToolCustom::OnDestroy()
{
 	Lib.RemoveEditObject	(m_pAxisMoveObject);
	VERIFY					(m_bReady);
    m_bReady				= false;
}
//---------------------------------------------------------------------------

void CToolCustom::SetAction(ETAction action)
{
	switch(action)
    {
    case etaSelect: 
    	m_bHiddenMode	= false; 
        break;
    case etaAdd:
    case etaMove:
    case etaRotate:
    case etaScale:  
    	m_bHiddenMode	= true; 
        break;
    }
    m_Action = action;
    switch(m_Action)
    {
    case etaSelect:  
    	UI->GetD3DWindow()->Cursor = crCross;     
    break;
    case etaAdd:     
    	UI->GetD3DWindow()->Cursor = crArrow;     
    break;
    case etaMove:
    	if(!EPrefs->tools_show_move_axis)    
        	UI->GetD3DWindow()->Cursor = crSizeAll;   
        else
        	UI->GetD3DWindow()->Cursor = crHandPoint;   
        
    break;
    case etaRotate:  
    	UI->GetD3DWindow()->Cursor = crSizeWE;    
    break;
    case etaScale:   
    	UI->GetD3DWindow()->Cursor = crVSplit;    
    break;
    default:         
    	UI->GetD3DWindow()->Cursor = crHelp;
    }

    if(m_Action==etaMove)
    {
        if(!m_pAxisMoveObject && EPrefs->tools_show_move_axis)
        {
            m_pAxisMoveObject = Lib.CreateEditObject("editor\\axis");
            m_Axis = etAxisUndefined;
         }
    }else
    {
        if(m_pAxisMoveObject)
 			Lib.RemoveEditObject	(m_pAxisMoveObject);
	}           
    UI->RedrawScene();
    ExecCommand(COMMAND_REFRESH_UI_BAR);
}


void CToolCustom::SetAxis(ETAxis axis)
{
	m_Axis=axis;
    UI->RedrawScene();
    ExecCommand(COMMAND_REFRESH_UI_BAR);
}

void CToolCustom::SetSettings(u32 mask, BOOL val)
{
	m_Settings.set(mask,val);
    UI->RedrawScene();
    ExecCommand(COMMAND_REFRESH_UI_BAR);
}


bool __fastcall CToolCustom::MouseStart(TShiftState Shift)
{
	switch(m_Action)
    {
    case etaSelect:	break;
    case etaAdd:	break;
    case etaMove:
        if (etAxisY==m_Axis){
            m_MoveXVector.set(0,0,0);
            m_MoveYVector.set(0,1,0);
        }else{
            m_MoveXVector.set( EDevice.m_Camera.GetRight() );
            m_MoveXVector.y = 0;
            m_MoveYVector.set( EDevice.m_Camera.GetDirection() );
            m_MoveYVector.y = 0;
            m_MoveXVector.normalize_safe();
            m_MoveYVector.normalize_safe();
        }
        m_MoveReminder.set(0,0,0);
    	m_MovedAmount.set(0,0,0);
    break;
    case etaRotate:
        m_RotateCenter.set(0,0,0);
        m_RotateVector.set(0,0,0);
        if (etAxisX==m_Axis) m_RotateVector.set(1,0,0);
        else if (etAxisY==m_Axis) m_RotateVector.set(0,1,0);
        else if (etAxisZ==m_Axis) m_RotateVector.set(0,0,1);
        m_fRotateSnapValue 	= 0;
		m_RotateAmount    	= 0;
	break;
    case etaScale:
		m_ScaleAmount.set	(0,0,0);
    break;
    }

    if(m_Action==etaMove && m_pAxisMoveObject)
    {
		Fmatrix 				inv_parent;
        inv_parent.invert		(m_axis_xform);
        Fvector 				start_point, start_dir;
        float					dist;
        SRayPickInfo			pinfo;

        start_point				= UI->m_CurrentRStart;
        start_dir				= UI->m_CurrentRDir;
        dist					= 10000;
		m_pAxisMoveObject->RayPick(dist, start_point, start_dir,inv_parent,&pinfo);
        if(pinfo.e_mesh)
        {
        	LPCSTR mn = pinfo.e_mesh->Name().c_str();
            if(0==stricmp(mn,"axis_x"))
        		SetAxis(etAxisX);
            else
            if(0==stricmp(mn,"axis_y"))
        		SetAxis(etAxisY);
            else
            if(0==stricmp(mn,"axis_z"))
        		SetAxis(etAxisZ);
            else
            if(0==stricmp(mn,"center"))
        		SetAxis(etAxisZX);
            else
            	R_ASSERT2(0, "fix axis name");
        };
    }
    
	return m_bHiddenMode;
}

bool __fastcall CToolCustom::MouseEnd(TShiftState Shift)
{
	switch(m_Action){
    case etaSelect: break;
    case etaAdd: 	break;
    case etaMove:	
        if(EPrefs->tools_show_move_axis)
    		m_Axis			= etAxisUndefined;
    break;
    case etaRotate:	break;
    case etaScale:	break;
    }
	return true;
}

void __fastcall CToolCustom::MouseMove(TShiftState Shift)
{
	switch(m_Action){
    case etaSelect: break;
    case etaAdd: 	break;
    case etaMove:
    {      
        m_MovedAmount.mul( m_MoveXVector, UI->m_MouseSM * UI->m_DeltaCpH.x );
        m_MovedAmount.mad( m_MoveYVector, -UI->m_MouseSM * UI->m_DeltaCpH.y );

        if( m_Settings.is(etfMSnap) ){
        	CHECK_SNAP(m_MoveReminder.x,m_MovedAmount.x,m_MoveSnap);
        	CHECK_SNAP(m_MoveReminder.y,m_MovedAmount.y,m_MoveSnap);
        	CHECK_SNAP(m_MoveReminder.z,m_MovedAmount.z,m_MoveSnap);
        }

        if (!(etAxisX==m_Axis)&&!(etAxisZX==m_Axis)) 	m_MovedAmount.x = 0.f;
        if (!(etAxisZ==m_Axis)&&!(etAxisZX==m_Axis)) 	m_MovedAmount.z = 0.f;
        if (!(etAxisY==m_Axis)) 						m_MovedAmount.y = 0.f;
        
    }break;
    case etaRotate:{
        m_RotateAmount = -UI->m_DeltaCpH.x * UI->m_MouseSR;
        if( m_Settings.is(etfASnap) ) CHECK_SNAP(m_fRotateSnapValue,m_RotateAmount,m_RotateSnapAngle);
    }break;
    case etaScale:{
        float dy = UI->m_DeltaCpH.x * UI->m_MouseSS;
        if (dy>1.f) dy=1.f; else if (dy<-1.f) dy=-1.f;

        m_ScaleAmount.set( dy, dy, dy );

        if (m_Settings.is(etfNUScale)){
            if (!(etAxisX==m_Axis)&&!(etAxisZX==m_Axis)) 	m_ScaleAmount.x = 0.f;
            if (!(etAxisZ==m_Axis)&&!(etAxisZX==m_Axis)) 	m_ScaleAmount.z = 0.f;
            if (!(etAxisY==m_Axis)) 						m_ScaleAmount.y = 0.f;
        }
    }break;
    }
}

void CToolCustom::GetCurrentFog(u32& fog_color, float& s_fog, float& e_fog)
{
    s_fog				= psDeviceFlags.is(rsFog)?(1.0f - fFogness)* 0.85f * UI->ZFar():0.99f*UI->ZFar();
    e_fog				= psDeviceFlags.is(rsFog)?0.91f * UI->ZFar():UI->ZFar();
    fog_color 			= dwFogColor;
}

void CToolCustom::RenderEnvironment()
{
}

void CToolCustom::Clear()
{
	ClearDebugDraw		();
}

void CToolCustom::Render()
{
	// render errors
    EDevice.SetShader		(EDevice.m_SelectionShader);
    RCache.set_xform_world	(Fidentity);
    EDevice.RenderNearer		(0.0003f);
    EDevice.SetRS			(D3DRS_CULLMODE,D3DCULL_NONE);
    AnsiString temp;
    int cnt=0;
    for (SDebugDraw::PointIt vit=m_DebugDraw.m_Points.begin(); vit!=m_DebugDraw.m_Points.end(); ++vit)
    {
        LPCSTR s = NULL;
        if (vit->i)
        {
        	temp.sprintf		("P: %d",cnt++);
            s = temp.c_str();
        }

        if(vit->descr.size())
        {
            s = vit->descr.c_str();
        }
        DU_impl.dbgDrawVert(vit->p[0],			vit->c,	s?s:"");
    }
    EDevice.SetShader		(EDevice.m_SelectionShader);
    cnt=0;
    for (SDebugDraw::LineIt eit=m_DebugDraw.m_Lines.begin(); eit!=m_DebugDraw.m_Lines.end(); eit++){
        if (eit->i)        temp.sprintf		("L: %d",cnt++);
        DU_impl.dbgDrawEdge		(eit->p[0],eit->p[1],				eit->c,	eit->i?temp.c_str():"");
    }
    EDevice.SetShader		(EDevice.m_SelectionShader);
    cnt=0;
    for (SDebugDraw::FaceIt fwit=m_DebugDraw.m_WireFaces.begin(); fwit!=m_DebugDraw.m_WireFaces.end(); fwit++){
    	if (fwit->i)        temp.sprintf		("F: %d",cnt++);
        DU_impl.dbgDrawFace		(fwit->p[0],fwit->p[1],fwit->p[2],fwit->c,	fwit->i?temp.c_str():"");
    }
    cnt=0;
    if (!m_DebugDraw.m_SolidFaces.empty()){
	    EDevice.SetShader		(EDevice.m_SelectionShader);
        DU_impl.DD_DrawFace_begin	(FALSE);
        for (SDebugDraw::FaceIt fsit=m_DebugDraw.m_SolidFaces.begin(); fsit!=m_DebugDraw.m_SolidFaces.end(); fsit++)
            DU_impl.DD_DrawFace_push	(fsit->p[0],fsit->p[1],fsit->p[2],	fsit->c);
        DU_impl.DD_DrawFace_end		();
    }
    EDevice.SetShader		(EDevice.m_SelectionShader);
    cnt=0;
    for (SDebugDraw::OBBVecIt oit=m_DebugDraw.m_OBB.begin(); oit!=m_DebugDraw.m_OBB.end(); oit++)
    {
        temp.sprintf		("OBB: %d",cnt++);
        DU_impl.DrawOBB			(Fidentity,*oit,0x2F00FF00,0xFF00FF00);
        DU_impl.OutText			(oit->m_translate,temp.c_str(),0xffff0000,0x0000000);
    }
    EDevice.SetRS			(D3DRS_CULLMODE,D3DCULL_CCW);
    EDevice.ResetNearer		();
    
	if(m_pAxisMoveObject && GetSelectionPosition(m_axis_xform) )
    {
        for(SurfaceIt s_it = m_pAxisMoveObject->Surfaces().begin(); s_it != m_pAxisMoveObject->Surfaces().end(); ++s_it)
        {
            EDevice.SetShader			((*s_it)->_Shader());
        	RCache.set_xform_world		(m_axis_xform);
            
            for(int idx=0; idx< m_pAxisMoveObject->Meshes().size(); ++idx)
            {
            	 CEditableMesh* M = m_pAxisMoveObject->Meshes()[idx];
                 if(	(m_Axis==idx)   										||
                 		(idx==etAxisZX) 										||
                        (m_Axis==etAxisZX && (idx==etAxisX || idx==etAxisZ) ) 	||
                        (m_Axis==etAxisUndefined)
                   )
                 	M->Render			(m_axis_xform, *s_it);
             }
        }
   	}
    
	if(m_pAxisMoveObject && GetSelectionPosition(m_axis_xform) )
    {
        for(SurfaceIt s_it = m_pAxisMoveObject->Surfaces().begin(); s_it != m_pAxisMoveObject->Surfaces().end(); ++s_it)
        {
            EDevice.SetShader			((*s_it)->_Shader());
        	RCache.set_xform_world		(m_axis_xform);
            
            for(int idx=0; idx< m_pAxisMoveObject->Meshes().size(); ++idx)
            {
            	 CEditableMesh* M = m_pAxisMoveObject->Meshes()[idx];
                 if(	(m_Axis==idx)   										||
                 		(idx==etAxisZX) 										||
                        (m_Axis==etAxisZX && (idx==etAxisX || idx==etAxisZ) ) 	||
                        (m_Axis==etAxisUndefined)
                   )
                 	M->Render			(m_axis_xform, *s_it);
             }
        }
   	}
}
//------------------------------------------------------------------------------

