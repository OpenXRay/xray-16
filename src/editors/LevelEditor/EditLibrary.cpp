//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "EditLibrary.h"
#include "../ECore/Editor/Library.h"
#include "ui_leveltools.h"
#include "../ECore/Editor/EditObject.h"
#include "Main.h"
#include "Scene.h"
#include "Builder.h"
#include "../ECore/Engine/Texture.h"
#include "BottomBar.h"
#include "../ECore/Editor/ImageManager.h"
#include "../ECore/Editor/EThumbnail.h"
#include "ESceneDOTools.h"
#include "xr_trims.h"
#include "SceneObject.h"
#include "../ECore/Engine/Image.h"
#include "../ECore/Editor/ui_main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ElTree"
#pragma link "ElHeader"
#pragma link "ElXPThemedControl"
#pragma link "ExtBtn"
#pragma link "mxPlacemnt"
#pragma link "ElXPThemedControl"
#pragma link "ExtBtn"
#pragma link "mxPlacemnt"
#pragma link "MxMenus"
#pragma link "ElTreeAdvEdit"
#pragma link "MXCtrls"
#pragma resource "*.dfm"

TfrmEditLibrary* TfrmEditLibrary::form=0;
FS_FileSet TfrmEditLibrary::modif_map;
bool TfrmEditLibrary::bFinalExit = false;
bool TfrmEditLibrary::bExitResult= true;

//---------------------------------------------------------------------------
__fastcall TfrmEditLibrary::TfrmEditLibrary(TComponent* Owner)
    : TForm(Owner)
{
    DEFINE_INI(fsStorage);
//	m_pEditObject 	= xr_new<CSceneObject>((LPVOID)0,(LPSTR)0);
    m_Props 		= TfrmPropertiesEObject::CreateProperties(0,alNone,TOnModifiedEvent(this,&TfrmEditLibrary::OnModified));
    m_Items			= TItemList::CreateForm("Objects",paItems,alClient,TItemList::ilMultiSelect|TItemList::ilEditMenu|TItemList::ilDragAllowed|TItemList::ilFolderStore);
    m_Items->SetOnItemsFocusedEvent	(fastdelegate::bind<TOnILItemsFocused>(this,&TfrmEditLibrary::OnItemsFocused));

    m_Items->SetOnItemRemoveEvent	(fastdelegate::bind<TOnItemRemove>(&Lib,&ELibrary::RemoveObject));
    m_Items->SetOnItemRenameEvent	(fastdelegate::bind<TOnItemRename>(&Lib,&ELibrary::RenameObject));
    bReadOnly		= false;
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLibrary::ShowEditor()
{
	if (!form){
    	form = xr_new<TfrmEditLibrary>((TComponent*)0);
		Scene->lock();
    }
    form->Show();
}
//---------------------------------------------------------------------------
CSceneObject* __fastcall TfrmEditLibrary::RayPick(const Fvector& start, const Fvector& direction, SRayPickInfo* pinf)
{
	if (!form) return 0;
	if (form->cbPreview->Checked)
    {
    	float dist=UI->ZFar();
        xr_vector<CSceneObject*>::iterator it 	= form->m_pEditObjects.begin();
        xr_vector<CSceneObject*>::iterator it_e = form->m_pEditObjects.end();
        for( ;it!=it_e; ++it)
        {
            CSceneObject* SO = *it;

            if(SO->RayPick(dist,start,direction,pinf) )
            {
                R_ASSERT(pinf&&pinf->e_mesh && pinf->e_obj);
                form->m_Props->OnPick(*pinf);
                pinf->s_obj = SO;
                return SO;
            }
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLibrary::OnRender()
{
	if (!form) return;
	if (!form->cbPreview->Checked) return;
    xr_vector<CSceneObject*>::iterator it 	= form->m_pEditObjects.begin();
    xr_vector<CSceneObject*>::iterator it_e = form->m_pEditObjects.end();
    for( ;it!=it_e; ++it)
    {
        CSceneObject* SO 	= *it;

        CSceneObject*		S = SO;

        CEditableObject* 	O = SO->GetReference();
        if (O)
        {
            if (!S->PPosition.similar(O->t_vPosition))
                S->PPosition 	= O->t_vPosition;

            if (!S->PRotation.similar(O->t_vRotate))
                S->PRotation 	= O->t_vRotate;

            if (!S->PScale.similar(O->t_vScale))
                S->PScale		= O->t_vScale;

            SO->OnFrame				();
            SO->RenderSingle			();
        }
    }
}

//---------------------------------------------------------------------------
void __fastcall TfrmEditLibrary::ZoomObject()
{
	if (!form) return;
	if (!form->cbPreview->Checked) return;

    xr_vector<CSceneObject*>::iterator it 	= form->m_pEditObjects.begin();
    xr_vector<CSceneObject*>::iterator it_e = form->m_pEditObjects.end();
    Fbox bb_max;
    for( ;it!=it_e; ++it)
    {
        CSceneObject* SO 	= *it;
    	Fbox bb;
        if (SO->GetBox(bb))
        	bb_max.merge(bb);
     }
     EDevice.m_Camera.ZoomExtents(bb_max);
}

//---------------------------------------------------------------------------
void __fastcall TfrmEditLibrary::FormShow(TObject *Sender)
{
	UI->BeginEState(esEditLibrary);
	modif_map.clear();

    InitObjects();
    ebSave->Enabled = false;
    // add directional light
    Flight L;
    ZeroMemory(&L,sizeof(Flight));
    L.type = D3DLIGHT_DIRECTIONAL;
    L.diffuse.set(1,1,1,1);
    L.direction.set(1,-1,1); L.direction.normalize();
	EDevice.SetLight(0,L);
	EDevice.LightEnable(0,true);
    L.diffuse.set(0.5,0.5,0.5,1);
    L.direction.set(1,-1,-1); L.direction.normalize();
	EDevice.SetLight(1,L);
	EDevice.LightEnable(1,true);

	// check window position
	UI->CheckWindowPos(this);
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLibrary::FormClose(TObject *Sender, TCloseAction &Action)
{
	Action = caFree;

    if (!bFinalExit&&ebSave->Enabled){
	    bFinalExit = false;
        UI->SetStatus("Objects reloading...");
        FS_FileSetIt it=modif_map.begin();
		FS_FileSetIt _E=modif_map.end();
        for (;it!=_E;it++)
        	Lib.ReloadObject(it->name.c_str());
        UI->ResetStatus();
    }
	Scene->unlock();

    UI->EndEState(esEditLibrary);

    // remove directional light                             
	EDevice.LightEnable(0,false);
	EDevice.LightEnable(1,false);

    xr_vector<CSceneObject*>::iterator it 	= form->m_pEditObjects.begin();
    xr_vector<CSceneObject*>::iterator it_e = form->m_pEditObjects.end();
    for( ;it!=it_e; ++it)
    {
        CSceneObject* SO 	= *it;
        xr_delete		(SO);
	}
    form->m_pEditObjects.clear	();
	xr_delete					(m_Thm);

    
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLibrary::FormDestroy(TObject *Sender)
{
	TItemList::DestroyForm(m_Items);
    TfrmPropertiesEObject::DestroyProperties(m_Props);

	form = 0;

    ExecCommand(COMMAND_CLEAR);
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLibrary::FormCloseQuery(TObject *Sender, bool &CanClose)
{
    CanClose = true;
    if (ebSave->Enabled){
    	int res = ELog.DlgMsg(mtConfirmation, "Library was change. Do you want save?");
		if (res==mrCancel) CanClose = false;
		if (res==mrYes) ebSaveClick(0);
    }
    bExitResult = CanClose;
}
//---------------------------------------------------------------------------
bool TfrmEditLibrary::FinalClose()
{
	if (!form) return true;
    bFinalExit = true;
    form->Close();
    return bExitResult;
}
//---------------------------------------------------------------------------
void TfrmEditLibrary::OnModified()
{
	if (!form) 				return;
    form->ebSave->Enabled 	= true;

    xr_vector<CSceneObject*>::iterator it 	= form->m_pEditObjects.begin();
    xr_vector<CSceneObject*>::iterator it_e = form->m_pEditObjects.end();
    for( ;it!=it_e; ++it)
    {
        CSceneObject* SO 		= *it;
        CEditableObject* E		= SO->GetReference();
        if (E)
        {
            modif_map.insert(FS_File(E->GetName()));
            E->Modified();
            SO->UpdateTransform();
        }
	}
    UI->RedrawScene();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::OnItemsFocused(ListItemsVec& items)
{
	xr_delete		(m_Thm);
//    bool mt			= false;
    bool b_one		= (items.size()==1);

    if (b_one /*&& FHelper.IsObject(items[0])*/ && UI->ContainEState(esEditLibrary))
    {
        // change thm
        ListItem* prop 			= items[0];
        VERIFY					(prop);
        AnsiString nm			= prop->Key();
        string_path 			thm_fn;
        ebRenameObject->Enabled = !bReadOnly;
        ebRemoveObject->Enabled = !bReadOnly;
//		ebExportLWO->Enabled 	= !bReadOnly;

        FS.update_path			(thm_fn,_objects_,ChangeFileExt(nm,".thm").c_str());
        if (FS.exist(thm_fn))
            m_Thm 				= xr_new<EObjectThumbnail>(nm.c_str());
        /*
        if (cbPreview->Checked || m_Props->Visible)
        {
            ChangeReference(nm.c_str());
		    if (cbPreview->Checked)
            	mt = true;
        }
        */
    }else
    {
//		ChangeReference(0);
    }

    if (cbPreview->Checked || m_Props->Visible)
    	SelectionToReference		(&items);
        
    if (m_Thm && m_Thm->Valid())
    {
        lbFaces->Caption 	= m_Thm->_FaceCount()?AnsiString(m_Thm->_FaceCount()):AnsiString("?");
        lbVertices->Caption = m_Thm->_VertexCount()?AnsiString(m_Thm->_VertexCount()):AnsiString("?");
    }else
    {
        lbFaces->Caption 	= "?";
        lbVertices->Caption = "?";
    }
    
    paImage->Repaint		();
    UpdateObjectProperties	();
    UI->RedrawScene			();
    ebMakeThm->Enabled		= m_pEditObjects.size()>0;
    ebMakeLOD_high->Enabled	= cbPreview->Checked;
    ebMakeLOD_low->Enabled	= cbPreview->Checked;

}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::cbPreviewClick(TObject *Sender)
{
	RefreshSelected();
}
//---------------------------------------------------------------------------

void TfrmEditLibrary::InitObjects()
{
	ListItemsVec items;
    FS_FileSet lst;
    Lib.GetObjects(lst);
    FS_FileSetIt it=lst.begin();
    FS_FileSetIt _E=lst.end();
    for(; it!=_E; it++)
    	LHelper().CreateItem(items,it->name.c_str(),0,0,0);
    m_Items->AssignItems(items,false,true);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Shift.Contains(ssCtrl)){
    	if (Key==VK_CANCEL)		ExecCommand(COMMAND_BREAK_LAST_OPERATION);
    }else{
        if (Key==VK_ESCAPE){
            if (bFormLocked)	ExecCommand(COMMAND_BREAK_LAST_OPERATION);
            else				ebCancel->Click();
            Key = 0; // :-) нужно для того чтобы AccessVoilation не вылазил по ESCAPE
        }
    }
}
//---------------------------------------------------------------------------


void __fastcall TfrmEditLibrary::ebPropertiesClick(TObject *Sender)
{
    SelectionToReference		(NULL);
    UpdateObjectProperties		();
    m_Props->ShowProperties		();
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLibrary::ebSaveClick(TObject *Sender)
{
	RStringVec			sel_strings;
	ebSave->Enabled 	= false;
    ChangeReference		(sel_strings);
    Lib.Save			(&modif_map);
    modif_map.clear 	();
    RefreshSelected		();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::ebCancelClick(TObject *Sender)
{
    Close();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::tvItemsDblClick(TObject *Sender)
{
	ebPropertiesClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::ebMakeThmClick(TObject *Sender)
{
    U32Vec 						pixels;

    ListItemsVec 				sel_items;
   	m_Items->GetSelected		(NULL, sel_items, false);
    ListItemsIt it 				= sel_items.begin();
    ListItemsIt it_e 			= sel_items.end();

    for( ;it!=it_e; ++it)
    {
        ListItem* item			= *it;
   	    CEditableObject* obj 	= Lib.CreateEditObject(item->Key());
    	if(obj && cbPreview->Checked)
        {
            string_path 			fn;
        	FS.update_path			(fn,_objects_,ChangeFileExt(obj->GetName(),".thm").c_str());

            m_Items->SelectItem			(item->Key(),true,false,true);
            if (ImageLib.CreateOBJThumbnail	(fn,obj,obj->Version()))
            {
	           ELog.Msg					(mtInformation,"Thumbnail successfully created.");
//             AnsiString 					full_name;
//             FHelper.MakeFullName		(node,0,full_name);
//                m_Items->SelectItem			(item->Key(),true,false,true);
            }
	    }else{
            ELog.DlgMsg(mtError,"Can't create thumbnail. Set preview mode.");
        }
		Lib.RemoveEditObject(obj);
	}

/*
    TElTreeItem* node 			= m_Items->GetSelected();
	if (node&&FHelper.IsObject(node))
    {
    	AnsiString 				name;
        FHelper.MakeName		(node,0,name,false);
   	    CEditableObject* obj 	= Lib.CreateEditObject(name.c_str());
    	if (obj&&cbPreview->Checked)
        {
            string_path 			fn;
        	FS.update_path			(fn,_objects_,ChangeFileExt(obj->GetName(),".thm").c_str());

            if (ImageLib.CreateOBJThumbnail	(fn,obj,obj->Version())){
	            ELog.Msg					(mtInformation,"Thumbnail successfully created.");
                AnsiString 					full_name;
                FHelper.MakeFullName		(node,0,full_name);
                m_Items->SelectItem			(full_name.c_str(),true,false,true);
            }
	    }else{
            ELog.DlgMsg(mtError,"Can't create thumbnail. Set preview mode.");
        }
		Lib.RemoveEditObject(obj);
    }
*/
    ELog.DlgMsg(mtInformation,"Done.");
}
//---------------------------------------------------------------------------

bool TfrmEditLibrary::GenerateLOD(ListItemsVec& props, bool bHighQuality)
{
    ListItemsIt it 				= props.begin();
    ListItemsIt it_e 			= props.end();

    SPBItem* pb 				= UI->ProgressStart(props.size(),"Making LOD");
    for( ;it!=it_e; ++it)
    {
        ListItem* item				= *it;
        m_Items->SelectItem			(item->Key(),true,false,true);
        R_ASSERT					(form->m_pEditObjects.size()==1);

        CSceneObject* SO 			= form->m_pEditObjects[0];
    	CEditableObject* O 			= SO->GetReference();

        if (O && O->IsMUStatic())
        {
            pb->Inc					(O->GetName());
            BOOL bLod 				= O->m_objectFlags.is(CEditableObject::eoUsingLOD);
            O->m_objectFlags.set	(CEditableObject::eoUsingLOD,FALSE);
            xr_string 				tex_name;
            tex_name 				= EFS.ChangeFileExt(O->GetName(),"");

            string_path				tmp;
            strcpy					(tmp,tex_name.c_str()); _ChangeSymbol(tmp,'\\','_');
            tex_name 				= xr_string("lod_")+tmp;
            tex_name 				= ImageLib.UpdateFileName(tex_name);
            ImageLib.CreateLODTexture(O, tex_name.c_str(),LOD_IMAGE_SIZE,LOD_IMAGE_SIZE,LOD_SAMPLE_COUNT,O->Version(),bHighQuality?4/*7*/:1);
            O->OnDeviceDestroy		();
            O->m_objectFlags.set	(CEditableObject::eoUsingLOD,bLod);
            ELog.Msg				(mtInformation,"LOD for object '%s' successfully created.",O->GetName());
        }else{
            ELog.Msg				(mtError,"Can't create LOD texture from non 'Multiple Usage' object.", SO->Name);
        }

        if (UI->NeedAbort()) break;
    }
    UI->ProgressEnd	(pb);

/*
	SelectionToReference			(&props);

    xr_vector<CSceneObject*>::iterator it 	= form->m_pEditObjects.begin();
    xr_vector<CSceneObject*>::iterator it_e = form->m_pEditObjects.end();
    SPBItem* pb 					= UI->ProgressStart(form->m_pEditObjects.size(),"Making LOD");
    for( ;it!=it_e; ++it)
    {
        CSceneObject* SO 			= *it;
    	CEditableObject* O 			= SO->GetReference();

        if (O && O->IsMUStatic())
        {
            pb->Inc					(O->GetName());
            BOOL bLod 				= O->m_objectFlags.is(CEditableObject::eoUsingLOD);
            O->m_objectFlags.set	(CEditableObject::eoUsingLOD,FALSE);
            xr_string 				tex_name;
            tex_name 				= EFS.ChangeFileExt(O->GetName(),"");

            string_path				tmp;
            strcpy					(tmp,tex_name.c_str()); _ChangeSymbol(tmp,'\\','_');
            tex_name 				= xr_string("lod_")+tmp;
            tex_name 				= ImageLib.UpdateFileName(tex_name);
            ImageLib.CreateLODTexture(O, tex_name.c_str(),LOD_IMAGE_SIZE,LOD_IMAGE_SIZE,LOD_SAMPLE_COUNT,O->Version(),bHighQuality?7:1);
            O->OnDeviceDestroy		();
            O->m_objectFlags.set	(CEditableObject::eoUsingLOD,bLod);
            ELog.Msg				(mtInformation,"LOD for object '%s' successfully created.",O->GetName());
        }else{
            ELog.Msg				(mtError,"Can't create LOD texture from non 'Multiple Usage' object.", SO->Name);
        }
        if (UI->NeedAbort()) break;
    }
    UI->ProgressEnd	(pb);
*/
    return true;
}
//---------------------------------------------------------------------------

void TfrmEditLibrary::MakeLOD(bool bHighQuality)
{
	if (ebSave->Enabled){
    	ELog.DlgMsg				(mtError,"Save library changes before generating LOD.");
        return;
    }
    ListItemsVec 				sel_items;
   	m_Items->GetSelected		(NULL, sel_items, false /*true*/);
    GenerateLOD					(sel_items,bHighQuality);
    ELog.DlgMsg(mtInformation,"Done.");

/*
	TElTreeItem* node 			= m_Items->GetSelected();
	if (node && cbPreview->Checked)
    {
        LockForm();
    	int res 				= ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbNo << mbCancel,"Do you want to select multiple objects?");
        if (res!=mrCancel)
        {
            if (res==mrYes)
            {
                LPCSTR new_val		= 0;
                if (TfrmChoseItem::SelectItem(smObject,new_val,256))
                {
                    int cnt			= _GetItemCount(new_val);
                    int iLODcnt 	= 0;
                    SPBItem* pb 	= UI->ProgressStart(cnt,"Making LOD");

                    for (int k=0; k<cnt; ++k)
                    {
                        xr_string 		tmp;
                        _GetItem		(new_val,k,tmp);
                        pb->Inc			(tmp.c_str());
                        ListItem* I		= m_Items->FindItem(tmp.c_str());
                        VERIFY			(I);
                        ListItemsVec	V;
                        V.push_back		(I);
                        if (GenerateLOD	(V,bHighQuality))
                        	iLODcnt++;

                        if (UI->NeedAbort()) break;
                    }
                    ELog.DlgMsg 	(mtInformation,"'%d' LOD's succesfully created.",iLODcnt);
                    UI->ProgressEnd	(pb);
                }
            }else{
                AnsiString m_LastSelection;
                FHelper.MakeFullName(node,0,m_LastSelection);
                if (FHelper.IsObject(node))
                {
                    ListItem* prop 		= (ListItem*)node->Tag; VERIFY(prop);
                    ListItemsVec		V2;
                    V2.push_back		(prop);
                    GenerateLOD			(V2,bHighQuality);

                } if(FHelper.IsFolder(node))
                {
                    if (mrYes==ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbNo,"Are you sure to generate LOD for all object in this folder?"))
                    {
                        int iLODcnt = 0;
                        SPBItem* pb = UI->ProgressStart(node->ChildrenCount,"Making LOD");
                        for (TElTreeItem* item=node->GetFirstChild(); item; item=node->GetNextChild(item))
                        {
                            ListItem* prop 		= (ListItem*)node->Tag; VERIFY(prop);
                            pb->Inc				(prop->Key());
                            ListItemsVec		V3;
                            V3.push_back		(prop);
                            if (GenerateLOD(V3,bHighQuality))
                            	iLODcnt++;

                            if (UI->NeedAbort()) break;
                        }
                        UI->ProgressEnd			(pb);
                        ELog.DlgMsg 			(mtInformation,"'%d' LOD's succesfully created.",iLODcnt);
                    }
                }
            }
        }
        UnlockForm();
//        m_Items->SelectItem	(m_LastSelection.c_str(),true,false,true);
    }
*/    
}

void __fastcall TfrmEditLibrary::ebMakeLOD_highClick(TObject *Sender)
{
	MakeLOD(true);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::ebMakeLOD_lowClick(TObject *Sender)
{
	MakeLOD(false);
}
//---------------------------------------------------------------------------

void TfrmEditLibrary::ChangeReference(const RStringVec& items)
{
    xr_vector<CSceneObject*>::iterator it 	= m_pEditObjects.begin();
    xr_vector<CSceneObject*>::iterator it_e = m_pEditObjects.end();
    for( ;it!=it_e; ++it)
    {
        CSceneObject* SO 			= *it;
        xr_delete					(SO);
    }
    m_pEditObjects.clear			();

	RStringVec::const_iterator sit 		= items.begin();
	RStringVec::const_iterator sit_e 	= items.end();
    for(;sit!=sit_e; ++sit)
    {
	CSceneObject* SO 				= xr_new<CSceneObject>((LPVOID)0,(LPSTR)0);
    m_pEditObjects.push_back		(SO);
    SO->SetReference				((*sit).c_str());

	CEditableObject* NE				= SO->GetReference();
    if (NE)
    {
		SO->PPosition 				= NE->t_vPosition;
		SO->PScale 					= NE->t_vScale;
		SO->PRotation				= NE->t_vRotate;
    }
    // update transformation
    SO->UpdateTransform	();

/*
	// save new position
	CEditableObject* E				= m_pEditObject->GetReference();

    if (E && new_name && (stricmp(E->GetName(),new_name))==0 ) return;

    if (E)
    {
		E->t_vPosition.set			(m_pEditObject->PPosition);
		E->t_vScale.set				(m_pEditObject->PScale);
		E->t_vRotate.set			(m_pEditObject->PRotation);
    }
    m_pEditObject->SetReference		(new_name);
    // get old position
	E								= m_pEditObject->GetReference();
    if (E)
    {
		m_pEditObject->PPosition 	= E->t_vPosition;
		m_pEditObject->PScale 		= E->t_vScale;
		m_pEditObject->PRotation	= E->t_vRotate;
    }
    // update transformation
    m_pEditObject->UpdateTransform	();
*/
    }

    ExecCommand						(COMMAND_EVICT_OBJECTS);
    ExecCommand						(COMMAND_EVICT_TEXTURES);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::ResetSelected()
{
	if (form)
    {
        xr_vector<CSceneObject*>::iterator it 	= form->m_pEditObjects.begin();
        xr_vector<CSceneObject*>::iterator it_e = form->m_pEditObjects.end();
        for( ;it!=it_e; ++it)
        {
            CSceneObject* SO 			= *it;
            SO->SetReference			(0);
        }
    }
}
//---------------------------------------------------------------------------
bool TfrmEditLibrary::SelectionToReference(ListItemsVec* props)
{
    RStringVec					sel_strings;
    ListItemsVec 				sel_items;

    if(props==NULL)
    	m_Items->GetSelected	(NULL, sel_items, false /*true*/);
    else
		sel_items				= *props;

    ListItemsIt it 				= sel_items.begin();
    ListItemsIt it_e 			= sel_items.end();

    for( ;it!=it_e; ++it)
    {
        ListItem* item			= *it;
        sel_strings.push_back   (item->Key());
    }
    ChangeReference				(sel_strings);
    return                       sel_strings.size() > 0;
}

void __fastcall TfrmEditLibrary::RefreshSelected()
{
	if (form)
    {
        bool mt				= false;
		if(cbPreview->Checked)
        	mt = SelectionToReference	(NULL);

        ebMakeThm->Enabled 			= !bReadOnly && mt;
        ebMakeLOD_high->Enabled 	= !bReadOnly && cbPreview->Checked;
        ebMakeLOD_low->Enabled 		= !bReadOnly && cbPreview->Checked;
        UI->RedrawScene				();
    }
}
//---------------------------------------------------------------------------
void __fastcall TfrmEditLibrary::paImagePaint(TObject *Sender)
{
    if (m_Thm) m_Thm->Draw(paImage);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::ebExportLWOClick(TObject *Sender)
{
    TElTreeItem* node 				= m_Items->GetSelected();
    if (node&&FHelper.IsObject(node))
    {
    	AnsiString 					name;
        FHelper.MakeName			(node,0,name,false);
        xr_string 					save_nm;

        if (EFS.GetSaveName(_import_,save_nm,0,1))
        {
            CEditableObject* obj 	= Lib.CreateEditObject(name.c_str());
            if (obj)
            {
                if (!obj->ExportLWO(save_nm.c_str()))
                {
                    ELog.DlgMsg(mtInformation, "Can't export object '%s'.", obj->GetName());
                }else{
					ELog.DlgMsg(mtInformation, "Export complete.");
                }
            }else
            {
            	ELog.DlgMsg				(mtError,"Can't load object.");
        	}
			Lib.RemoveEditObject		(obj);
	    }
    }else{
        ELog.DlgMsg						(mtInformation, "Select object to export.");
    }
}

//---------------------------------------------------------------------------
void __fastcall TfrmEditLibrary::ebImportClick(TObject *Sender)
{
    xr_string open_nm, save_nm, nm;
    if (EFS.GetOpenName(_import_,open_nm,true))
    {
    	// remove selected object
        ResetSelected			();
		// load
    	AStringVec 				lst;
        _SequenceToList       	(lst,open_nm.c_str());
		bool bNeedUpdate		= false;
        // folder name
        AnsiString 				folder;


        ListItemsVec 				sel_items;
        m_Items->GetSelected		(NULL, sel_items, false);
        if(sel_items.size())
       		FHelper.GetFolderName	(sel_items[0]->Key(), folder);

		xr_string m_LastSelection;
        for (AStringIt it=lst.begin(); it!=lst.end(); ++it)
        {
        	nm = ChangeFileExt(ExtractFileName(*it),"").c_str();
            CEditableObject* O = xr_new<CEditableObject>(nm.c_str());
            if (O->Load(it->c_str()))
            {
                save_nm = xr_string(FS.get_path(_objects_)->m_Path)+folder.c_str()+EFS.ChangeFileExt(nm,".object");

                if (FS.exist(save_nm.c_str()))
			        if (mrNo==ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbNo,"Object '%s' already exist. Owerwrite it?",nm.c_str()))
                    {
                    	xr_delete(O);
                    	break;
                    }
                
                O->Save			(save_nm.c_str());
                EFS.MarkFile	(it->c_str(),true);
                bNeedUpdate		= true;
            }else
            	ELog.DlgMsg		(mtError,"Can't load file '%s'.",it->c_str());

            xr_delete(O);

            LPCSTR p = FS.get_path(_objects_)->m_Path;
            if (folder.Pos(p)>0)
            {
            	m_LastSelection = xr_string(folder.c_str()+strlen(p))+nm;
                xr_strlwr		(m_LastSelection);
            }else{
            	m_LastSelection = xr_string(folder.c_str())+nm;
            }
        }
        if (bNeedUpdate)
        {
			Lib.CleanLibrary	();
			InitObjects			();
            m_Items->SelectItem	(m_LastSelection.c_str(),true,false,true);
        }
    }
}
//---------------------------------------------------------------------------

void TfrmEditLibrary::UpdateObjectProperties()
{
	m_Props->UpdateProperties(m_pEditObjects,bReadOnly);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::FormActivate(TObject *Sender)
{
	m_Items->SetILFocus();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::OnObjectRename(LPCSTR p0, LPCSTR p1, EItemType type)
{
	Lib.RenameObject(p0,p1,type);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::fsStorageRestorePlacement(TObject *Sender)
{
	m_Items->LoadSelection(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::fsStorageSavePlacement(TObject *Sender)
{
	m_Items->SaveSelection(fsStorage);
}
//---------------------------------------------------------------------------


void __fastcall TfrmEditLibrary::ebRenameObjectClick(TObject *Sender)
{
	m_Items->RenameSelItem();
}
//---------------------------------------------------------------------------

void __fastcall TfrmEditLibrary::ebRemoveObjectClick(TObject *Sender)
{
	m_Items->RemoveSelItems();
}
//---------------------------------------------------------------------------

#include "../ECore/Editor/ExportObjectOGF.h"

void __fastcall TfrmEditLibrary::ebExportOBJClick(TObject *Sender)
{
	if(!cbPreview->Checked)
    {
        ListItemsVec 				sel_items;
        m_Items->GetSelected		(NULL, sel_items, false);
        ListItemsIt it 				= sel_items.begin();
        ListItemsIt it_e 			= sel_items.end();

        SPBItem* pb 				= UI->ProgressStart(form->m_pEditObjects.size(),"Expotring to OBJ");
        CSceneObject* SO 			= xr_new<CSceneObject>((LPVOID)0,(LPSTR)0);
        for( ;it!=it_e; ++it)
        {
            ListItem* item			= *it;
            pb->Inc					(item->Key());
            SO->SetReference		(item->Key());
            CEditableObject* NE		= SO->GetReference();
            SO->UpdateTransform		();
            if (NE)
            {
                SO->PPosition 		= NE->t_vPosition;
                SO->PScale 			= NE->t_vScale;
                SO->PRotation		= NE->t_vRotate;

                ExportOneOBJ		(NE);
            }
            if (UI->NeedAbort()) 	break;
        }
        xr_delete					(SO);
        UI->ProgressEnd				(pb);
    }else
    {
        xr_vector<CSceneObject*>::iterator it 	= form->m_pEditObjects.begin();
        xr_vector<CSceneObject*>::iterator it_e = form->m_pEditObjects.end();
        SPBItem* pb 							= UI->ProgressStart(form->m_pEditObjects.size(),"Expotring to OBJ");
        for( ;it!=it_e; ++it)
        {
            CSceneObject* SO 		= *it;
            CEditableObject* 		O = SO->GetReference();
            pb->Inc					(O->GetName());

            if(O)
            {
                ExportOneOBJ		(O);
            }
            if (UI->NeedAbort()) 	break;
        }
        UI->ProgressEnd				(pb);
    }
    ELog.DlgMsg(mtInformation,"Done.");
}

void TfrmEditLibrary::ExportOneOBJ(CEditableObject* EO)
{
    string_path			fn;
    FS.update_path		(fn, _import_, EO->m_LibName.c_str());
    CExportObjectOGF 	E(EO);
    CMemoryWriter 		F;
    if (E.ExportAsWavefrontOBJ(F,fn))
    {
        strcat			(fn,".obj");
        F.save_to		(fn);
    }
}

