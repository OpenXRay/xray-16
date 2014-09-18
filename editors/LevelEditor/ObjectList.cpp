#include "stdafx.h"
#pragma hdrstop
// test comment
#include "ObjectList.h"
#include "GroupObject.h"
#include "ui_leveltools.h"
#include "Scene.h"
#include "../ECore/Editor/ui_main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "mxPlacemnt"
#pragma link "ElEdits"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
TfrmObjectList* TfrmObjectList::CreateForm(TWinControl* parent)
{
	TfrmObjectList* OL=xr_new<TfrmObjectList>(parent);
    if (parent) OL->Parent = parent;
	return OL;
}

void TfrmObjectList::DestroyForm(TfrmObjectList*& obj_list)
{
	VERIFY(obj_list);
    xr_delete(obj_list);
}

void __fastcall TfrmObjectList::ShowObjectList()
{
	Show();
}

void __fastcall TfrmObjectList::ShowObjectListModal()
{
	ShowModal();
}

void __fastcall TfrmObjectList::HideObjectList()
{
	Hide();
}

void __fastcall TfrmObjectList::UpdateObjectList()
{
	if (Visible&&!bLockUpdate) sbRefreshListClick(0);
}

//---------------------------------------------------------------------------
__fastcall TfrmObjectList::TfrmObjectList(TComponent* Owner)
    : TForm(Owner)
{
	bLockUpdate = false;
    find_node	= NULL;
}

void __fastcall TfrmObjectList::sbCloseClick(TObject *Sender)
{
    Close();
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::FormShow(TObject *Sender)
{
    obj_count = 0;
    m_cur_cls = OBJCLASS_DUMMY;
	tvItems->FilteredVisibility = ((rgSO->ItemIndex==1)||(rgSO->ItemIndex==2));
    InitListBox();
	// check window position
    UI->CheckWindowPos(this);
}
//---------------------------------------------------------------------------
TElTreeItem* TfrmObjectList::FindFolderByType(int type)
{
    for ( TElTreeItem* node = tvItems->Items->GetFirstNode(); node; node = node->GetNext())
        if (!node->Parent && (node->Tag == type)) return node;
    return 0;
}
//---------------------------------------------------------------------------
TElTreeItem* TfrmObjectList::FindObjectByType(int type, void *obj)
{
    for ( TElTreeItem* node = tvItems->Items->GetFirstNode(); node; node = node->GetNext())
        if (!node->Parent && (node->Tag==type))
        {
            for (TElTreeItem* chield = node->GetFirstChild(); chield; chield = node->GetNextChild(chield))
                if (chield->Data==obj) 
                	return chield;
            break;
        }
    return 0;
}
//---------------------------------------------------------------------------
TElTreeItem* TfrmObjectList::AddFolder(LPCSTR name, void* data, TElTreeItem* parent_node)
{
    for(TElTreeItem* fnode = tvItems->Items->GetFirstNode(); fnode; fnode = fnode->GetNext())
    {
        if( fnode->Parent==parent_node )
         if( AnsiString(fnode->Text)==name )
           return fnode;
    }

    TElTreeItem* node;
    if(parent_node)
    	node 			= tvItems->Items->AddChildObject(parent_node, name, NULL);
    else
    	node 			= tvItems->Items->AddObject(parent_node, name, NULL);
        
    node->ParentStyle 	= false;
    node->Bold 			= true;
    return 				node;
}

TElTreeItem* TfrmObjectList::AddObject(TElTreeItem* node, LPCSTR name, void* obj, TColor color)
{
    TElTreeItem* ret 	= tvItems->Items->AddChildObject(node, name, obj);

    ret->ParentColors 	= false;
    node->ParentStyle = false;
    ret->Color			= color;
    ret->BkColor		= clGray;

    return ret;
}

void gen_object_name(CCustomObject* O, string1024& str_name, bool b_parent_is_group)
{
     if(b_parent_is_group)
     {
       	sprintf					(	str_name,
        							"[%s] %s", 
                                    O->ParentTool->ClassDesc(), 
                                    O->Name);
     }else
     {
        sprintf					(	str_name,
                                    "%s", 
                                    O->Name);
     }
}

void gen_group_name(CGroupObject* O, string1024& str_name)
{
    strcpy				(str_name, O->RefName());

    if(strchr(str_name,'\\'))
    	*strchr(str_name,'\\') = 0;
}

void TfrmObjectList::storeExpandedItems()
{
	ItemsStore.clear();

    for ( TElTreeItem* node = tvItems->Items->GetFirstNode(); node; node = node->GetNext())
    {
    	if(node->Expanded)
		ItemsStore[node->Text].expanded = true;
 	}
}

void TfrmObjectList::restoreExpandedItems()
{
    for ( TElTreeItem* node = tvItems->Items->GetFirstNode(); node; node = node->GetNext())
    {
		FolderStorePairIt it  =  ItemsStore.find(node->Text);
        if (it!=ItemsStore.end())
    		node->Expand(false);
	}
    
}

void __fastcall TfrmObjectList::InitListBox()
{
	storeExpandedItems		();
    tvItems->IsUpdating 	= true;
    tvItems->Items->Clear	();
    m_cur_cls 				= LTools->CurrentClassID();
    string1024				str_name;
    
    for(SceneToolsMapPairIt it=Scene->FirstTool(); it!=Scene->LastTool(); ++it)
    {
    	ESceneCustomOTool* ot = dynamic_cast<ESceneCustomOTool*>(it->second);
        if (ot&&((m_cur_cls==OBJCLASS_DUMMY)||(it->first==m_cur_cls)))
        {
        	if (it->first==OBJCLASS_DUMMY)
            	continue;

            TElTreeItem* tool_node	= FindFolderByType(it->first);
            if (!tool_node)
            {
                AnsiString 			name;
                name.sprintf		("%ss",it->second->ClassDesc());
            	tool_node			= AddFolder(name.LowerCase().c_str(), NULL, NULL);
                tool_node->Tag		= it->first;
            }

            ObjectList& lst 		= ot->GetObjects();
            
            if (OBJCLASS_GROUP==it->first)
            {
                for(ObjectIt _F = lst.begin();_F!=lst.end();++_F)
                {
                    TElTreeItem* group_profile_node     = NULL;
                    
                    gen_group_name				((CGroupObject*)(*_F), str_name);
            		group_profile_node			= AddFolder(str_name, NULL, tool_node);
                    group_profile_node->Tag		= -1;
                    
                    TElTreeItem* grp_node 		= AddObject(group_profile_node, str_name, (void*)(*_F), clBlack);
                    
                    ObjectList 					grp_lst;
                    
                    ((CGroupObject*)(*_F))->GetObjects(grp_lst);
                    
                    for (ObjectIt _G=grp_lst.begin(); _G!=grp_lst.end(); ++_G)
                    {
                    	gen_object_name		(*_G, str_name, true);
                        TElTreeItem* obj	= AddObject	(	grp_node, 
                        									str_name, 
                                                			(void*)(*_G), 
                                                			(*_G)->m_CO_Flags.test(CCustomObject::flObjectInGroupUnique)?clBlack:TColor(0x00a9a6a0));
                    }
                }
                for (TElTreeItem* chield = tool_node->GetFirstChild(); chield; chield = tool_node->GetNextChild(chield))
                	chield->Collapse(true);
                

            }else
            {
                for(ObjectIt _F = lst.begin();_F!=lst.end();++_F)
                {
                	Flags32 fl 					= (*_F)->m_CO_Flags;
               		TElTreeItem* parent_node 	= tool_node;
					if(fl.test(CCustomObject::flObjectInGroup))
					{   
                    	CGroupObject* GO		= (CGroupObject*)((*_F)->GetOwner());
                        string1024 gn;
                        gen_group_name			(GO, gn);
                        sprintf					(str_name,"inGroup %s", gn /*GO->RefName()*/);
                    	parent_node				= AddFolder(str_name, NULL, tool_node);
                    	parent_node->Tag		= -1;
                    }

                    gen_object_name			(*_F, str_name, false);
					TColor clr 				= (fl.test(CCustomObject::flObjectInGroup) && !fl.test(CCustomObject::flObjectInGroupUnique)) ? TColor(0x00a9a6a0):clBlack;
                    AddObject				(parent_node, str_name, (void*)(*_F), clr);
                }
            }
            tool_node->Expand(false);
        }
    }
    tvItems->Sort			(true);
    tvItems->IsUpdating 	= false;

    UpdateState				();
//.    tvItems->FullExpand		();
    obj_count 				= Scene->ObjCount();
    find_node				= NULL;

    restoreExpandedItems	();
}

void TfrmObjectList::UpdateState()
{
    tvItems->IsUpdating 			= true;

	tvItems->OnItemSelectedChange 	= 0;

    TElTreeItem* sel_node 			= NULL;
    u32 sel_count					= 0;
    TElTreeItem* first_sel_node 	= NULL;
    bool need_sort					= false;
    
    for ( TElTreeItem* node = tvItems->Items->GetFirstNode(); node; node = node->GetNext())
    {
    	if(NULL==node->Data)
        {
        	node->Selected = false;
            continue;
        }
        
        TElTreeItem* parent_node	=  node->Parent;
        CCustomObject* PO			= (parent_node && parent_node->Data)?(CCustomObject*)parent_node->Data : NULL;
            
        CCustomObject* O 			= (CCustomObject*)node->Data;
            
        node->ParentStyle 			= false;
        node->StrikeOut 			= !O->Visible();

        if(rgSO->ItemIndex==1) 	
            node->Hidden 			= !O->Visible();
        else 
        if(rgSO->ItemIndex==2)
            node->Hidden 			= O->Visible();
                
        if (O->Visible())			
            node->Selected			= O->Selected();
                
        if (O->Selected())			
        {
            ++sel_count;
            sel_node				= node;
            if(first_sel_node==NULL && O->Visible())
               first_sel_node		= node;
        }

        if (!node->Hidden)
        {
            string1024 				str_name;
            bool b_parent_is_group	= false;
            if(PO && PO->ClassID==OBJCLASS_GROUP)
                b_parent_is_group	= true;
                
            gen_object_name			(O, str_name, b_parent_is_group);
                
            if (AnsiString(node->Text)!=str_name)
            {
                node->Text 			= str_name;
                need_sort			= true;
            }
           	Flags32 fl 				= O->m_CO_Flags;
			TColor clr 				= (fl.test(CCustomObject::flObjectInGroup) && !fl.test(CCustomObject::flObjectInGroupUnique)) ? TColor(0x00a9a6a0):clBlack;
            if(node->Color!=clr)
            	node->Color = clr;
        }
    }

    tvItems->IsUpdating 	= false;

    if (need_sort) 
    	tvItems->Sort		(true);
    
    if (first_sel_node && sel_count==1) 
    	tvItems->EnsureVisible(first_sel_node);
}                                                

void TfrmObjectList::UpdateSelection()
{
	if(tvItems->Items->Count)
    {
        bLockUpdate = true;

        Scene->SelectObjects( false, OBJCLASS_DUMMY/*m_cur_cls*/);
        
        for(TElTreeItem* node = tvItems->GetNextSelected(0); node; node=tvItems->GetNextSelected(node))
        {
            if(node->Data)
            {
            	CCustomObject* O = (CCustomObject*)(node->Data);
                O->Select(true);
                ElEdit1->Text = O->Name;
                
                for(TElTreeItem* child = node->GetFirstChild(); child; child=node->GetNextChild(child))
                {
                    CCustomObject* CO 	= (CCustomObject*)(child->Data);
                    R_ASSERT			(CO);
                    CO->Select			(true);
                }
            }
        }    
        UI->RedrawScene();

        bLockUpdate = false;
    }
}
//---------------------------------------------------------------------------


void __fastcall TfrmObjectList::ebHideSelClick(TObject *Sender)
{
    for (TElTreeItem* node = tvItems->GetNextSelected(0); node; node=tvItems->GetNextSelected(node))
        if (node->Parent) 
        	((CCustomObject*)(node->Data))->Show(false);
            
    UpdateState();
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::ebShowSelClick(TObject *Sender)
{
    for (TElTreeItem* node = tvItems->GetNextSelected(0); node; node=tvItems->GetNextSelected(node))
        if (node->Parent)
        {
            ((CCustomObject*)(node->Data))->Show	(true);
            ((CCustomObject*)(node->Data))->Select	(true);
        }
    UpdateState();
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::ebShowPropertiesClick(TObject *Sender)
{
	ExecCommand	(COMMAND_SHOW_PROPERTIES);
}
//---------------------------------------------------------------------------
BOOL bForceInitListBox = FALSE;
void __fastcall TfrmObjectList::sbRefreshListClick(TObject *Sender)
{
    if((Scene->ObjCount()!=obj_count)||(m_cur_cls!=LTools->CurrentClassID()) || bForceInitListBox)
    {
	    InitListBox();
        bForceInitListBox = FALSE;
    }else
    	UpdateState();
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::tmRefreshListTimer(TObject *Sender)
{
    UpdateState();
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::FormClose(TObject *Sender,
      TCloseAction &Action)
{
    tvItems->IsUpdating 	= true;
    tvItems->Items->Clear	();
    tvItems->IsUpdating 	= false;
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key==VK_ESCAPE) sbClose->Click();
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::rgSOClick(TObject *Sender)
{
	UpdateState();
	tvItems->FilteredVisibility = ((rgSO->ItemIndex==1)||(rgSO->ItemIndex==2));
}
//---------------------------------------------------------------------------

extern bool __fastcall LookupFunc(TElTreeItem* Item, void* SearchDetails);
//---------------------------------------------------------------------------
void __fastcall TfrmObjectList::tvItemsKeyPress(TObject *Sender,
      char &Key)
{
	if (Key==VK_RETURN){
		ExecCommand	(COMMAND_SHOW_PROPERTIES);
    }else{
		TElTreeItem* node = tvItems->Items->LookForItemEx(tvItems->Selected,-1,false,false,false,&Key,LookupFunc);
    	if (!node) 
        	node = tvItems->Items->LookForItemEx(0,-1,false,false,false,&Key,LookupFunc);
            
	    FHelper.RestoreSelection(tvItems,node,false);
    }
}
//---------------------------------------------------------------------------
#include "xr_input.h"
void __fastcall TfrmObjectList::tvItemsItemFocused(TObject *Sender)
{
    UpdateSelection();
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::tvItemsAfterSelectionChange(
      TObject *Sender)
{
    UpdateSelection();
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::tvItemsDblClick(TObject *Sender)
{
	TElTreeItem* node = tvItems->ItemFocused;
    if(node->Data)
		ExecCommand	(COMMAND_SHOW_PROPERTIES);
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::ElEdit1Change(TObject *Sender)
{
    TElEdit* edt 			= (TElEdit*)Sender;
    AnsiString str 			= edt->Text;
    ProcessFindItemInList	(tvItems->Items->GetFirstNode(), str);
}
//---------------------------------------------------------------------------

void TfrmObjectList::ProcessFindItemInList(TElTreeItem* from, AnsiString str)
{
    bool bfound 			= false;
    for ( TElTreeItem* node = from; node; node = node->GetNext())
    {
    	if(NULL==node->Data)
        	continue;
            
        CCustomObject* O 			= (CCustomObject*)node->Data;
        if( strstr(O->FName.c_str(), str.c_str()) )
        {
            if(find_node)
            {
                find_node->ParentColors 	= stored_parent_colors;
                find_node->BkColor			= storred_bk_color;
            }
            stored_parent_colors		= node->ParentColors;
            storred_bk_color			= node->BkColor;
            find_node 					= node;

            find_node->MakeVisible		();
            find_node->ParentColors 	= false;
            find_node->BkColor			= clYellow;

            bfound						= true;
            break;
         }
	}                

    if(!bfound && find_node)
    {
        find_node->ParentColors 	= stored_parent_colors;
        find_node->BkColor			= storred_bk_color;
    	find_node = NULL;
    }
}

void __fastcall TfrmObjectList::ElEdit1Exit(TObject *Sender)
{
    if(find_node)
    {
        find_node->ParentColors 	= stored_parent_colors;
        find_node->BkColor			= storred_bk_color;
    }
//.    find_node = NULL;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------


void __fastcall TfrmObjectList::ElEdit1KeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
	if(Key == VK_F3)	
    {
        TElEdit* edt 			= ElEdit1;
        AnsiString str 			= edt->Text;
        ProcessFindItemInList	((find_node&&find_node->GetNext())?find_node->GetNext():tvItems->Items->GetFirstNode(), str);
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmObjectList::tvItemsMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    TElTreeItem* node 		= tvItems->GetNodeAt(X,Y);
    if(node && NULL==node->Data)
    {
        if(node->Expanded)
            node->Collapse(Button==mbRight);
        else
            node->Expand(Button==mbRight);
    }
}
//---------------------------------------------------------------------------

