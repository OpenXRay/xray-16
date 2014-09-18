//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "EditLightAnim.h"
#include "Scene.h"
#include "LightAnimLibrary.h"
#include "../ECore/Editor/ColorPicker.h"
#include "../ECore/Editor/ui_main.h"
//---------------------------------------------------------------------------
#pragma link "multi_edit"
#pragma link "Gradient"
#pragma link "ElTrackBar"
#pragma link "ElTreeAdvEdit"
#pragma link "MxMenus"
#pragma link "MXCtrls"
#pragma link "RenderWindow"
#pragma resource "*.dfm"

TfrmEditLightAnim* TfrmEditLightAnim::form=0;

//---------------------------------------------------------------------------
__fastcall TfrmEditLightAnim::TfrmEditLightAnim(TComponent* Owner)
    : TForm(Owner)
{
    DEFINE_INI(fsStorage);
    bFinalClose		= false;
    m_CurrentItem 	= 0;
    m_CurrentOwner	= 0;
    iMoveKey        = -1;
    m_Props 		= 0;
    m_Items			= 0;
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLightAnim::FormCreate(TObject *Sender)
{
    m_Props = TProperties::CreateForm("LAProps",paProps,alClient,TOnModifiedEvent(this,&TfrmEditLightAnim::OnModified));
    m_Items	= TItemList::CreateForm("LA Items",paItems,alClient,TItemList::ilEditMenu|TItemList::ilDragAllowed|TItemList::ilFolderStore);
    m_Items->SetOnModifiedEvent		(TOnModifiedEvent(this,&TfrmEditLightAnim::OnModified));
    m_Items->SetOnItemFocusedEvent	(fastdelegate::bind<TOnILItemFocused>(this,&TfrmEditLightAnim::OnItemFocused));
    m_Items->SetOnItemRemoveEvent	(fastdelegate::bind<TOnItemRemove>(&LALib,&ELightAnimLibrary::RemoveObject));
    m_Items->SetOnItemRenameEvent	(fastdelegate::bind<TOnItemRename>(&LALib,&ELightAnimLibrary::RenameObject));
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::FormDestroy(TObject *Sender)
{
	TProperties::DestroyForm(m_Props);
    TItemList::DestroyForm(m_Items);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::fsStorageRestorePlacement(
      TObject *Sender)
{            
	m_Props->RestoreParams(fsStorage);
}                    
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::fsStorageSavePlacement(TObject *Sender)
{
	m_Props->SaveParams(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::OnModified()
{
	ebSave->Enabled = true;
    UpdateView();
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLightAnim::ShowEditor()
{
	if (!form){
    	form = xr_new<TfrmEditLightAnim>((TComponent*)0);
		Scene->lock();
    }
    form->Show();
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLightAnim::FormShow(TObject *Sender)
{
    ebSave->Enabled = false;
    UI->BeginEState(esEditLightAnim);

    InitItems();
	// check window position
	UI->CheckWindowPos(this);
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLightAnim::FormClose(TObject *Sender, TCloseAction &Action)
{
	form = 0;
	Action = caFree;
	Scene->unlock();
    UI->EndEState(esEditLightAnim);

   	if (ebSave->Enabled&&!bFinalClose)
    	LALib.Reload();
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLightAnim::FormCloseQuery(TObject *Sender,
      bool &CanClose)
{
	if (!bFinalClose) 	CanClose	= IsClose();
    else                CanClose	= true;
}
//---------------------------------------------------------------------------
bool TfrmEditLightAnim::IsClose(){
    if (ebSave->Enabled){
    	int res = ELog.DlgMsg(mtConfirmation, "Library was change. Do you want save?");
		if (res==mrCancel) return false;
		if (res==mrYes) ebSaveClick(0);
    }
    return true;
}
//---------------------------------------------------------------------------
bool TfrmEditLightAnim::FinalClose(){
	if (!form) return true;
	if (form->IsClose()){
	    form->bFinalClose=true;
		form->Close();
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::OnItemFocused(TElTreeItem* item)
{
    if (item&&FHelper.IsObject(item)){
        ListItem* prop 			= (ListItem*)item->Tag; VERIFY(prop);
        AnsiString nm			= prop->Key();
		CLAItem* I  			= LALib.FindItem(nm.c_str());
        SetCurrentItem			(I,prop);
    }else{
        SetCurrentItem			(0,0);
    }
}
//---------------------------------------------------------------------------

void TfrmEditLightAnim::InitItems()
{
	ListItemsVec items;
    for (LAItemIt it=LALib.Items.begin(); it!=LALib.Items.end(); it++)
    	LHelper().CreateItem(items,*(*it)->cName,0,0,0);
    m_Items->AssignItems(items,false,true);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key==VK_ESCAPE) 	Close();
}
//---------------------------------------------------------------------------

void TfrmEditLightAnim::UpdateView()
{
	if (m_CurrentItem){
        stStartFrame->Caption = 0;
        stEndFrame->Caption = m_CurrentItem->iFrameCount-1;
        sePointer->MaxValue	= m_CurrentItem->iFrameCount-1;
        if (sePointer->Value>sePointer->MaxValue) sePointer->Value = sePointer->MaxValue;
        sePointer->Enabled	= m_CurrentItem->iFrameCount>1;
        pbG->Repaint();

//        stStartFrame->Caption = m_CurrentItem->Keys.size();
		sePointer->Color = TColor(m_CurrentItem->IsKey(sePointer->Value)?0x00BFFFFF:0x00A0A0A0);
    }
    Caption = AnsiString().sprintf("Light Animation Library%s",ebSave->Enabled?"*":"");
}
//------------------------------------------------------------------------------
bool TfrmEditLightAnim::OnFrameCountAfterEdit  (PropValue* v, s32& val)
{
	if (val!=m_CurrentItem->iFrameCount) OnModified();
	m_CurrentItem->Resize(val);
    UpdateView();    
    return true;
}
//---------------------------------------------------------------------------

void TfrmEditLightAnim::UpdateProperties()
{
    // fill data
    PropItemVec items;
	if (m_CurrentItem){
        PHelper().CreateName	(items, "Name",			&m_CurrentItem->cName,		m_CurrentOwner);
        PHelper().CreateFloat	(items,	"FPS",			&m_CurrentItem->fFPS,		0.1f,1000,1.f,1);
        S32Value* V;
        V=PHelper().CreateS32	(items,	"Frame Count",	&m_CurrentItem->iFrameCount,1,100000,1);
        V->OnAfterEditEvent.bind(this,&TfrmEditLightAnim::OnFrameCountAfterEdit);

        u32 frame 				= sePointer->Value;
        PHelper().CreateCaption	(items,	"Current\\Frame",	shared_str().printf("%d",frame));
        u32* val				= m_CurrentItem->GetKey(sePointer->Value);
        if (val){
            PHelper().CreateColor(items,"Current\\Color",	val);
            PHelper().CreateU8	(items,	"Current\\Alpha",	((u8*)val)+3, 0x00, 0xFF);
        }
    }
    m_Props->AssignItems		(items);
    UpdateView					();
}
//---------------------------------------------------------------------------

void TfrmEditLightAnim::SetCurrentItem(CLAItem* I, ListItem* owner)
{
	m_CurrentItem 				= I;
    m_CurrentOwner				= owner;
    paItemProps->Visible 		= !!I;
    UpdateProperties			();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebAddAnimClick(TObject *Sender)
{
    // folder name
    AnsiString pref;
    TElTreeItem* item 			= m_Items->GetSelected(); 
    if (item) 					FHelper.MakeName(item,0,pref,true);
    pref 						+= "la";
    AnsiString name				= FHelper.GenerateName	(pref.c_str(),2,fastdelegate::bind<TFindObjectByName>(this,&TfrmEditLightAnim::FindItemByName),false,true);
    CLAItem* I 					= LALib.AppendItem(name.c_str(),0);
    InitItems					();
    m_Items->SelectItem			(*I->cName,true,false,true);
    OnModified					();
}
//---------------------------------------------------------------------------

void TfrmEditLightAnim::FindItemByName(LPCSTR name, bool& res)
{	
	res = !!LALib.FindItem(name);
}

void __fastcall TfrmEditLightAnim::ebCloneClick(TObject *Sender)
{
	if (m_CurrentItem){
        // folder name
	    AnsiString name			= FHelper.GenerateName	(m_CurrentItem->cName.c_str(),2,fastdelegate::bind<TFindObjectByName>(this,&TfrmEditLightAnim::FindItemByName),false,true);
        CLAItem* I 				= LALib.AppendItem(name.c_str(),m_CurrentItem);
        InitItems				();
        m_Items->SelectItem		(*I->cName,true,false,true);
        OnModified				();
    }else{
    	ELog.DlgMsg(mtError, "Select item at first.");
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebDeleteAnimClick(TObject *Sender)
{
	m_Items->RemoveSelItems		();	
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebSaveClick(TObject *Sender)
{
	ebSave->Enabled				 = false;
	LALib.Save					();
    UpdateView					();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebReloadClick(TObject *Sender)
{
	ebSave->Enabled 			= false;
	m_CurrentItem 				= 0;
	LALib.Reload				();
	m_Props->ClearProperties	();
    m_Items->ClearList			();
    InitItems					();
}
//---------------------------------------------------------------------------


void __fastcall TfrmEditLightAnim::pbGMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    if (Shift.Contains(ssLeft)){
		TRect R 	= pbG->ClientRect;
		TPoint pt(X,Y);
		pbG->ScreenToClient(pt);
        sePointer->Value = iFloor(float(m_CurrentItem->iFrameCount)*float(X)/float(R.Width()));
        pbG->Repaint();
	    if (Shift.Contains(ssDouble))
        	ebCreateKeyClick(Sender);
        else{
        	if (m_CurrentItem->IsKey(sePointer->Value)&&(sePointer->Value!=0)){
            	iMoveKey 	= sePointer->Value;
                iTgtMoveKey = iMoveKey;
            }
        }
    }
    wnShape->SetFocus();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::pbGMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    if (Shift.Contains(ssLeft))
        if (iMoveKey>=0){
            TRect R 	= pbG->ClientRect;
            TPoint pt(X,Y);
            pbG->ScreenToClient(pt);
            int new_key = iFloor(float(m_CurrentItem->iFrameCount)*float(X)/float(R.Width()));
            if ((new_key!=iTgtMoveKey)&&(new_key<m_CurrentItem->iFrameCount)&&(!m_CurrentItem->IsKey(new_key)||(new_key==iMoveKey))){
				iTgtMoveKey = new_key;
            	pbG->Repaint();
            }
        }
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::pbGMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    if (iMoveKey>=0){
    	if (iTgtMoveKey!=iMoveKey){
        	m_CurrentItem->MoveKey(iMoveKey,iTgtMoveKey);
	        sePointer->Value = iTgtMoveKey;
            pbG->Repaint();
            OnModified();
        }
		iMoveKey = -1;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebDeleteKeyClick(TObject *Sender)
{
	m_CurrentItem->DeleteKey(sePointer->Value);
    UpdateView();
    OnModified();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebCreateKeyClick(TObject *Sender)
{
    u32 color				= m_CurrentItem->InterpolateRGB(sePointer->Value);

    m_CurrentItem->InsertKey(sePointer->Value,color);
    UpdateProperties		();
    OnModified				();
}                    
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::pbGPaint(TObject *Sender)
{
	TRect R 	= pbG->ClientRect;
    Graphics::TBitmap* B 	= xr_new<Graphics::TBitmap>();
    B->Width 	= R.Width()-2;
    B->Height	= R.Height()-2;
	pbG->Canvas->Brush->Style 	= bsSolid;
    pbG->Canvas->Brush->Color	= clBlack;
    pbG->Canvas->FrameRect		(R);
    R.right		-= 2;
    R.bottom	-= 2;
    B->Canvas->Brush->Color		= TColor(0x00707070);
    B->Canvas->FillRect(R);
    if (m_CurrentItem){
    	float segment 	= float(R.Width())/(float(m_CurrentItem->iFrameCount));
        int half_segment= iFloor(segment/2);
        // draw gradient
        CLAItem::KeyMap& Keys=m_CurrentItem->Keys;
        int last=m_CurrentItem->iFrameCount;
        Keys[last]=Keys.rbegin()->second;
        CLAItem::KeyPairIt prev_key=Keys.begin();
        CLAItem::KeyPairIt it=prev_key; it++;
        float x_prev=(float(prev_key->first)/float(m_CurrentItem->iFrameCount))*R.Width();
		TRect cb;
        cb.Top 		= R.Top;
        cb.Bottom	= R.Bottom-R.Height()*0.3f;
	    for (; it!=Keys.end(); it++){
		    float x		= (it->first/float(m_CurrentItem->iFrameCount))*R.Width();
        	float g_cnt	= it->first-prev_key->first;
            for (int k=0; k<g_cnt; k++){
	            cb.Left		= iFloor(x_prev+k*segment);
    	        cb.Right	= iFloor(x_prev+k*segment+segment+1);
			    B->Canvas->Brush->Color	= TColor(subst_alpha(m_CurrentItem->InterpolateBGR(prev_key->first+k),0));
		    	B->Canvas->FillRect(cb);
            }
            prev_key 	= it;
            x_prev		= x;
        }
        Keys.erase(Keys.find(last));
        // draw keys
	    B->Canvas->Brush->Color= TColor(0x00BFFFFF);
        cb.Top 		= R.Bottom-R.Height()*0.1f;
        cb.Bottom	= R.Bottom;
	    for (it=m_CurrentItem->Keys.begin(); it!=m_CurrentItem->Keys.end(); it++){
		    int t		= iFloor((it->first/float(m_CurrentItem->iFrameCount))*R.Width());
            cb.Left		= t-1+half_segment;
   	        cb.Right	= t+2+half_segment;
			B->Canvas->FillRect(cb);
        }
	    // draw pointer
	    int t=iFloor((sePointer->Value/float(m_CurrentItem->iFrameCount))*R.Width())+half_segment;
	    B->Canvas->Pen->Color= TColor(0x0000FF00);
    	B->Canvas->MoveTo(t,R.Bottom);
    	B->Canvas->LineTo(t,R.Top+R.Height()*0.75f);
	    if ((iMoveKey>=0)&&(iTgtMoveKey!=iMoveKey))
        	t=iFloor((iTgtMoveKey/float(m_CurrentItem->iFrameCount))*R.Width())+half_segment;
        TRect rp=R;
        rp.Left 	= t-2;
        rp.Right 	= t+3;
        rp.Top		= R.Top+1;
        rp.Bottom	= R.Bottom-R.Height()*0.3f-1;
	    B->Canvas->Brush->Color	= TColor(0x00AAAAAA);
	    B->Canvas->FrameRect(rp);
        rp.Left 	-=1;
        rp.Right 	+=1;
        rp.Top		-=1;
        rp.Bottom	+=1;
	    B->Canvas->Brush->Color	= TColor(0x00000000);
	    B->Canvas->FrameRect(rp);
        //
    }
	pbG->Canvas->Draw(1,1,B);
    xr_delete(B);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::sePointerKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
	if (Key==VK_RETURN) pbG->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::sePointerExit(TObject *Sender)
{
	pbG->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::OnIdle()
{
	if (form){
		if (form->m_CurrentItem){
        	int 	frame;
            u32 C 	= form->m_CurrentItem->CalculateBGR(EDevice.fTimeGlobal,frame);
			form->paColor->Color		= TColor(subst_alpha(C,0));
            form->lbCurFrame->Caption	= AnsiString().sprintf("%d",frame);
            form->lbAlpha->Caption		= AnsiString().sprintf("[%d]",color_get_A(C));
        }
        UI->RedrawScene();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::sePointerChange(TObject *Sender)
{
	sePointer->Color = TColor(m_CurrentItem->IsKey(sePointer->Value)?0x00BFFFFF:0x00A0A0A0);
    UpdateProperties();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebFirstKeyClick(TObject *Sender)
{
	sePointer->Value	= m_CurrentItem->FirstKeyFrame();
	pbG->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebPrevKeyClick(TObject *Sender)
{
	sePointer->Value	= m_CurrentItem->PrevKeyFrame(sePointer->Value);
	pbG->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebNextKeyClick(TObject *Sender)
{
	sePointer->Value	= m_CurrentItem->NextKeyFrame(sePointer->Value);
	pbG->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebLastKeyClick(TObject *Sender)
{
	sePointer->Value	= m_CurrentItem->LastKeyFrame();
	pbG->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebMoveKeyLeftClick(TObject *Sender)
{
	if ((sePointer->Value!=0)&&(m_CurrentItem->IsKey(sePointer->Value))){
    	int f0, f1;
    	f1 = f0 = sePointer->Value;
        f1--;
        if (f1>=0){
	        m_CurrentItem->MoveKey(f0,f1);
    	    sePointer->Value--;
        	pbG->Repaint();
	        OnModified();
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebMoveKeyRightClick(TObject *Sender)
{
	if ((sePointer->Value!=0)&&(m_CurrentItem->IsKey(sePointer->Value))){
    	int f0, f1;
    	f1 = f0 = sePointer->Value;
        f1++;
        if (f1<m_CurrentItem->iFrameCount){
	        m_CurrentItem->MoveKey(f0,f1);
    	    sePointer->Value++;
        	pbG->Repaint();
	        OnModified();
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebLastFrameClick(TObject *Sender)
{
	sePointer->Value	= m_CurrentItem->iFrameCount-1;
	pbG->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::ebFirstFrameClick(TObject *Sender)
{
	sePointer->Value	= 0;
	pbG->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLightAnim::wnShapeKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    if (m_CurrentItem){
        if (Key==VK_LEFT){
            if (Shift.Contains(ssAlt)){
                sePointer->Value--;
                pbG->Repaint();
            }else if (Shift.Contains(ssCtrl)){
	            ebFirstKeyClick(Sender);
            }else{
                ebPrevKeyClick(Sender);
            }
        }else if (Key==VK_RIGHT){
            if (Shift.Contains(ssAlt)){
                sePointer->Value++;
                pbG->Repaint();
            }else if (Shift.Contains(ssCtrl)){
	            ebLastKeyClick(Sender);
            }else{
                ebNextKeyClick(Sender);
            }
        }
    }
}
//---------------------------------------------------------------------------


