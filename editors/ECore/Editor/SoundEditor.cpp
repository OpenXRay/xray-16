//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SoundEditor.h"
#include "EThumbnail.h"
#include "SoundManager.h"
#include "PropertiesList.h"
#include "FolderLib.h"
#include "ui_main.h"
#include "../../../xrSound/soundrender_source.h"
#include "ItemList.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ElTree"
#pragma link "ElXPThemedControl"
#pragma link "ExtBtn"
#pragma link "mxPlacemnt"
#pragma link "MXCtrls"
#pragma resource "*.dfm"

TfrmSoundLib* TfrmSoundLib::form = 0;
FS_FileSet	TfrmSoundLib::modif_map;
Flags32 TfrmSoundLib::m_Flags={0};
//---------------------------------------------------------------------------
__fastcall TfrmSoundLib::TfrmSoundLib(TComponent* Owner)
    : TForm(Owner)
{
    DEFINE_INI(fsStorage);
	bFormLocked = false;
}
//---------------------------------------------------------------------------

void __fastcall TfrmSoundLib::FormCreate(TObject *Sender)
{
	m_ItemProps = TProperties::CreateForm	("SoundED",paProperties,alClient);
    m_ItemProps->SetModifiedEvent			(fastdelegate::bind<TOnModifiedEvent>(this,&TfrmSoundLib::OnModified));
    m_ItemList	= TItemList::CreateForm		("Items",paItems,alClient,TItemList::ilMultiSelect/*|TItemList::ilEditMenu|TItemList::ilDragAllowed*/);
    m_ItemList->SetOnItemsFocusedEvent		(fastdelegate::bind<TOnILItemsFocused>(this,&TfrmSoundLib::OnItemsFocused));
    TOnItemRemove on_remove; on_remove.bind	(this,&TfrmSoundLib::RemoveSound);
    TOnItemRename on_rename; on_rename.bind	(this,&TfrmSoundLib::RenameSound);
    m_ItemList->SetOnItemRemoveEvent		(on_remove);
	m_ItemList->SetOnItemRenameEvent		(on_rename);
    m_ItemList->SetImages					(ImageList);
    bAutoPlay 								= FALSE;
}
//---------------------------------------------------------------------------

void __fastcall TfrmSoundLib::FormDestroy(TObject *Sender)
{
    TProperties::DestroyForm(m_ItemProps);
    TItemList::DestroyForm	(m_ItemList);
	m_Snd.destroy			();
}
//---------------------------------------------------------------------------

void __fastcall TfrmSoundLib::EditLib(AnsiString& title)
{
	if (!form){
        form 				= xr_new<TfrmSoundLib>((TComponent*)0);
        form->Caption 		= title;
		form->modif_map.clear();
        m_Flags.zero		();
	    form->InitItemsList	();
        if (!FS.can_write_to_alias(_sounds_)){
        	Log				("#!You don't have permisions to modify sounds.");
	        form->ebOk->Enabled 				= false;
            form->m_ItemProps->SetReadOnly		(TRUE);
            m_Flags.set		(flReadOnly,TRUE);
            form->bAutoPlay	= TRUE;
        }
    }

    form->ShowModal			();
    UI->RedrawScene			();
}
//---------------------------------------------------------------------------

void TfrmSoundLib::OnModified()
{
	m_ItemProps->RefreshForm();
}
//---------------------------------------------------------------------------

void __fastcall TfrmSoundLib::UpdateLib()
{
    RegisterModifiedTHM		();
    SaveUsedTHM				();
    // save game sounds
    if (modif_map.size()){
        AStringVec 			modif;
        LockForm			();
        SndLib->SynchronizeSounds	(true,true,true,&modif_map,0);
//		SndLib->ChangeFileAgeTo		(&modif_map,time(NULL));
        UnlockForm			();
        SndLib->RefreshSounds(false);
		modif_map.clear		();
    }
}

bool __fastcall TfrmSoundLib::HideLib()
{
	if (form){
    	form->Close();
		modif_map.clear();
    }
    return true;
}
//---------------------------------------------------------------------------

void TfrmSoundLib::AppendModif(LPCSTR nm)
{
    FS_File 		dest;
    string_path		fname;
    FS.update_path	(fname,_sounds_,ChangeFileExt(nm,".wav").c_str());
	BOOL bFind		= FS.file_find(fname,dest); R_ASSERT(bFind);
    modif_map.insert(dest);
}
//---------------------------------------------------------------------------

void TfrmSoundLib::RemoveSound(LPCSTR fname, EItemType type, bool& res)
{
	// delete it from modif map
    FS_FileSetIt it=modif_map.find(FS_File(fname));
    if (it!=modif_map.end()) modif_map.erase(it);
	// remove sound source
	res = SndLib->RemoveSound(fname,type);
}
//---------------------------------------------------------------------------

void TfrmSoundLib::RenameSound(LPCSTR p0, LPCSTR p1, EItemType type)
{
    // rename sound source
	SndLib->RenameSound(p0,p1,type);
	// delete old from map
    FS_FileSetIt old_it=modif_map.find(FS_File(p0)); 
    if (old_it!=modif_map.end()){
    	modif_map.erase	(old_it);
        AppendModif		(p1);
	}	
}
//---------------------------------------------------------------------------

void __fastcall TfrmSoundLib::FormShow(TObject *Sender)
{
	// check window position
	UI->CheckWindowPos	(this);
}
//---------------------------------------------------------------------------
void __fastcall TfrmSoundLib::FormClose(TObject *Sender, TCloseAction &Action)
{
	if (!form) 		return;
    form->Enabled 	= false;
    DestroyUsedTHM	();
	form 			= 0;
	Action 			= caFree;
}
//---------------------------------------------------------------------------
void TfrmSoundLib::InitItemsList()
{
    FS_FileSet		sound_map;
    SndLib->GetSounds(sound_map,TRUE);

	ListItemsVec items;

    // fill items
	FS_FileSetIt it = sound_map.begin();
	FS_FileSetIt _E = sound_map.end();
    for (; it!=_E; it++)
		LHelper().CreateItem(items,it->name.c_str(),0);

    m_ItemList->AssignItems(items,false,true);
}

//---------------------------------------------------------------------------
void __fastcall TfrmSoundLib::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{                                    
    if (Shift.Contains(ssCtrl)){
    	if (Key==VK_CANCEL)		ExecCommand(COMMAND_BREAK_LAST_OPERATION);
    }else{
        if (Key==VK_ESCAPE){
            if (bFormLocked)	ExecCommand(COMMAND_BREAK_LAST_OPERATION);
            Key = 0; // :-) нужно для того чтобы AccessVoilation не вылазил по ESCAPE
        }
    }
}
//---------------------------------------------------------------------------


void __fastcall TfrmSoundLib::ebOkClick(TObject *Sender)
{
	if (bFormLocked) return;
    m_Snd.destroy	();
    
	UpdateLib		();
    HideLib			();
}
//---------------------------------------------------------------------------

void __fastcall TfrmSoundLib::ebCancelClick(TObject *Sender)
{
	if (bFormLocked){
		ExecCommand(COMMAND_BREAK_LAST_OPERATION);
    	return;
    }

    HideLib			();
}
//---------------------------------------------------------------------------

ESoundThumbnail* TfrmSoundLib::FindUsedTHM(LPCSTR name)
{
	for (THMIt it=m_THM_Used.begin(); it!=m_THM_Used.end(); it++)
    	if (0==strcmp((*it)->SrcName(),name)) return *it;
    return 0;
}
//---------------------------------------------------------------------------

void TfrmSoundLib::SaveUsedTHM()
{
    int m_age 		= time(NULL);
	for (THMIt t_it=m_THM_Used.begin(); t_it!=m_THM_Used.end(); t_it++)
		(*t_it)->Save(m_age,0);
}
//---------------------------------------------------------------------------

void __fastcall TfrmSoundLib::RegisterModifiedTHM()
{
	if (m_ItemProps->IsModified()){
	    for (THMIt t_it=m_THM_Current.begin(); t_it!=m_THM_Current.end(); t_it++){
//.            (*t_it)->Save	(0,0);
            AppendModif		((*t_it)->SrcName());
        }
    }
}

void __fastcall TfrmSoundLib::fsStorageRestorePlacement(TObject *Sender)
{
	m_ItemProps->RestoreParams(fsStorage);
    m_ItemList->LoadParams(fsStorage);
    if (!m_Flags.is(flReadOnly)){
	    bAutoPlay = fsStorage->ReadInteger("auto_play",FALSE);
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmSoundLib::fsStorageSavePlacement(TObject *Sender)
{
	m_ItemProps->SaveParams(fsStorage);
    m_ItemList->SaveParams(fsStorage);
    if (!m_Flags.is(flReadOnly)){
	    fsStorage->WriteInteger("auto_play",bAutoPlay);
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmSoundLib::OnControlClick(ButtonValue* V, bool& bModif, bool& bSafe)
{
    switch (V->btn_num){
    case 0: m_Snd.play(0,sm_2D); 	break;
    case 1: m_Snd.stop();			break;
    case 2:{ 
    	bAutoPlay=!bAutoPlay; 
        V->value[V->btn_num] = shared_str().printf("Auto (%s)",bAutoPlay?"on":"off");
    }break;
	}
    bModif = false;
}
//------------------------------------------------------------------------------

void __fastcall TfrmSoundLib::OnControl2Click(ButtonValue* V, bool& bModif, bool& bSafe)
{
    switch (V->btn_num){
    case 0:{
    	bAutoPlay=!bAutoPlay; 
        V->value[V->btn_num] = bAutoPlay?"on":"off";
    }break;
	}
    bModif = false;
}
//------------------------------------------------------------------------------

void TfrmSoundLib::DestroyUsedTHM()
{
    for (THMIt it=m_THM_Used.begin(); it!=m_THM_Used.end(); it++)
    	xr_delete(*it);
    m_THM_Used.clear();
}
//------------------------------------------------------------------------------

#define X_GRID 10
#define Y_GRID 5
void  TfrmSoundLib::OnAttenuationDraw(CanvasValue* sender, void* _canvas, const Irect& _rect)
{
	TCanvas* canvas 	= (TCanvas*)_canvas;
    const TRect& rect	= *((TRect*)&_rect);
//	canvas
    int w = rect.Width();
    int h = rect.Height();
    int x0= rect.left;
    int y0= rect.top;

    canvas->Brush->Color = clBlack;
    canvas->FillRect(rect);
    canvas->Pen->Color = TColor(0x00006600);
    canvas->MoveTo(x0,y0);
    for (int i=0; i<X_GRID+1; i++){
        canvas->LineTo(x0+i*w/X_GRID,y0+h);
        canvas->MoveTo(x0+(i+1)*w/X_GRID,y0+0);
    }
    canvas->MoveTo(x0+0,y0+0);
    for (int j=0; j<Y_GRID+1; j++){
        canvas->LineTo(x0+w,y0+j*h/Y_GRID);
        canvas->MoveTo(x0+0,y0+(j+1)*h/Y_GRID);
    }
    canvas->Pen->Color = clYellow;
    canvas->MoveTo(x0+0,y0+iFloor(float(h)-float(h)*0.01f)); // snd cull = 0.01f
    canvas->LineTo(x0+w,y0+iFloor(float(h)-float(h)*0.01f));

    ESoundThumbnail* thm	= m_THM_Current.back();
    float d_cost 			= thm->MaxDist()/w;
    AnsiString temp;
//    float v = m_D3D.range;
//    temp.sprintf("Range = %.2f",v); lbRange->Caption = temp;
    canvas->Pen->Color = clLime;
    for (int d=1; d<w; d++){
        float R = d*d_cost;
    	float b = thm->MinDist()/(psSoundRolloff*R);
//		float b = m_Brightness/(m_Attenuation0+m_Attenuation1*R+m_Attenuation2*R*R);
        float bb = h-(h*b);
        int y = iFloor(y0+bb); clamp(y,int(rect.Top),int(rect.Bottom));
        if (1==d)	canvas->MoveTo(x0+d,y);
        else		canvas->LineTo(x0+d,y);
    }
}

void __stdcall TfrmSoundLib::OnAttClick(ButtonValue* V, bool& bModif, bool& bSafe)
{
    bModif = true;
    ESoundThumbnail* thm	= m_THM_Current.back();
    switch (V->btn_num){
    case 0:{
    	float dist			= thm->MinDist()/(0.01f*psSoundRolloff);
    	thm->SetMaxDist		(dist+0.1f*dist);
   	}break;
    case 1:{
    	float dist			= psSoundRolloff*(thm->MaxDist()-(0.1f/1.1f)*thm->MaxDist())*0.01f;
    	thm->SetMinDist		(dist);
    }break;
	}
}

void __fastcall TfrmSoundLib::OnItemsFocused(ListItemsVec& items)
{
	PropItemVec props;

    RegisterModifiedTHM	();
    m_Snd.destroy		();
    m_THM_Current.clear	();
                                          
	if (!items.empty()){
	    for (ListItemsIt it=items.begin(); it!=items.end(); it++){
            ListItem* prop = *it;
            if (prop){
            	ESoundThumbnail* thm=FindUsedTHM(prop->Key());
                if (!thm) m_THM_Used.push_back(thm=xr_new<ESoundThumbnail>(prop->Key()));
                m_THM_Current.push_back(thm);
                thm->FillProp		(props);
            }
        }
    }

	ButtonValue* B=0;
    if (m_THM_Current.size()==1)
    {
        ESoundThumbnail* thm=m_THM_Current.back();
        u32 size=0;
        u32 time=0;
        PlaySound(thm->SrcName(), size, time);

        CanvasValue* C=0;
        C=PHelper().CreateCanvas	(props,"Attenuation",	"", 64);
        C->tag						= (int)this;
        C->OnDrawCanvasEvent.bind	(this,&TfrmSoundLib::OnAttenuationDraw);
//		C->OnTestEqual.bind			(this,&TfrmSoundLib::OnPointDataTestEqual);
        B=PHelper().CreateButton	(props,"Auto Att",		"By Min,By Max",ButtonValue::flFirstOnly);
        B->OnBtnClickEvent.bind		(this,&TfrmSoundLib::OnAttClick);
        
        PHelper().CreateCaption		(props,"File Length",	shared_str().printf("%.2f Kb",float(size)/1024.f));
        PHelper().CreateCaption		(props,"Total Time", 	shared_str().printf("%.2f sec",float(time)/1000.f));
        if (!m_Flags.is(flReadOnly)){
	        B=PHelper().CreateButton(props,"Control",		"Play,Stop",ButtonValue::flFirstOnly);
    	    B->OnBtnClickEvent.bind	(this,&TfrmSoundLib::OnControlClick);
        }
    }
    if (!m_Flags.is(flReadOnly))
    {
	    B=PHelper().CreateButton	(props,"Auto Play",		bAutoPlay?"on":"off",ButtonValue::flFirstOnly);
    	B->OnBtnClickEvent.bind		(this,&TfrmSoundLib::OnControl2Click);
    }

    if (!m_Flags.is(flReadOnly) && m_THM_Current.size())
    {
        B=PHelper().CreateButton(props,"MANAGE", "SyncCurrent", ButtonValue::flFirstOnly);
        B->OnBtnClickEvent.bind	(this,&TfrmSoundLib::OnSyncCurrentClick);
    }

	m_ItemProps->AssignItems		(props);
}
//---------------------------------------------------------------------------

void TfrmSoundLib::PlaySound(LPCSTR name, u32& size, u32& time)
{
	string_path fname;
    FS.update_path			(fname,_game_sounds_,ChangeFileExt(name,".ogg").c_str());
    FS_File F;
    if (FS.file_find(fname,F))
    {
        m_Snd.create		(name,st_Effect,sg_Undefined);
        m_Snd.play			(0,sm_2D);
        CSoundRender_Source* src= (CSoundRender_Source*)m_Snd._handle(); VERIFY(src);
        size				= F.size;
        time				= iFloor(src->fTimeTotal/1000.0f);
    	if (!bAutoPlay)		m_Snd.stop();
    }
}

void TfrmSoundLib::OnFrame()
{
	if (form){
    	if (m_Flags.is(flUpdateProperties)){
        	form->m_ItemList->FireOnItemFocused();
        	m_Flags.set(flUpdateProperties,FALSE);
        }
    }
}

void __fastcall TfrmSoundLib::OnSyncCurrentClick(ButtonValue* V, bool& bModif, bool& bSafe)
{
//.
	THMIt it 	= m_THM_Current.begin();
	THMIt it_e 	= m_THM_Current.end();

    for(;it!=it_e; ++it)
    {
    	ESoundThumbnail* pTHM 		= *it;

        string_path             src_name, game_name;
        FS.update_path			(src_name,_sounds_,pTHM->SrcName());
        strconcat				(sizeof(src_name),src_name,src_name,".wav");

        FS.update_path			(game_name,_game_sounds_,pTHM->SrcName());
        strconcat				(sizeof(game_name),game_name,game_name,".ogg");

        Msg						("synchronizing [%s]", game_name);
		SndLib->MakeGameSound	(pTHM, src_name, game_name);
    }
    Msg	("Done.");
}

