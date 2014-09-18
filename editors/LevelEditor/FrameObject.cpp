#include "stdafx.h"
#pragma hdrstop

#include "ui_leveltools.h"
#include "FrameObject.h"
#include "scene.h"
#include "../ECore/Editor/ui_main.h"
#include "../ECore/Editor/EditObject.h"
#include "SceneObject.h"
#include "../ECore/Editor/library.h"
#include "ESceneObjectTools.h"
#include "../ECore/Editor/EThumbnail.h"
#include "Scene.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "multi_edit"
#pragma link "mxPlacemnt"
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
__fastcall TfraObject::TfraObject(TComponent* Owner,ESceneObjectTool* parent_tools)
        : TForm(Owner)
{
    DEFINE_INI(fsStorage);
    m_Current 	= 0;
    ParentTools	= parent_tools;
}
//---------------------------------------------------------------------------
void TfraObject::OnDrawObjectThumbnail(LPCSTR name, HDC hdc, const Irect &r)
{
	EObjectThumbnail* thm	= xr_new<EObjectThumbnail>(name);
    thm->Draw				(hdc,r);
    xr_delete				(thm);
}
//---------------------------------------------------------------------------
void __fastcall TfraObject::OnItemFocused(ListItemsVec& items)
{
	VERIFY(items.size()<=1);
    m_Current 			= 0;
    for (ListItemsIt it=items.begin(); it!=items.end(); it++)
        m_Current 		= (*it)->Key();
    ExecCommand			(COMMAND_RENDER_FOCUS);
}
//------------------------------------------------------------------------------
void __fastcall TfraObject::PaneMinClick(TObject *Sender)
{
    PanelMinMaxClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::ExpandClick(TObject *Sender)
{
    PanelMaximizeClick(Sender);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Selecting
//---------------------------------------------------------------------------

void __fastcall TfraObject::ebSelectByRefsClick(TObject *Sender)
{
	SelByRefObject( true );
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::ebDeselectByRefsClick(TObject *Sender)
{
	SelByRefObject( false );
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::MultiSelByRefObject ( bool clear_prev )
{
    ObjectList 	objlist;
    LPU32Vec 	sellist;
    if (Scene->GetQueryObjects(objlist,OBJCLASS_SCENEOBJECT,1,1,-1)){
    	for (ObjectIt it=objlist.begin(); it!=objlist.end(); it++){
	        LPCSTR N = ((CSceneObject*)*it)->RefName();
            ObjectIt _F = Scene->FirstObj(OBJCLASS_SCENEOBJECT);
            ObjectIt _E = Scene->LastObj(OBJCLASS_SCENEOBJECT);
            for(;_F!=_E;_F++){
	            CSceneObject *_O = (CSceneObject *)(*_F);
                if((*_F)->Visible()&&_O->RefCompare(N)){
                	if (clear_prev){
                    	_O->Select( false );
	                    sellist.push_back((u32*)_O);
                    }else{
                    	if (!_O->Selected())
                        	sellist.push_back((u32*)_O);
                    }
                }
            }
        }
        std::sort			(sellist.begin(),sellist.end());
        sellist.erase		(std::unique(sellist.begin(),sellist.end()),sellist.end());
        std::random_shuffle	(sellist.begin(),sellist.end());
        int max_k		= iFloor(float(sellist.size())/100.f*float(seSelPercent->Value)+0.5f);
        int k			= 0;
        for (LPU32It o_it=sellist.begin(); k<max_k; o_it++,k++){
            CSceneObject *_O = (CSceneObject *)(*o_it);
            _O->Select( true );
        }
    }
}
//---------------------------------------------------------------------------


void TfraObject::SelByRefObject( bool flag )
{
    ObjectList objlist;
//    LPCSTR sel_name=0;
//    if (Scene->GetQueryObjects(objlist,OBJCLASS_SCENEOBJECT,1,1,-1))
//        sel_name = ((CSceneObject*)objlist.front())->GetRefName();
	LPCSTR N=Current();
//    if (!TfrmChoseItem::SelectItem(TfrmChoseItem::smObject,N,1,sel_name)) return;
	if (N){
        ObjectIt _F = Scene->FirstObj(OBJCLASS_SCENEOBJECT);
        ObjectIt _E = Scene->LastObj(OBJCLASS_SCENEOBJECT);
        for(;_F!=_E;_F++){
            if((*_F)->Visible() ){
                CSceneObject *_O = (CSceneObject *)(*_F);
                if(_O->RefCompare(N)) _O->Select( flag );
            }
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::ebMultiAppendClick(TObject *Sender)
{
	LPCSTR N;
    if (TfrmChoseItem::SelectItem(smObject,N,32,0)){
    	Fvector pos={0.f,0.f,0.f};
    	Fvector up={0.f,1.f,0.f};
        Scene->SelectObjects(false,OBJCLASS_SCENEOBJECT);
	    AStringVec lst;
    	_SequenceToList(lst,N);
        SPBItem* pb = UI->ProgressStart(lst.size(),"Append object: ");
        for (AStringIt it=lst.begin(); it!=lst.end(); it++){
            string256 namebuffer;
            Scene->GenObjectName(OBJCLASS_SCENEOBJECT, namebuffer, it->c_str());
            CSceneObject *obj = xr_new<CSceneObject>((LPVOID)0,namebuffer);
            CEditableObject* ref = obj->SetReference(it->c_str());
            if (!ref){
                ELog.DlgMsg(mtError,"TfraObject:: Can't load reference object.");
                xr_delete(obj);
                return;
            }
/*            if (ref->IsDynamic()){
                ELog.DlgMsg(mtError,"TfraObject:: Can't assign dynamic object.");
                xr_delete(obj);
                return;
            }
*/
            obj->MoveTo(pos,up);
            Scene->AppendObject( obj );
        }         
        UI->ProgressEnd(pb);
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::ebMultiSelectByRefMoveClick(TObject *Sender)
{
	MultiSelByRefObject(true);
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::ebMultiSelectByRefAppendClick(TObject *Sender)
{
	MultiSelByRefObject(false);
}
//---------------------------------------------------------------------------


void __fastcall TfraObject::seSelPercentKeyPress(TObject *Sender,
      char &Key)
{
	if (Key==VK_RETURN) ExecCommand(COMMAND_RENDER_FOCUS);
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::ExtBtn4Click(TObject *Sender)
{
    if (TfrmChoseItem::SelectItem(smObject,m_Current,1,m_Current)){
    	m_Items->SelectItem	(m_Current,true,false,true);
//..		RefreshList			();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::paCurrentObjectResize(TObject *Sender)
{
    if (m_Current) m_Items->SelectItem(m_Current,true,false,true);
}
//---------------------------------------------------------------------------

void TfraObject::RefreshList()
{
    ListItemsVec items;
    FS_FileSet lst;
    if (Lib.GetObjects(lst)){
	    FS_FileSetIt	it	= lst.begin();            
    	FS_FileSetIt	_E	= lst.end();
	    for (; it!=_E; it++){
            xr_string fn;
	    	ListItem* I=LHelper().CreateItem(items,it->name.c_str(),0,ListItem::flDrawThumbnail,0);
            if (I->m_Flags.is(ListItem::flDrawThumbnail)) I->OnDrawThumbnail.bind(this,&TfraObject::OnDrawObjectThumbnail);
        }
    }
    m_Items->AssignItems	(items,false,true);
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::FormShow(TObject *Sender)
{
    m_Items->LoadSelection	(fsStorage);
    m_Items->LoadParams		(fsStorage);

//..RefreshList				();

    ebRandomAppendMode->Down= ParentTools->IsAppendRandomActive();
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::FormHide(TObject *Sender)
{
    m_Items->SaveSelection	(fsStorage);
    m_Items->SaveParams		(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::FormCreate(TObject *Sender)
{
    m_Items 				= TItemList::CreateForm("Objects", paItems, alClient, 0);
    m_Items->SetOnItemsFocusedEvent(TOnILItemsFocused(this,&TfraObject::OnItemFocused));
	// fill list
    RefreshList				();
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::FormDestroy(TObject *Sender)
{
    TItemList::DestroyForm	(m_Items);
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::ebRandomAppendModeClick(TObject *Sender)
{
    ParentTools->ActivateAppendRandom(ebRandomAppendMode->Down);
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::ExtBtn8Click(TObject *Sender)
{
    ParentTools->FillAppendRandomProperties();
}
//---------------------------------------------------------------------------

void __fastcall TfraObject::ExtBtn9Click(TObject *Sender)
{
	RefreshList();	
}
//---------------------------------------------------------------------------

