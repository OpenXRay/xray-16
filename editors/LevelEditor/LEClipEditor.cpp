#include "stdafx.h"
#pragma hdrstop

#include "LEClipEditor.h"
#include "SkeletonAnimated.h"
#include "motion.h"
#include "../ECore/Editor/editobject.h"
//.#include "UI_ActorTools.h"
#include "../ECore/Editor/UI_Main.h"
#include "../xrEProps/ItemList.h"

#include <ElVCLUtils.hpp>
#include <ElTools.hpp>
#include "ElTree.hpp"


//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "mxPlacemnt"
#pragma link "mxPlacemnt"
#pragma link "ExtBtn"
#pragma link "MXCtrls"
#pragma link "Gradient"
#pragma link "ElScrollBar"
#pragma link "ElXPThemedControl"
#pragma link "MxMenus"
#pragma link "ElPanel"
#pragma link "ElSplit"
#pragma link "multi_edit"
#pragma link "ElTrackBar"
#pragma resource "*.dfm"

static const TColor CLIP_INACTIVE_COLOR		= 0x00686868;
static const TColor CLIP_ACTIVE_COLOR		= 0x00A1A1A1;
static const TColor CLIP_ACTIVE_DRAG_COLOR	= 0x00FFFFFF;
static const TColor BP_INACTIVE_COLOR		= 0x00686868;
static const TColor BP_ACTIVE_COLOR			= 0x00A1A1A1;
static const TColor BP_ACTIVE_DRAG_COLOR	= 0x00FFFFFF;

static TShiftState 		drag_state;
static int 	drag_obj	= 0xFFFF;
static BOOL g_resizing	= FALSE;
static int 	g_X_prev	= 0;
static int 	g_X_dx		= 0;

TClipMaker* g_clip_maker = NULL;
CAnimationClip::CAnimationClip(LPCSTR n, TClipMaker* own) 
{
	owner				= own;
	start_time			= 0.0f;
	length				= 2.f;
    name				= n;
    idx					= -1;
}

CAnimationClip::CAnimationClip(TClipMaker* own)
{
	owner				= own;
	start_time			= 0.0f;
	length				= 2.f;
    idx					= -1;
}

CAnimationClip::~CAnimationClip()
{
};

void CAnimationClip::SetCycle(MotionID mid, u16 part_id, u8 part_count)
{
    for (int k=0; k<part_count; ++k)	
    {
        if( k==part_id || part_id==BI_NONE )
        	animItems[k].mid = mid;
        else
        	animItems[k].clear();
     }
}

IC bool clip_pred(CAnimationClip* x, CAnimationClip* y)
{
	return x->start_time<y->start_time;
};

IC bool clip_pred_float(float x, CAnimationClip* y)
{
	return x<y->start_time;
};

TClipMaker*	TClipMaker::CreateForm()
{
	return xr_new<TClipMaker>((TComponent*)0);
}

void TClipMaker::DestroyForm(TClipMaker* form)
{
	if(form)
    {
  	TItemList::DestroyForm	(form->m_ObjectItems);
	xr_delete				(form);
    }
}
    
void TClipMaker::ShowEditor(CKinematicsAnimated* O)
{
    VERIFY						(O);
	Show						();

    if(m_RenderObject != O)
    {
        m_RenderObject 				= O; 
        ListItemsVec 				items;
    
        u16 cnt						= m_RenderObject->LL_MotionsSlotCount();
        for (u16 k=0; k<cnt; k++)
        {
            accel_map *ll_motions	= m_RenderObject->LL_Motions(k);
            accel_map::iterator 	_I, _E;
            _I						= ll_motions->begin();
            _E						= ll_motions->end();
            for (; _I!=_E; ++_I)
            {
                ListItem* I 		= LHelper().CreateItem(items,_I->first.c_str(),0,0,0);

                MotionID 			mid;
                mid.set				(k, _I->second);
                I->tag				= mid.val;
            }
        }

		m_ObjectItems->AssignItems(items,false);
    }
    UpdateClips		();
    UpdateProperties();
}

void TClipMaker::HideEditor()
{
	m_RenderObject 	= NULL;
	Clear			();
	Hide			();
}

void TClipMaker::Clear()
{
	m_ClipProps->ClearProperties();
	m_ClipList->ClearList		();
	Stop			();

	for (AnimClipIt it=clips.begin(); it!=clips.end(); ++it)
    	xr_delete	(*it);
        
    clips.clear		();
    sel_clip		= 0;
    UpdateClips		(true);
	m_RTFlags.zero	();
}

__fastcall TClipMaker::TClipMaker(TComponent* Owner) : TForm(Owner)
{
    DEFINE_INI		(fsStorage);
    m_LB[0]    		= lbBPName0;
    m_LB[1]    		= lbBPName1;
    m_LB[2]    		= lbBPName2;
    m_LB[3]    		= lbBPName3;
    m_TotalLength	= 0.f;
    m_Zoom			= 24.f;
    m_CurrentPlayTime=0.f;

    m_ObjectItems 		= TItemList::CreateForm("",paAnimSelect,alClient,TItemList::ilDragCustom|TItemList::ilMultiSelect|TItemList::ilSuppressStatus);
}

void __fastcall TClipMaker::FormCreate(TObject *Sender)
{
	m_ClipProps		= TProperties::CreateForm("Clip Properties",paClipProps,alClient);
	m_ClipList		= TItemList::CreateForm("Clips",paClipList,alClient,0);
	m_ClipList->SetOnItemsFocusedEvent(TOnILItemsFocused(this,&TClipMaker::OnClipItemFocused));

	EDevice.seqFrame.Add	(this);
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::FormDestroy(TObject *Sender)
{
	EDevice.seqFrame.Remove(this);
	Clear			();
	TProperties::DestroyForm(m_ClipProps);
	TItemList::DestroyForm	(m_ClipList);
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::FormShow(TObject *Sender)
{
	UI->CheckWindowPos(this);
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	CanClose		= true;
/*	bool bAsk		= false;
    ClipVec& tgt 	= m_CurrentObject->Clips();
    if (tgt.size()!=clips.size()) bAsk=true;
    else{
        ClipIt t_it		= tgt.begin();
        for (UIClipIt s_it=clips.begin(); s_it!=clips.end(); s_it++,t_it++){
            if (!(*s_it)->Equal(*t_it)){
                bAsk 	= true;
                break;
            }
        }
    }
	if (bAsk){
        int res 	= ELog.DlgMsg(mtConfirmation, "Save changes before quit?");
        switch (res){
        case mrYes:{
            for (ClipIt it=tgt.begin(); it!=tgt.end(); it++)
                xr_delete	(*it);
            tgt.resize(clips.size());
            ClipIt t_it=tgt.begin();
            for (UIClipIt s_it=clips.begin(); s_it!=clips.end(); s_it++,t_it++){
                *t_it = xr_new<CClip>();
                **t_it= *(CClip*)*s_it;
            }
            Tools.Modified();
        }break;
        case mrNo: 	break;
        case mrCancel: CanClose=false; break;
        }
    }
*/
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::FormClose(TObject *Sender,
      TCloseAction &Action)
{
	Stop	();
//.	Clear();
}
//---------------------------------------------------------------------------

CAnimationClip* TClipMaker::FindClip(int x)
{
	return FindClip(float(x)/m_Zoom);
}
//---------------------------------------------------------------------------

CAnimationClip* TClipMaker::FindClip(float t)
{
	if (clips.empty()) return 0;
	AnimClipIt it = std::upper_bound(clips.begin(),clips.end(),t,clip_pred_float);
    VERIFY (it!=clips.begin());
    it--;
    return *it;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// clip Drag'n'Drop
// clip Mouse
//.
//---------------------------------------------------------------------------
void __fastcall TClipMaker::ClipDragOver(TObject *Sender,
      TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
//TEMP
/*
    Accept = false;
	TElTreeDragObject* obj 	= dynamic_cast<TElTreeDragObject*>(Source);
    if (obj)
    {
        TMxPanel* A			= dynamic_cast<TMxPanel*>(Sender);
        if (A==paClips)
        {
            TElTree* tv		= dynamic_cast<TElTree*>(obj->Control);
            if (tv->SelectedCount)
            {
                for (TElTreeItem* item = tv->GetNextSelected(0); item; item = tv->GetNextSelected(item))
                {
                    ListItem* prop		= (ListItem*)item->Tag;
					if (prop&&(prop->Type()==emMotion))
                    {
                        Accept			= true;
                    }
                }
            }
        }
    }else
    {
		Accept = (Sender==paClips);
    }
*/    
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ClipDragDrop(TObject *Sender,
      TObject *Source, int X, int Y)
{
    VERIFY 					(Sender==paClips);
    CAnimationClip* tgt		= FindClip(X); 
    VERIFY					(tgt);
	TElTreeDragObject* obj 	= dynamic_cast<TElTreeDragObject*>(Source);
    if (obj)
    {
        TElTree* tv			= dynamic_cast<TElTree*>(obj->Control);
        if (tv->SelectedCount)
        {

            for (TElTreeItem* item 	= tv->GetNextSelected(0); item; item = tv->GetNextSelected(item))
            {
                ListItem* prop		= (ListItem*)item->Tag; VERIFY(prop);
               
                MotionID 			mid;
                mid.val				= prop->tag;

        		CMotionDef* MD 		= m_RenderObject->LL_GetMotionDef(mid);
				u8 pc 				= m_RenderObject->partitions().count();
                tgt->SetCycle		(mid, MD->bone_or_part, pc);
                break;
            }
        }
    }else
    {
        float rt					= float(X)/m_Zoom-tgt->StartTime();

        if (rt<tgt->Length()/2.f)	
        	sel_clip->start_time	= tgt->start_time-EPS_L;
        else						
        	sel_clip->start_time	= tgt->start_time+EPS_L;
    }
    UpdateClips		();
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ClipStartDrag(TObject *Sender,
      TDragObject *&DragObject)
{
	TMxPanel* P = dynamic_cast<TMxPanel*>(Sender); VERIFY(P);
	drag_obj	= P->Tag;
	RepaintClips();
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ClipEndDrag(TObject *Sender,
      TObject *Target, int X, int Y)
{
	drag_obj	= 0xFFFF;
	RepaintClips();
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ClipMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
    if (Button==mbLeft)
    {
        SelectClip		(FindClip(X));
        if (paClips->Cursor==crHSplit)
        {
            g_resizing	= TRUE;
            Stop		();
            g_X_prev 	= X;
            g_X_dx		= X-sel_clip->PRightUI();
            RepaintClips();
        }else{
            paClips->BeginDrag(false, 2);
		}
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ClipMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    VERIFY				(sel_clip);
    
	TMxPanel* P			= dynamic_cast<TMxPanel*>(Sender);
    CAnimationClip* C	= FindClip(X); VERIFY(C);
    int cX				= X-C->PLeftUI();
    float w0 			= float(cX)/C->PWidthUI();
    int w1	 			= C->PWidthUI()-cX;
	if ((w0>0.75f)&&(w0<1.f)&&(w1<7)){
    	P->Cursor 		= crHSplit;
    }else{
    	if (!g_resizing)
	    	P->Cursor 	= crDefault;
    }
    if (g_resizing)
    {
        float dx		= float(X-(g_X_prev+g_X_dx))/m_Zoom;
        if (!fis_zero(dx))
        {
    	    sel_clip->length += dx;
            if (sel_clip->length<0.01f) sel_clip->length=0.01f;
    		g_X_prev 	= sel_clip->PRightUI();
            UI->ShowHint	(AnsiString().sprintf("Length: %s",FloatTimeToStrTime(sel_clip->Length(),false,true,true,true).c_str()));
        }
        UpdateClips		();
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ClipMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (Button==mbLeft){
		UI->HideHint	();
        g_resizing	= FALSE;
        UpdateClips	();
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// BP Drag'n'Drop
// BP Mouse
//.
//---------------------------------------------------------------------------
void __fastcall TClipMaker::BPStartDrag(TObject *Sender, TDragObject *&DragObject)
{
	TMxPanel* P = dynamic_cast<TMxPanel*>(Sender); VERIFY(P);
	drag_obj	= P->Tag;
	RepaintClips();
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::BPEndDrag(TObject *Sender, TObject *Target, int X, int Y)
{
	drag_obj	= 0xFFFF;
	RepaintClips();
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::BPDragOver(TObject *Sender, TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
    Accept = false;
    if (Sender==Source)
    {
        CAnimationClip* clip = FindClip(X);
        Accept = (clip&&(clip!=sel_clip));
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::BPDragDrop(TObject *Sender, TObject *Source,
      int X, int Y)
{
    TMxPanel* P = dynamic_cast<TMxPanel*>(Source); VERIFY(P);
    CAnimationClip* tgt = FindClip(X); VERIFY(tgt);
    CAnimationClip* src = sel_clip;
    if (P->Tag==-2)
    {
    	R_ASSERT(0);
    		/*
        if (drag_state.Contains(ssAlt))
        {
        	std::swap		(tgt->fx,src->fx);
        }else if (drag_state.Contains(ssCtrl))
        {
            tgt->fx 		= src->fx;
        }else{
            tgt->fx 		= src->fx;
            src->fx.clear	();
        }     */
    }else{
        if (drag_state.Contains(ssAlt))
        {
        	std::swap		(tgt->animItems[P->Tag], src->animItems[P->Tag]);
        }else if (drag_state.Contains(ssCtrl))
        {
            tgt->animItems[P->Tag] = src->animItems[P->Tag];
        }else{
            tgt->animItems[P->Tag] = src->animItems[P->Tag];
            src->animItems[P->Tag].clear();
        }
    }
    RepaintClips();
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::BPMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    TMxPanel* P = dynamic_cast<TMxPanel*>(Sender);
    if (P){
        if (Button==mbRight){
//            TPoint pt; pt.x = X; pt.y = Y;
//            pt=P->ClientToScreen(pt);
//            pmClip->Popup(pt.x,pt.y-10);
        }else if (Button==mbLeft){
	        SelectClip	(FindClip(X));
            P->BeginDrag(false, 2);
			drag_state = Shift;	
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::BPMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	drag_state = Shift;	
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::OnClipItemFocused(ListItemsVec& items)
{
	if (!items.empty()){
	    for (ListItemsIt it=items.begin(); it!=items.end(); it++){
            ListItem* prop = *it;
            m_ClipList->LockUpdating();
            SelectClip((CAnimationClip*)prop->m_Object);
            m_ClipList->UnlockUpdating();
        }
        if (sel_clip)
            sbBase->HorzScrollBar->Position = sbBase->HorzScrollBar->Range * (sel_clip->StartTime()/m_TotalLength);
    }
}
//------------------------------------------------------------------------------

void __fastcall TClipMaker::OnNameChange(PropValue* V)
{
	VERIFY(sel_clip);
    RepaintClips();
}
//------------------------------------------------------------------------------

void __fastcall TClipMaker::OnClipLengthChange(PropValue* V)
{
	UpdateClips		();
}
//------------------------------------------------------------------------------

void __fastcall TClipMaker::OnZoomChange(PropValue* V)
{
	UpdateClips		();
}
//---------------------------------------------------------------------------

void TClipMaker::RealUpdateProperties()
{
	m_RTFlags.set	(flRT_UpdateProperties,FALSE);
    // clip props
    PropItemVec		p_items;
    PropValue* V	= 0;
	PHelper().CreateCaption		(p_items,"Length",				FloatTimeToStrTime(m_TotalLength,true,true,true,true).c_str());
    V=PHelper().CreateFloat		(p_items,"Zoom",				&m_Zoom,			1.f,1000.f,0.1f,1);
    V->OnChangeEvent.bind		(this,&TClipMaker::OnZoomChange);
    if (sel_clip){
    	ListItem* l_owner		= m_ClipList->FindItem(*sel_clip->name); VERIFY(l_owner);
	    V=PHelper().CreateName	(p_items,"Current Clip\\Name",	&sel_clip->name,	l_owner);
        V->OnChangeEvent.bind	(this,&TClipMaker::OnNameChange);
	    V=PHelper().CreateFloat	(p_items,"Current Clip\\Length",&sel_clip->length,	0.f,10000.f,0.1f,2);
        V->OnChangeEvent.bind	(this,&TClipMaker::OnClipLengthChange);
//TEMP        
/*
        for (u16 k=0; k<4; k++)
        {
            AnsiString mname	= sel_clip->CycleName(k);	
            u16 slot			= sel_clip->CycleSlot(k);
            if (mname.IsEmpty())
            	continue;
                
            CMotionDef* MD		= m_RenderObject->FindMotionDef		(mname.c_str(),slot);
            CMotion* MI			= m_RenderObject->FindMotionKeys	(mname.c_str(),slot);
            SBonePart* BP		= (k<(u16)m_CurrentObject->BoneParts().size())?&m_CurrentObject->BoneParts()[k]:0;

            shared_str tmp;
            if (MI)				
            	tmp.sprintf("%s [%3.2fs, %s]",mname.c_str(),MI->GetLength()/MD->Speed(),MD->bone_or_part?"stop at end":"looped");
                
            if (BP)				
            	PHelper().CreateCaption	(p_items,PrepareKey("Current Clip\\Cycles",BP->alias.c_str()), tmp);
		}            
                
        if (sel_clip->fx.valid())
        	PHelper().CreateFloat		(p_items,PrepareKey("Current Clip\\FXs",*sel_clip->fx.name), &sel_clip->fx_power, 0.f, 1000.f);
*/            
    }
	m_ClipProps->AssignItems(p_items);
}
//---------------------------------------------------------------------------

void TClipMaker::SelectClip(CAnimationClip* clip)
{
	if (sel_clip!=clip)
    {
        AnsiString nm			= clip?*clip->name:"";
        sel_clip				= clip;
        m_ClipList->SelectItem	(nm.c_str(),true,false,true);
        RepaintClips			();
        UpdateProperties		();
    }
}

void TClipMaker::InsertClip()
{
	shared_str nm;
    m_ClipList->GenerateObjectName	(nm,0,"clip",true);
	CAnimationClip* clip			= xr_new<CAnimationClip>(*nm,this);
    clip->start_time				= (sel_clip)?sel_clip->StartTime()-EPS_L:0;
    clips.push_back					(clip);
    UpdateClips		                (true,false);     
    SelectClip		                (clip);
}
//---------------------------------------------------------------------------

void TClipMaker::AppendClip()
{
	shared_str nm;
    m_ClipList->GenerateObjectName	(nm,0,"clip",true);
	CAnimationClip* clip			= xr_new<CAnimationClip>(*nm,this);
    clip->start_time				= (sel_clip)?sel_clip->StartTime()+sel_clip->Length()-EPS_L:0;
    clips.push_back					(clip);
    UpdateClips						(true,false);
    SelectClip						(clip);
}
//---------------------------------------------------------------------------

#define	CHUNK_ZOOM	0x9000
#define	CHUNK_CLIPS	0x9001

void TClipMaker::LoadClips()
{
    bool bRes=true;
	if (EFS.GetOpenName("$clips$",m_ClipFName))
    {
    	Clear		();
    	IReader* F	= FS.r_open(m_ClipFName.c_str()); VERIFY(F);
        m_ClipFName	= EFS.ExcludeBasePath(m_ClipFName.c_str(),FS.get_path("$clips$")->m_Path);
        if (F->find_chunk(CHUNK_ZOOM))
        {
        	m_Zoom	= F->r_float();
        }
        IReader* C 	= F->open_chunk(CHUNK_CLIPS);
        if(C){
            IReader* M   = C->open_chunk(0);
            for (int count=1; M; ++count) 
            {
                CAnimationClip* clip	= xr_new<CAnimationClip>(this);
                if (!clip->Load(*M))
                {
                    ELog.Msg(mtError,"Unsupported clip version. Load failed.");
                    xr_delete(clip);
                    bRes = false;
                }
                M->close();
                if (!bRes)	break;
                clips.push_back(clip);
                M = C->open_chunk(count);
            }
            C->close	();
            UpdateClips	();
        }
        FS.r_close(F);
    }
}
//---------------------------------------------------------------------------

void TClipMaker::SaveClips()
{
    if (!clips.empty()){
        if (EFS.GetSaveName("$clips$",m_ClipFName))
        {
            IWriter* F	= FS.w_open(m_ClipFName.c_str()); VERIFY(F);
	        m_ClipFName	= EFS.ExcludeBasePath(m_ClipFName.c_str(),FS.get_path("$clips$")->m_Path);
            if (F)
            {
                F->open_chunk(CHUNK_ZOOM);
                F->w_float	(m_Zoom);
                F->close_chunk();

                F->open_chunk	(CHUNK_CLIPS);
                int count = 0;
                for (AnimClipIt c_it=clips.begin(); c_it!=clips.end(); ++c_it)
                {
                    F->open_chunk(count); count++;
                    (*c_it)->Save(*F);
                    F->close_chunk();
                }
                F->close_chunk	();
                FS.w_close(F);
            }else
            {
		        Log			("!Can't save clip:",m_ClipFName.c_str());
            }
        }
    }else{
    	ELog.DlgMsg(mtError,"Clip list empty.");
    }
}
//---------------------------------------------------------------------------

void TClipMaker::RemoveAllClips()
{
	SelectClip		(0);
	for (AnimClipIt it=clips.begin(); it!=clips.end(); it++)
    	xr_delete(*it);
        
    clips.clear		();
    UpdateClips		();
    Stop			();
}
//---------------------------------------------------------------------------

void TClipMaker::RemoveClip(CAnimationClip* clip)
{
	if (clip){
    	Stop		();
    	AnimClipIt it 	= std::find(clips.begin(),clips.end(),clip);
        if (it!=clips.end())
        {
        	AnimClipIt p_it	= it; 
            p_it++;
            if ((p_it==clips.end())&&(clips.size()>1))
            	{ p_it=it; p_it--;}
                
            CAnimationClip* C	= p_it==clips.end()?0:*p_it;
            xr_delete	(*it);
            clips.erase	(it);
            SelectClip	(C);
            UpdateClips	();
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::gtClipPaint(TObject *Sender)
{
	TCanvas* canvas 	= gtClip->Canvas;
    canvas->Font->Name 	= "MS Sans Serif";
    canvas->Font->Style	= TFontStyles();
	canvas->Pen->Color 	= clBlack;
    canvas->Pen->Width	= 1;
    canvas->Pen->Style	= psSolid;  
	for (AnimClipIt it=clips.begin(); it!=clips.end(); ++it)
    {
        canvas->MoveTo	((*it)->PLeftUI(), 0);
        canvas->LineTo	((*it)->PLeftUI(), 6);
		AnsiString s	= AnsiString().sprintf("%2.1f",(*it)->StartTime());
        float dx		= 2.f;
        float dy		= canvas->TextHeight(s);
        TRect R 		= TRect((*it)->PLeftUI()+1-dx, 20-dy, (*it)->PRightUI()-dx, 20);
        canvas->TextRect(R,R.Left,R.Top,s);
	}
    if (!clips.empty())
    {
    	CAnimationClip* C = clips.back();
        canvas->MoveTo	(C->PRightUI()-1, 0);
        canvas->LineTo	(C->PRightUI()-1, 6);
		AnsiString s	= AnsiString().sprintf("%2.1f",m_TotalLength);
        float dx		= canvas->TextWidth(s);
        float dy		= canvas->TextHeight(s);
        TRect R 		= TRect(C->PRightUI()-dx, 20-dy, C->PRightUI(), 20);
        canvas->TextRect(R,R.Left,R.Top,s);
    }
/*    if (g_resizing){
    	canvas->Pen->Color = clGreen;
        canvas->MoveTo	(g_X_cur, 0);
        canvas->LineTo	(g_X_cur, gtClip->Width);
    }
*/
    if (m_RTFlags.is(flRT_Playing)){
        canvas->Pen->Color 	= clRed;
        canvas->Pen->Width	= 3;
        canvas->MoveTo		(m_CurrentPlayTime*m_Zoom, 0);
        canvas->LineTo		(m_CurrentPlayTime*m_Zoom, gtClip->Width);
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ClipPaint(TObject *Sender)
{
	TMxPanel* P 		= dynamic_cast<TMxPanel*>(Sender); VERIFY(P);
    TCanvas* canvas 	= P->Canvas;
    canvas->Font->Name 	= "MS Sans Serif";
    canvas->Font->Style	= TFontStyles();
    canvas->Font->Color = clBlack;
    canvas->Pen->Color 	= clBlack;
    canvas->Pen->Style	= psSolid;
    canvas->Brush->Style= bsSolid;
    for (AnimClipIt it=clips.begin(); it!=clips.end(); ++it)
    {
        TRect R 			= TRect((*it)->PLeftUI(), 1, (*it)->PRightUI()-1, paClips->Height);
        canvas->Pen->Width	= 1;
        canvas->Brush->Color= (*it==sel_clip)?(drag_obj==P->Tag?CLIP_ACTIVE_DRAG_COLOR:CLIP_ACTIVE_COLOR):CLIP_INACTIVE_COLOR;
        canvas->Rectangle	(R);
        R.Top				+= 1;
        R.Bottom			-= 1;
        R.Left				+= 1;
        R.Right				-= 1;
        canvas->TextRect	(R,R.Left,R.Top,*(*it)->name);
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::BPOnPaint(TObject *Sender)
{
	TMxPanel* bp 		= dynamic_cast<TMxPanel*>(Sender); VERIFY(bp);

//.    CEditableObject* O	= 	m_CurrentObject; VERIFY(O);
    
    TCanvas* canvas 	= bp->Canvas;
    canvas->Font->Name 	= "MS Sans Serif";
    canvas->Font->Style	= TFontStyles();
    canvas->Font->Color = clBlack;
    canvas->Pen->Color	= clBlack;
    canvas->Pen->Style	= psSolid;
    canvas->Brush->Style= bsSolid;
    if (-2==bp->Tag)
    {
    	R_ASSERT(0);
    	/*
        for (UIClipIt it=clips.begin(); it!=clips.end(); it++)
        {
	        canvas->Brush->Color= (*it==sel_clip)?(drag_obj==bp->Tag?BP_ACTIVE_DRAG_COLOR:BP_ACTIVE_COLOR):BP_INACTIVE_COLOR;
            TRect R 			= TRect((*it)->PLeft(), 1, (*it)->PRight()-1, 15);
    	    AnsiString fx_name	= (*it)->FXName();
            if (!fx_name.IsEmpty()){
                canvas->Rectangle	(R);
                R.Top				+= 1;
                R.Bottom			-= 1;
                R.Left				+= 1;
                R.Right				-= 1;
                canvas->TextRect	(R,R.Left,R.Top,fx_name);
            }
        }
        */
    }else if ((bp->Tag>=0) && (bp->Tag < MAX_PARTS))
    {
        AnsiString mn_prev		= "";
        for (AnimClipIt it=clips.begin(); it!=clips.end(); ++it)
        {
			std::pair<LPCSTR,LPCSTR> name_pair = m_RenderObject->LL_MotionDefName_dbg( (*it)->animItems[bp->Tag].mid );
            AnsiString mn		= name_pair.first;
            TRect R 			= TRect((*it)->PLeftUI(), 1, (*it)->PRightUI()-1, 15);
            if (!mn.IsEmpty())
            {
                canvas->Brush->Color= (*it==sel_clip)?(drag_obj==bp->Tag?BP_ACTIVE_DRAG_COLOR:BP_ACTIVE_COLOR):BP_INACTIVE_COLOR;
                canvas->Rectangle	(R);
                R.Top				+= 1;
                R.Bottom			-= 1;
                R.Left				+= 1;
                R.Right				-= 1;
                canvas->TextRect	(R,R.Left,R.Top,mn);
	            mn_prev				= mn;
            }else if (!mn_prev.IsEmpty())
            {
	            canvas->MoveTo		((*it)->PLeftUI()+1,13);
                canvas->LineTo		(R.Right,13);
                canvas->LineTo		(R.Width()>5?R.Right-5:R.Right-R.Width(),8);
                R.Top				+= 1;
                R.Bottom			-= 1;
                R.Left				+= 1;
                R.Right				-= 1;
            }
        }
    }
}
//---------------------------------------------------------------------------

void TClipMaker::RealRepaintClips()
{
    m_RTFlags.set		(flRT_RepaintClips,FALSE);
    // repaint
    paClips->Repaint	();
    gtClip->Repaint		();
    paBP0->Repaint		();
    paBP1->Repaint		();
    paBP2->Repaint		();
    paBP3->Repaint		();          
    paFXs->Repaint		();

	// set BP name                   
    CPartition* P		= m_RenderObject->m_Partition;
    for (u16 k=0; k<MAX_PARTS; ++k)
        m_LB[k]->Caption = (P->part(k).Name.size())?P->part(k).Name.c_str():"-";

    UpdateProperties	();
}
//---------------------------------------------------------------------------

void TClipMaker::RealUpdateClips()
{
	m_RTFlags.set	(flRT_UpdateClips,FALSE);
    m_TotalLength	= 0.f;
    std::sort		(clips.begin(),clips.end(),clip_pred);
	for (AnimClipIt it=clips.begin(); it!=clips.end(); ++it)
    {
    	(*it)->start_time	= m_TotalLength;
        m_TotalLength		+= (*it)->length;
        (*it)->idx			= it-clips.begin();
    }
	paFrame->Width			= m_TotalLength*m_Zoom;
    Stop					();
    // clip list
    ListItemsVec			l_items;
    
    for (it=clips.begin(); it!=clips.end(); ++it)
    	LHelper().CreateItem	(l_items,*(*it)->name,0,0,*it);
        
	m_ClipList->AssignItems		(l_items,true);
    
	// select default clip
 	if (!clips.empty()&&(sel_clip==0)) 
    	SelectClip(clips[0]);
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::fsStorageRestorePlacement(TObject *Sender)
{
	m_ClipProps->RestoreParams	(fsStorage); 
    int idx 					= fsStorage->ReadInteger("sel_clip",0);
    if (idx<(int)clips.size())	SelectClip(clips[idx]);
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::fsStorageSavePlacement(TObject *Sender)
{
	m_ClipProps->SaveParams(fsStorage);
    fsStorage->WriteInteger		("sel_clip",sel_clip?sel_clip->idx:0);
}
//---------------------------------------------------------------------------

void TClipMaker::PlayAnimation(CAnimationClip* clip)
{
    u8 pc 				= m_RenderObject->partitions().count();
    for (u8 k=0; k<pc; ++k)
    	if (clip->animItems[k].valid())
        	m_RenderObject->LL_PlayCycle(k,clip->animItems[k].mid,FALSE,0,0,0);
}
//---------------------------------------------------------------------------

void TClipMaker::OnFrame()
{
	if (m_RTFlags.is(flRT_UpdateClips))
    	RealUpdateClips();
	if (m_RTFlags.is(flRT_RepaintClips))
    	RealRepaintClips();
    if (m_RTFlags.is(flRT_UpdateProperties))
    	RealUpdateProperties();
    if (m_RTFlags.is(flRT_Playing))
    {
    	// playing
        VERIFY(play_clip<clips.size());
        if (m_CurrentPlayTime>(clips[play_clip]->StartTime()+clips[play_clip]->Length()))
        {
        	play_clip++;
            if (play_clip>=clips.size())
            { 
			    if (m_RTFlags.is(flRT_PlayingLooped))
                {
    	        	play_clip=0;
                }else{
                	Stop();
                }
            }
		    if (m_RTFlags.is(flRT_Playing)) 
            	PlayAnimation(clips[play_clip]);
        }
		// play onframe
    	if (m_CurrentPlayTime>m_TotalLength) 
        	m_CurrentPlayTime-=m_TotalLength;
            
	    m_CurrentPlayTime+=EDevice.fTimeDelta;

        gtClip->Repaint();
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebInsertClipClick(TObject *Sender)
{
	InsertClip		();
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebAppendClipClick(TObject *Sender)
{
	AppendClip		();	
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebLoadClipsClick(TObject *Sender)
{
	LoadClips		();
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebSaveClipsClick(TObject *Sender)
{
	SaveClips		();
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebSyncClick(TObject *Sender)
{
//TEMP
/*
    if (ATools->IsEngineMode())
    {
        for (UIClipIt c_it=clips.begin(); c_it!=clips.end(); c_it++)
        {
            float len = 0.f;
            for (u16 k=0; k<4; k++)
            {
                AnsiString mname	= (*c_it)->CycleName(k);	
                u16 slot			= (*c_it)->CycleSlot(k);	
				CMotion* MI			= ATools->m_RenderObject.FindMotionKeys	(mname.c_str(),slot);
				CMotionDef* MD		= ATools->m_RenderObject.FindMotionDef	(mname.c_str(),slot);
				if (MI)
                {
                	float new_len	= (MD->StopAtEnd()?MI->GetLength()-SAMPLE_SPF:MI->GetLength())/MD->Speed();
	                if (len<new_len) 
                    	len = new_len;
                }
            }            
            (*c_it)->length = fis_zero(len)?2.f:len;
        }
        UpdateClips		();
    }else{
        ELog.DlgMsg(mtInformation,"Time syncronize only in Engine Mode.");
    }
*/    
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebPrevClipClick(TObject *Sender)
{
	if (sel_clip){
		AnimClipIt it = std::find(clips.begin(),clips.end(),sel_clip);
        if (it!=clips.begin()){
	        it--;
            SelectClip(*it);
    	}    
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebNextClipClick(TObject *Sender)
{
	if (sel_clip)
    {
		AnimClipIt it = std::find(clips.begin(),clips.end(),sel_clip);
        if (it!=clips.end())
        {
	        it++;
	        if (it!=clips.end())
	            SelectClip(*it);
    	}    
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebPlayClick(TObject *Sender)
{
    Play		(FALSE);
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebPlayCycleClick(TObject *Sender)
{
	Play		(TRUE);
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebStopClick(TObject *Sender)
{
    Stop		();
}
//---------------------------------------------------------------------------

void TClipMaker::Play(BOOL bLoop)
{
    if (sel_clip)
    {
        m_RTFlags.set	(flRT_Playing,TRUE);
        m_RTFlags.set	(flRT_PlayingLooped,bLoop);
        play_clip		= sel_clip->idx;
        m_CurrentPlayTime=sel_clip->start_time;
        PlayAnimation	(sel_clip);
    }
}
//---------------------------------------------------------------------------

void TClipMaker::Stop()
{
	if(m_RTFlags.is(flRT_Playing))
    {
        m_RTFlags.set					(flRT_Playing,FALSE);
        m_CurrentPlayTime				= 0.f;
        RepaintClips					();

        for (u16 i=0; i<MAX_PARTS; ++i)
           m_RenderObject->LL_CloseCycle(i, u8(-1));
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebTrashClick(TObject *Sender)
{
	if (!clips.empty())
    	if (mrYes==ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbNo, "Remove selected clip?"))
        	RemoveClip	(sel_clip);
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebTrashDragOver(TObject *Sender,
      TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
	Accept 		= false;
    TMxPanel* P = dynamic_cast<TMxPanel*>(Source);
    Accept		= !!P;
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebTrashDragDrop(TObject *Sender,
      TObject *Source, int X, int Y)
{
    TMxPanel* P = dynamic_cast<TMxPanel*>(Source);
    if (P==paClips){
	    RemoveClip(sel_clip);
    }else
    {
        sel_clip->animItems[P->Tag].clear();
        UpdateClips();
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebClearClick(TObject *Sender)
{
    if (mrYes==ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbCancel, "Remove all clips?"))
		Clear();
}
//---------------------------------------------------------------------------

