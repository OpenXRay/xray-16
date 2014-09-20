#include "stdafx.h"
#pragma hdrstop

#include "ClipEditor.h"
#include <ElVCLUtils.hpp>
#include <ElTools.hpp>

#include "ElTree.hpp"
#include "motion.h"
//.#include "skeletoncustom.h"
#include "../ECore/Editor/editobject.h"
#include "UI_ActorTools.h"
#include "../ECore/Editor/UI_Main.h"
//.#include "SkeletonAnimated.h"
#include "../xrEProps/ItemList.h"

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

TClipMaker::CUIClip::CUIClip(LPCSTR n, TClipMaker* own, float r_t) 
{
	owner				= own;
	run_time			= r_t;
	length				= 2.f;
    name				= n;
    idx					= -1;
    fx_power			= 1.f;
}

TClipMaker::CUIClip::~CUIClip()
{
};

void TClipMaker::CUIClip::SetCycle(LPCSTR name, u16 part, u16 slot)
{
    if (part==BI_NONE){
        for (int k=0; k<4; k++)	cycles[k].set(name,slot);
    }else{
        cycles[part].set(name,slot);
    }
}

void TClipMaker::CUIClip::SetFX(LPCSTR name, u16 slot)
{
    fx.set(name,slot); 
}

IC bool clip_pred(TClipMaker::CUIClip* x, TClipMaker::CUIClip* y)
{
	return x->run_time<y->run_time;
};

IC bool clip_pred_float(float x, TClipMaker::CUIClip* y)
{
	return x<y->run_time;
};

TClipMaker*	TClipMaker::CreateForm()
{
	return xr_new<TClipMaker>((TComponent*)0);
}

void TClipMaker::DestroyForm(TClipMaker* form)
{
	xr_delete(form);
}
    
void TClipMaker::ShowEditor(CEditableObject* O)
{
	m_CurrentObject = O; VERIFY(O);
	Show			();
    UpdateClips		();
    UpdateProperties();
}

void TClipMaker::HideEditor()
{
	m_CurrentObject = 0;
	Clear			();
	Hide			();
}

void TClipMaker::Clear()
{
	m_ClipProps->ClearProperties();
	m_ClipList->ClearList		();
	Stop			();
	for (UIClipIt it=clips.begin(); it!=clips.end(); it++)
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
}
//---------------------------------------------------------------------------

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

TClipMaker::CUIClip* TClipMaker::FindClip(int x)
{
	return FindClip(float(x)/m_Zoom);
}
//---------------------------------------------------------------------------

TClipMaker::CUIClip* TClipMaker::FindClip(float t)
{
	if (clips.empty()) return 0;
	UIClipIt it = std::upper_bound(clips.begin(),clips.end(),t,clip_pred_float);
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
    Accept = false;
	TElTreeDragObject* obj 	= dynamic_cast<TElTreeDragObject*>(Source);
    if (obj){
        TMxPanel* A			= dynamic_cast<TMxPanel*>(Sender);
        if (A==paClips){
            TElTree* tv		= dynamic_cast<TElTree*>(obj->Control);
            if (tv->SelectedCount){
                for (TElTreeItem* item = tv->GetNextSelected(0); item; item = tv->GetNextSelected(item)){
                    ListItem* prop		= (ListItem*)item->Tag;
					if (prop&&(prop->Type()==emMotion)){
                        Accept			= true;
                    }
                }
            }
        }
    }else{
		Accept = (Sender==paClips);
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ClipDragDrop(TObject *Sender,
      TObject *Source, int X, int Y)
{
    VERIFY 					(Sender==paClips);
    CUIClip* tgt			= FindClip(X); VERIFY(tgt);
	TElTreeDragObject* obj 	= dynamic_cast<TElTreeDragObject*>(Source);
    if (obj){
        TElTree* tv			= dynamic_cast<TElTree*>(obj->Control);
        if (tv->SelectedCount){
            for (TElTreeItem* item 	= tv->GetNextSelected(0); item; item = tv->GetNextSelected(item)){
                ListItem* prop		= (ListItem*)item->Tag; VERIFY(prop);
                u16 bp;
                BOOL fx;
                LPCSTR m_name			= ATools->ExtractMotionName(prop->Key());
                u16 m_slot				= ATools->ExtractMotionSlot(prop->Key());
	            if (m_CurrentObject->m_SMotionRefs.size()){
		            CMotionDef* SM		= ATools->m_RenderObject.FindMotionDef(m_name,m_slot);	VERIFY(SM);
                    bp					= SM->bone_or_part;
                    fx					= SM->flags&esmFX;
                }else{
                	CSMotion* SM 		= ATools->FindMotion(m_name); VERIFY(SM);
                    bp					= SM->m_BoneOrPart;
                    fx					= SM->m_Flags.is(esmFX);
                }
                if (fx){
                	tgt->SetFX		(m_name,m_slot);
                }else{
                    tgt->SetCycle	(m_name,bp,m_slot);
                }
            }
        }
    }else{
        float rt			= float(X)/m_Zoom-tgt->RunTime();
        if (rt<tgt->Length()/2.f)	sel_clip->run_time	= tgt->run_time-EPS_L;
        else						sel_clip->run_time	= tgt->run_time+EPS_L;
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
    if (Button==mbLeft){
        SelectClip		(FindClip(X));
        if (paClips->Cursor==crHSplit){
            g_resizing	= TRUE;
            Stop		();
            g_X_prev 	= X;
            g_X_dx		= X-sel_clip->PRight();
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
    CUIClip* C			= FindClip(X); VERIFY(C);
    int cX				= X-C->PLeft();
    float w0 			= float(cX)/C->PWidth();
    int w1	 			= C->PWidth()-cX;
	if ((w0>0.75f)&&(w0<1.f)&&(w1<7)){
    	P->Cursor 		= crHSplit;
    }else{
    	if (!g_resizing)
	    	P->Cursor 	= crDefault;
    }
    if (g_resizing){
        float dx		= float(X-(g_X_prev+g_X_dx))/m_Zoom;
        if (!fis_zero(dx)){
    	    sel_clip->length += dx;
            if (sel_clip->length<0.01f) sel_clip->length=0.01f;
    		g_X_prev 	= sel_clip->PRight();
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
    if (Sender==Source){
        CUIClip* clip = FindClip(X);
        Accept = (clip&&(clip!=sel_clip));
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::BPDragDrop(TObject *Sender, TObject *Source,
      int X, int Y)
{
    TMxPanel* P = dynamic_cast<TMxPanel*>(Source); VERIFY(P);
    CUIClip* tgt = FindClip(X); VERIFY(tgt);
    CUIClip* src = sel_clip;
    if (P->Tag==-2){
        if (drag_state.Contains(ssAlt)){
        	std::swap		(tgt->fx,src->fx);
        }else if (drag_state.Contains(ssCtrl)){
            tgt->fx 		= src->fx;
        }else{
            tgt->fx 		= src->fx;
            src->fx.clear	();
        }
    }else{
        if (drag_state.Contains(ssAlt)){
        	std::swap		(tgt->cycles[P->Tag],src->cycles[P->Tag]);
        }else if (drag_state.Contains(ssCtrl)){
            tgt->cycles[P->Tag] = src->cycles[P->Tag];
        }else{
            tgt->cycles[P->Tag] = src->cycles[P->Tag];
            src->cycles[P->Tag].clear();
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
            SelectClip((CUIClip*)prop->m_Object);
            m_ClipList->UnlockUpdating();
        }
        if (sel_clip)
            sbBase->HorzScrollBar->Position = sbBase->HorzScrollBar->Range*(sel_clip->RunTime()/m_TotalLength);
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
        for (u16 k=0; k<4; k++){
            AnsiString mname	= sel_clip->CycleName(k);	
            u16 slot			= sel_clip->CycleSlot(k);
            if (mname.IsEmpty())continue;
            CMotionDef* MD		= ATools->m_RenderObject.FindMotionDef	(mname.c_str(),slot);
            CMotion* MI			= ATools->m_RenderObject.FindMotionKeys	(mname.c_str(),slot);
            SBonePart* BP		= (k<(u16)m_CurrentObject->BoneParts().size())?&m_CurrentObject->BoneParts()[k]:0;
            shared_str tmp;
            if (MI)				tmp.sprintf("%s [%3.2fs, %s]",mname.c_str(),MI->GetLength()/MD->Speed(),MD->bone_or_part?"stop at end":"looped");
            if (BP)				PHelper().CreateCaption	(p_items,PrepareKey("Current Clip\\Cycles",BP->alias.c_str()), tmp);
		}            
        if (sel_clip->fx.valid())PHelper().CreateFloat		(p_items,PrepareKey("Current Clip\\FXs",*sel_clip->fx.name), &sel_clip->fx_power, 0.f, 1000.f);
    }
	m_ClipProps->AssignItems(p_items);
}
//---------------------------------------------------------------------------

void TClipMaker::SelectClip(CUIClip* clip)
{
	if (sel_clip!=clip){
        AnsiString nm	= clip?*clip->name:"";
        sel_clip		= clip;
        m_ClipList->SelectItem(nm.c_str(),true,false,true);
        RepaintClips	();
        UpdateProperties();
    }
}

void TClipMaker::InsertClip()
{
	shared_str nm;
    m_ClipList->GenerateObjectName	(nm,0,"clip",true);
	CUIClip* clip	= xr_new<CUIClip>(*nm,this,sel_clip?sel_clip->RunTime()-EPS_L:0);
    clips.push_back	(clip);
    UpdateClips		(true,false);     
    SelectClip		(clip);
}
//---------------------------------------------------------------------------

void TClipMaker::AppendClip()
{
	shared_str nm;
    m_ClipList->GenerateObjectName	(nm,0,"clip",true);
	CUIClip* clip	= xr_new<CUIClip>(*nm,this,sel_clip?sel_clip->RunTime()+sel_clip->Length()-EPS_L:0);
    clips.push_back	(clip);
    UpdateClips		(true,false);
    SelectClip		(clip);
}
//---------------------------------------------------------------------------

#define	CHUNK_ZOOM	0x9000
#define	CHUNK_CLIPS	0x9001

void TClipMaker::LoadClips()
{
    bool bRes=true;
	if (EFS.GetOpenName("$clips$",m_ClipFName)){
    	Clear		();
    	IReader* F	= FS.r_open(m_ClipFName.c_str()); VERIFY(F);
        m_ClipFName	= EFS.ExcludeBasePath(m_ClipFName.c_str(),FS.get_path("$clips$")->m_Path);
        if (F->find_chunk(CHUNK_ZOOM)){
        	m_Zoom	= F->r_float();
        }
        IReader* C 	= F->open_chunk(CHUNK_CLIPS);
        if(C){
            IReader* M   = C->open_chunk(0);
            for (int count=1; M; count++) {
                CUIClip* clip	= xr_new<CUIClip>(this,count);
                if (!clip->Load(*M)){
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
        if (EFS.GetSaveName("$clips$",m_ClipFName)){
            IWriter* F	= FS.w_open(m_ClipFName.c_str()); VERIFY(F);
	        m_ClipFName	= EFS.ExcludeBasePath(m_ClipFName.c_str(),FS.get_path("$clips$")->m_Path);
            if (F){
                F->open_chunk(CHUNK_ZOOM);
                F->w_float	(m_Zoom);
                F->close_chunk();

                F->open_chunk	(CHUNK_CLIPS);
                int count = 0;
                for (UIClipIt c_it=clips.begin(); c_it!=clips.end(); c_it++){
                    F->open_chunk(count); count++;
                    (*c_it)->Save(*F);
                    F->close_chunk();
                }
                F->close_chunk	();
                FS.w_close(F);
            }else{
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
	for (UIClipIt it=clips.begin(); it!=clips.end(); it++)
    	xr_delete(*it);
    clips.clear		();
    UpdateClips		();
    Stop			();
}
//---------------------------------------------------------------------------

void TClipMaker::RemoveClip(CUIClip* clip)
{
	if (clip){
    	Stop		();
    	UIClipIt it 	= std::find(clips.begin(),clips.end(),clip);
        if (it!=clips.end()){
        	UIClipIt p_it	= it; p_it++;
            if ((p_it==clips.end())&&(clips.size()>1)){ p_it=it; p_it--;}
            CUIClip* C	= p_it==clips.end()?0:*p_it;
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
	for (UIClipIt it=clips.begin(); it!=clips.end(); it++){
        canvas->MoveTo	((*it)->PLeft(), 0);
        canvas->LineTo	((*it)->PLeft(), 6);
		AnsiString s	= AnsiString().sprintf("%2.1f",(*it)->RunTime());
        float dx		= 2.f;
        float dy		= canvas->TextHeight(s);
        TRect R 		= TRect((*it)->PLeft()+1-dx, 20-dy, (*it)->PRight()-dx, 20);
        canvas->TextRect(R,R.Left,R.Top,s);
	}
    if (!clips.empty()){
    	CUIClip* C		= clips.back();
        canvas->MoveTo	(C->PRight()-1, 0);
        canvas->LineTo	(C->PRight()-1, 6);
		AnsiString s	= AnsiString().sprintf("%2.1f",m_TotalLength);
        float dx		= canvas->TextWidth(s);
        float dy		= canvas->TextHeight(s);
        TRect R 		= TRect(C->PRight()-dx, 20-dy, C->PRight(), 20);
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
    for (UIClipIt it=clips.begin(); it!=clips.end(); it++){
        TRect R 		= TRect((*it)->PLeft(), 1, (*it)->PRight()-1, paClips->Height);
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
    CEditableObject* O	= 	m_CurrentObject; VERIFY(O);
    TCanvas* canvas 	= bp->Canvas;
    canvas->Font->Name 	= "MS Sans Serif";
    canvas->Font->Style	= TFontStyles();
    canvas->Font->Color = clBlack;
    canvas->Pen->Color	= clBlack;
    canvas->Pen->Style	= psSolid;
    canvas->Brush->Style= bsSolid;
    if (-2==bp->Tag){
        for (UIClipIt it=clips.begin(); it!=clips.end(); it++){
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
    }else if ((bp->Tag>=0)&&(bp->Tag<(int)O->BoneParts().size())){
        AnsiString mn_prev		= "";
        for (UIClipIt it=clips.begin(); it!=clips.end(); it++){
            AnsiString mn		= (*it)->CycleName(u16(bp->Tag));
            TRect R 			= TRect((*it)->PLeft(), 1, (*it)->PRight()-1, 15);
            if (!mn.IsEmpty()){
                canvas->Brush->Color= (*it==sel_clip)?(drag_obj==bp->Tag?BP_ACTIVE_DRAG_COLOR:BP_ACTIVE_COLOR):BP_INACTIVE_COLOR;
                canvas->Rectangle	(R);
                R.Top				+= 1;
                R.Bottom			-= 1;
                R.Left				+= 1;
                R.Right				-= 1;
                canvas->TextRect	(R,R.Left,R.Top,mn);
	            mn_prev				= mn;
            }else if (!mn_prev.IsEmpty()){
	            canvas->MoveTo		((*it)->PLeft()+1,13);
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
    CEditableObject* O	= m_CurrentObject;
    u32 k				= 0;
    if (O){
	    BPVec& bps 		= O->BoneParts();
        for (; k<bps.size(); k++)
        	m_LB[k]->Caption = bps[k].alias.c_str();
    }
	for (; k<4; k++)	m_LB[k]->Caption	= "-";
    UpdateProperties	();
}
//---------------------------------------------------------------------------

void TClipMaker::RealUpdateClips()
{
	m_RTFlags.set	(flRT_UpdateClips,FALSE);
    m_TotalLength	= 0.f;
    std::sort		(clips.begin(),clips.end(),clip_pred);
	for (UIClipIt it=clips.begin(); it!=clips.end(); it++){
    	(*it)->run_time	= m_TotalLength;
        m_TotalLength	+= (*it)->length;
        (*it)->idx	= it-clips.begin();
    }
	paFrame->Width	= m_TotalLength*m_Zoom;
    timeTrackBar->Width = paFrame->Width;
    timeTrackBar->Min = 0;
    timeTrackBar->Max = m_TotalLength*10000;
    Stop			();
    // clip list
    ListItemsVec	l_items;
    for (it=clips.begin(); it!=clips.end(); it++)
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

void TClipMaker::PlayAnimation(CUIClip* clip)
{
    for (u32 k=0; k<m_CurrentObject->BoneParts().size(); k++)
    	if (clip->cycles[k].valid())
        	ATools->m_RenderObject.PlayCycle(*clip->cycles[k].name,k,clip->cycles[k].slot);
    if (clip->fx.valid()) ATools->m_RenderObject.PlayFX(*clip->fx.name,clip->fx_power,clip->fx.slot);
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
    if (m_RTFlags.is(flRT_Playing)){
    	// playing
        VERIFY(play_clip<clips.size());
        if (m_CurrentPlayTime>(clips[play_clip]->RunTime()+clips[play_clip]->Length())){
        	play_clip++;
            if (play_clip>=clips.size()){ 
			    if (m_RTFlags.is(flRT_PlayingLooped)){
    	        	play_clip=0;
                }else{
                	Stop();
                }
            }
		    if (m_RTFlags.is(flRT_Playing)) PlayAnimation(clips[play_clip]);
        }
		// play onframe
    	if (m_CurrentPlayTime>m_TotalLength) m_CurrentPlayTime-=m_TotalLength;
	    m_CurrentPlayTime+=EDevice.fTimeDelta;
        timeTrackBar->Position = m_CurrentPlayTime*10000;
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
    if (ATools->IsEngineMode()){
        for (UIClipIt c_it=clips.begin(); c_it!=clips.end(); c_it++){
            float len = 0.f;
            for (u16 k=0; k<4; k++){
                AnsiString mname	= (*c_it)->CycleName(k);	
                u16 slot			= (*c_it)->CycleSlot(k);	
				CMotion* MI			= ATools->m_RenderObject.FindMotionKeys	(mname.c_str(),slot);
				CMotionDef* MD		= ATools->m_RenderObject.FindMotionDef	(mname.c_str(),slot);
				if (MI){
                	float new_len	= (MD->StopAtEnd()?MI->GetLength()-SAMPLE_SPF:MI->GetLength())/MD->Speed();
	                if (len<new_len) len = new_len;
                }
            }            
            (*c_it)->length = fis_zero(len)?2.f:len;
        }
        UpdateClips		();
    }else{
        ELog.DlgMsg(mtInformation,"Time syncronize only in Engine Mode.");
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebPrevClipClick(TObject *Sender)
{
	if (sel_clip){
		UIClipIt it = std::find(clips.begin(),clips.end(),sel_clip);
        if (it!=clips.begin()){
	        it--;
            SelectClip(*it);
    	}    
    }
}
//---------------------------------------------------------------------------

void __fastcall TClipMaker::ebNextClipClick(TObject *Sender)
{
	if (sel_clip){
		UIClipIt it = std::find(clips.begin(),clips.end(),sel_clip);
        if (it!=clips.end()){
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
    if (ATools->IsEngineMode()){
        if (!ATools->m_RenderObject.m_pVisual){
            Log("!Empty visual.");
        }else{
            if (sel_clip){
                m_RTFlags.set	(flRT_Playing,TRUE);
                m_RTFlags.set	(flRT_PlayingLooped,bLoop);
                play_clip		= sel_clip->idx;
                m_CurrentPlayTime=sel_clip->run_time;
                PlayAnimation	(sel_clip);
            }
        }
    }else{
        ELog.DlgMsg(mtInformation,"Motions play only in Engine Mode.");
    }
}
//---------------------------------------------------------------------------

void TClipMaker::Stop()
{
	if (m_RTFlags.is(flRT_Playing)){
        m_RTFlags.set	(flRT_Playing,FALSE);
        m_CurrentPlayTime=0.f;
        RepaintClips	();
        ATools->m_RenderObject.StopAnimation();
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
    }else{
    	if (P->Tag==-2)	sel_clip->fx.clear();
        else 			sel_clip->cycles[P->Tag].clear();
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

