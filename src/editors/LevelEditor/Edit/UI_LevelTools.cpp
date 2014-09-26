#include "stdafx.h"
#pragma hdrstop

#include "UI_LevelTools.h"
#include "ESceneControlsCustom.h"
#include "cursor3d.h"
#include "LeftBar.h"
#include "Scene.h"
#include "ui_levelmain.h"

#include "editlibrary.h"
#include "ObjectList.h"

#include "igame_persistent.h"
#include "Builder.h"

#include "lephysics.h"

#define DETACH_FRAME(a) 	if (a){ (a)->Hide(); 	(a)->Parent = NULL; }
#define ATTACH_FRAME(a,b)	if (a){ (a)->Parent=(b);(a)->Show(); 		}

CLevelTool*&	LTools=(CLevelTool*)Tools;

TShiftState ssRBOnly;
//---------------------------------------------------------------------------
CLevelTool::CLevelTool()
{
    fFogness	= 0.9f;
    dwFogColor	= 0xffffffff;
    m_Flags.zero();
}
//---------------------------------------------------------------------------
CLevelTool::~CLevelTool()
{
}
//---------------------------------------------------------------------------

TForm*	CLevelTool::GetFrame()
{
	if (pCurTool)
    	return pCurTool->pFrame;
        
    return 0;
}
//---------------------------------------------------------------------------
bool CLevelTool::OnCreate()
{
	inherited::OnCreate();
    target          = OBJCLASS_DUMMY;
    sub_target		= -1;
    pCurTool       = 0;
    ssRBOnly << ssRight;
    paParent 		= fraLeftBar->paFrames;   VERIFY(paParent);
    m_Flags.set		(flChangeAction,FALSE);
    m_Flags.set		(flChangeTarget,FALSE);
    // scene creating
    Scene->OnCreate	();
    // change target to Object
    ExecCommand		(COMMAND_CHANGE_TARGET, OBJCLASS_SCENEOBJECT);
	m_Props 		= TProperties::CreateForm(	"Object Inspector",
    											0,
                                                alClient,
                                                TOnModifiedEvent(this,&CLevelTool::OnPropsModified),
                                                0,
                                                TOnCloseEvent(this,&CLevelTool::OnPropsClose),
                          TProperties::plItemFolders|TProperties::plFolderStore|TProperties::plNoClearStore|TProperties::plFullExpand);
    pObjectListForm = TfrmObjectList::CreateForm();
    return true;
}
//---------------------------------------------------------------------------

void CLevelTool::OnDestroy()
{
	inherited::OnDestroy();
    TfrmObjectList::DestroyForm(pObjectListForm);
	TProperties::DestroyForm(m_Props);
    // scene destroing
    if (pCurTool)
    	pCurTool->OnDeactivate();
	Scene->OnDestroy		();
}
//---------------------------------------------------------------------------
void CLevelTool::Reset()
{
	RealSetTarget(GetTarget(),estDefault,true);
}
//---------------------------------------------------------------------------

bool __fastcall CLevelTool::MouseStart(TShiftState Shift)
{
	inherited::MouseStart(Shift);
    if(pCurTool && pCurTool->pCurControl)
    {
    	if ((pCurTool->pCurControl->Action()!=etaSelect)&&
	    	(!pCurTool->IsEditable() || !pCurTool->AllowMouseStart() || (pCurTool->ClassID==OBJCLASS_DUMMY)))
            return false;

        return pCurTool->pCurControl->Start(Shift);
    }
    return false;
}
//---------------------------------------------------------------------------
void __fastcall CLevelTool::MouseMove(TShiftState Shift)
{
	inherited::MouseMove(Shift);
    if(pCurTool&&pCurTool->pCurControl)
    {
	    if (HiddenMode())
        	ExecCommand(COMMAND_UPDATE_PROPERTIES);

     	pCurTool->pCurControl->Move(Shift);
    }
}
//---------------------------------------------------------------------------
bool __fastcall CLevelTool::MouseEnd(TShiftState Shift)
{
	inherited::MouseEnd(Shift);
    if(pCurTool&&pCurTool->pCurControl)
    {
	    if (HiddenMode())
        	ExecCommand(COMMAND_UPDATE_PROPERTIES);

    	return pCurTool->pCurControl->End(Shift);
    }
    return false;
}
//---------------------------------------------------------------------------
void __fastcall CLevelTool::OnObjectsUpdate()
{
	UpdateProperties(false);
    if(pCurTool&&pCurTool->pCurControl)
    	return pCurTool->OnObjectsUpdate();
}
//---------------------------------------------------------------------------
bool __fastcall CLevelTool::HiddenMode()
{
    if(pCurTool&&pCurTool->pCurControl)
    	return pCurTool->pCurControl->HiddenMode();
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall CLevelTool::KeyDown   (WORD Key, TShiftState Shift)
{
    if(pCurTool&&pCurTool->pCurControl)
    	return pCurTool->pCurControl->KeyDown(Key,Shift);

    return false;
}
//---------------------------------------------------------------------------
bool __fastcall CLevelTool::KeyUp     (WORD Key, TShiftState Shift)
{
    if(pCurTool&&pCurTool->pCurControl)
    	return pCurTool->pCurControl->KeyUp(Key,Shift);

    return false;
}
//---------------------------------------------------------------------------
bool __fastcall CLevelTool::KeyPress  (WORD Key, TShiftState Shift)
{
    if(pCurTool&&pCurTool->pCurControl)
    	return pCurTool->pCurControl->KeyPress(Key,Shift);

    return false;
}
//---------------------------------------------------------------------------

void CLevelTool::RealSetAction   (ETAction act)
{
	inherited::SetAction(act);
    if (pCurTool)
    	pCurTool->SetAction(act);

    ExecCommand(COMMAND_UPDATE_TOOLBAR);
    m_Flags.set	(flChangeAction,FALSE);
}

void __fastcall CLevelTool::SetAction(ETAction act)
{
	// если мышь захвачена - изменим action после того как она освободится
	if (UI->IsMouseCaptured()||UI->IsMouseInUse()||!false){
	    m_Flags.set	(flChangeAction,TRUE);
        iNeedAction=act;
    }else
    	RealSetAction	(act);
}
//---------------------------------------------------------------------------

void __fastcall CLevelTool::RealSetTarget   (ObjClassID tgt,int sub_tgt,bool bForced)
{
    if(bForced||(target!=tgt)||(sub_target!=sub_tgt)){
        target 					= tgt;
        sub_target 				= sub_tgt;
        if (pCurTool){
            DETACH_FRAME(pCurTool->pFrame);
            pCurTool->OnDeactivate();
        }
        pCurTool				= Scene->GetTool(tgt);
        VERIFY					(pCurTool);
        pCurTool->SetSubTarget	(sub_target);

        pCurTool->OnActivate	();

        pCurTool->SetAction		(GetAction());

        if (pCurTool->IsEditable())
        	ATTACH_FRAME(pCurTool->pFrame, paParent);
    }
    UI->RedrawScene();
    fraLeftBar->ChangeTarget(tgt);
    fraLeftBar->UpdateSnapList();
    ExecCommand(COMMAND_UPDATE_TOOLBAR);
    m_Flags.set(flChangeTarget,FALSE);
}
//---------------------------------------------------------------------------
void __fastcall CLevelTool::ResetSubTarget()
{
	VERIFY(pCurTool);
	pCurTool->ResetSubTarget();
}
//---------------------------------------------------------------------------
void __fastcall CLevelTool::SetTarget(ObjClassID tgt, int sub_tgt)
{
	// если мышь захвачена - изменим target после того как она освободится
	if (UI->IsMouseCaptured()||UI->IsMouseInUse()||!false){
	    m_Flags.set(flChangeTarget,TRUE);
        if(tgt == OBJCLASS_WAY && sub_tgt==2 && target==tgt)
        {
            iNeedTarget		= tgt;
            iNeedSubTarget  = (sub_target)?0:1;
        }else
        {
            iNeedTarget		= tgt;
            iNeedSubTarget  = sub_tgt;
        }
    }else
    	RealSetTarget(tgt,sub_tgt,false);
}
//---------------------------------------------------------------------------

ObjClassID CLevelTool::CurrentClassID()
{
	return GetTarget();
}
//---------------------------------------------------------------------------

void CLevelTool::OnShowHint(AStringVec& ss)
{
	Scene->OnShowHint(ss);
}
//---------------------------------------------------------------------------

bool CLevelTool::Pick(TShiftState Shift)
{
    if( Scene->locked() && (esEditLibrary==UI->GetEState())){
        UI->IR_GetMousePosReal(EDevice.m_hRenderWnd, UI->m_CurrentCp);
        UI->m_StartCp = UI->m_CurrentCp;
        EDevice.m_Camera.MouseRayFromPoint(UI->m_CurrentRStart, UI->m_CurrentRDir, UI->m_CurrentCp );
        SRayPickInfo pinf;
        TfrmEditLibrary::RayPick(UI->m_CurrentRStart,UI->m_CurrentRDir,&pinf);
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------

void CLevelTool::RefreshProperties()
{
	m_Props->RefreshForm();
}

void CLevelTool::ShowProperties(LPCSTR focus_to_item)
{
    m_Props->ShowProperties	();
    RealUpdateProperties	();

	if(focus_to_item)
    	m_Props->SelectFolder	(focus_to_item);
    else
    {
    	if(pCurTool && pCurTool->ClassID!=OBJCLASS_DUMMY)
        {
           	LPCSTR cn = pCurTool->ClassDesc();
    		m_Props->SelectFolder	(cn);
        }
    }

    UI->RedrawScene			();
}
//---------------------------------------------------------------------------

void CLevelTool::RealUpdateProperties()
{
	if (m_Props->Visible)
    {
		if (m_Props->IsModified()) Scene->UndoSave();
        
        ObjectList lst;
        PropItemVec items;

        // scene common props
        Scene->FillProp				("",items,CurrentClassID());

		m_Props->AssignItems		(items);
    }
	m_Flags.set(flUpdateProperties,FALSE);
}
//---------------------------------------------------------------------------

void CLevelTool::OnPropsClose()
{
	if (m_Props->IsModified()) Scene->UndoSave();
}
//---------------------------------------------------------------------------

void __fastcall CLevelTool::OnPropsModified()
{
	Scene->Modified();
//	Scene->UndoSave();
	UI->RedrawScene();
}
//---------------------------------------------------------------------------

#include "EditLightAnim.h"

bool CLevelTool::IfModified()
{
    EEditorState est 		= UI->GetEState();
    switch(est){
    case esEditLightAnim: 	return TfrmEditLightAnim::FinalClose();
    case esEditLibrary: 	return TfrmEditLibrary::FinalClose();
    case esEditScene:		return Scene->IfModified();
    default: THROW;
    }
    return false;
}
//---------------------------------------------------------------------------

void CLevelTool::ZoomObject(BOOL bSelectedOnly)
{
    if( !Scene->locked() ){
        Scene->ZoomExtents(CurrentClassID(),bSelectedOnly);
    } else {
        if (UI->GetEState()==esEditLibrary){
            TfrmEditLibrary::ZoomObject();
        }
    }
}
//---------------------------------------------------------------------------

void CLevelTool::GetCurrentFog(u32& fog_color, float& s_fog, float& e_fog)
{
/*
	if (psDeviceFlags.is(rsEnvironment)&&psDeviceFlags.is(rsFog)){
        s_fog				= g_pGamePersistent->Environment().CurrentEnv->fog_near;
        e_fog				= g_pGamePersistent->Environment().CurrentEnv->fog_far;
        Fvector& f_clr		= g_pGamePersistent->Environment().CurrentEnv->fog_color;
        fog_color 			= color_rgba_f(f_clr.x,f_clr.y,f_clr.z,1.f);
    }else{
*/    
        s_fog				= psDeviceFlags.is(rsFog)?(1.0f - fFogness)* 0.85f * UI->ZFar():0.99f*UI->ZFar();
        e_fog				= psDeviceFlags.is(rsFog)?0.91f * UI->ZFar():UI->ZFar();
        fog_color 			= dwFogColor;
/*
    }
*/    
}
//---------------------------------------------------------------------------

LPCSTR CLevelTool::GetInfo()
{
	static AnsiString sel;
	int cnt = Scene->SelectionCount(true,CurrentClassID());
	return sel.sprintf(" Sel: %d",cnt).c_str();
}
//---------------------------------------------------------------------------

void __fastcall CLevelTool::OnFrame()
{
	Scene->OnFrame		(EDevice.fTimeDelta);
    EEditorState est 	= UI->GetEState();
    if ((est==esEditScene)||(est==esEditLibrary)||(est==esEditLightAnim)){
        if (true/*!UI->IsMouseCaptured()*/)
        {
            // если нужно изменить target выполняем после того как мышь освободится
            if(m_Flags.is(flChangeTarget)) 		RealSetTarget(iNeedTarget,iNeedSubTarget,false);
            // если нужно изменить action выполняем после того как мышь освободится
            if(m_Flags.is(flChangeAction)) 		RealSetAction(ETAction(iNeedAction));
        }
        if (m_Flags.is(flUpdateProperties)) 	RealUpdateProperties();
        if (m_Flags.is(flUpdateObjectList)) 	RealUpdateObjectList();
        if (est==esEditLightAnim) TfrmEditLightAnim::OnIdle();
    }
}
//---------------------------------------------------------------------------
#include "d3dutils.h"
void __fastcall CLevelTool::RenderEnvironment()
{
    // draw sky
    EEditorState est 		= UI->GetEState();
    switch(est){
    case esEditLightAnim:
    case esEditScene:		
    	if (psDeviceFlags.is(rsEnvironment))
        { 
//.    		g_pGamePersistent->Environment().RenderSky	();
//.    		g_pGamePersistent->Environment().RenderClouds	();
        }
    }
}

void __fastcall CLevelTool::Render()
{
	// Render update
    ::Render->Calculate		();
    ::Render->Render		();

    EEditorState est 		= UI->GetEState();
    // draw scene
    switch(est){
    case esEditLibrary: 	TfrmEditLibrary::OnRender(); 	break;
    case esEditLightAnim:
    case esEditScene:
    	Scene->Render(EDevice.m_Camera.GetTransform()); 
//.	    if (psDeviceFlags.is(rsEnvironment)) g_pGamePersistent->Environment().RenderLast	();
    break;
    case esBuildLevel:  	Builder.OnRender();				break;
    }
    // draw cursor
    LUI->m_Cursor->Render();

    inherited::Render		();
}
//---------------------------------------------------------------------------

void CLevelTool::ShowObjectList()
{
	if (pObjectListForm) pObjectListForm->ShowObjectList();
}
//---------------------------------------------------------------------------

void CLevelTool::RealUpdateObjectList()
{
	if (pObjectListForm) pObjectListForm->UpdateObjectList();
	m_Flags.set(flUpdateObjectList,FALSE);
}
//---------------------------------------------------------------------------

bool CLevelTool::IsModified()
{
	return Scene->IsUnsaved();
}
//---------------------------------------------------------------------------

#include "../ECore/Editor/EditMesh.h"
bool CLevelTool::RayPick(const Fvector& start, const Fvector& dir, float& dist, Fvector* pt, Fvector* n)
{
    if (Scene->ObjCount()&&(UI->GetEState()==esEditScene)){
        SRayPickInfo pinf;
        pinf.inf.range	= dist;
        if (Scene->RayPickObject(dist, start,dir,OBJCLASS_SCENEOBJECT,&pinf,0)){ 
        	dist		= pinf.inf.range;
        	if (pt) 	pt->set(pinf.pt); 
            if (n){	
                const Fvector* PT[3];
                pinf.e_mesh->GetFacePT(pinf.inf.id, PT);
            	n->mknormal(*PT[0],*PT[1],*PT[2]);
            }
            return true;
        }
    }
    Fvector N={0.f,-1.f,0.f};
    Fvector P={0.f,0.f,0.f};
    Fplane PL; PL.build(P,N);
    float d;
    if (PL.intersectRayDist(start,dir,d)&&(d<=dist)){
        dist = d;
        if (pt) pt->mad(start,dir,dist); 
        if (n)	n->set(N);
        return true;
    }else return false;
}

bool CLevelTool::GetSelectionPosition(Fmatrix& result)
{
	if(pCurTool)
    {
    	Fvector 			center;
    	Fbox 				BB;
        BB.invalidate		();
//    	pCurTool->GetBBox	(BB, true);

        const CCustomObject* object = pCurTool->LastSelected();
        if(!object)
        	return false;
            
        object->GetBox		(BB);
        
        BB.getcenter		(center);
        center.y			= BB.max.y;

		Fvector2			pt_ss;
        pt_ss.set			(10000,-10000);
        Fvector				pt_ss_3d;
        BB.setb				(center, Fvector().set(1.0f,1.0f,1.0f));
        for(int k=0;k<8;++k)
        {
        	Fvector pt;
        	BB.getpoint(k,pt);
			EDevice.mFullTransform.transform(pt_ss_3d, pt);
            
            pt_ss.x = _min(pt_ss.x, pt_ss_3d.y);
            pt_ss.y = _max(pt_ss.y, pt_ss_3d.y);
        }

        float r_bb_ss	 = pt_ss.y - pt_ss.x;
        clamp(r_bb_ss, 0.0f,0.10f);
        float des_radius = 0.2f; 
        float csale 	 = des_radius/r_bb_ss;
        
        result.scale	(csale,csale,csale);
        result.c 		= center;
        return 			true;
    }else
		return 			false;
}
void   CLevelTool::Simulate()
{
	if( !g_scene_physics.Simulating() )
	    g_scene_physics.CreateShellsSelected();
    else
    	g_scene_physics.DestroyAll();
    UI->RedrawScene();
    ExecCommand(COMMAND_REFRESH_UI_BAR);
}
void   CLevelTool::UseSimulatePositions()
{
	g_scene_physics.UseSimulatePoses();
}
