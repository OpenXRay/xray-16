#include "stdafx.h"
#pragma hdrstop

#include "FolderLib.h"
#include "../../xrServerEntities/PropertiesListHelper.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

CFolderHelper FHelper;
DEFINE_VECTOR(TElTreeItem*,ELVec,ELVecIt);
static ELVec drag_items;

void CFolderHelper::ShowPPMenu(TMxPopupMenu* M, TExtBtn* B){
    POINT pt;
    GetCursorPos(&pt);
	M->Popup(pt.x,pt.y-10);
    if (B) B->MouseManualUp();
}
//---------------------------------------------------------------------------

AnsiString CFolderHelper::GetFolderName(const AnsiString& full_name, AnsiString& dest)
{
    for (int i=full_name.Length(); i>=1; i--)
    	if (full_name[i]=='\\'){
        	dest=full_name.SubString(1,i);
            break;
        }
    return dest.c_str();
}

AnsiString CFolderHelper::GetObjectName(const AnsiString& full_name, AnsiString& dest)
{
    for (int i=full_name.Length(); i>=1; i--)
    	if (full_name[i]=='\\'){
        	dest=full_name.SubString(i+1,full_name.Length());
            break;
        }
    return dest.c_str();
}

// собирает имя от стартового итема до конечного
// может включать либо не включать имя объекта
bool CFolderHelper::MakeName(TElTreeItem* begin_item, TElTreeItem* end_item, AnsiString& name, bool bOnlyFolder)
{
    name = "";
	if (begin_item){
    	TElTreeItem* node = (u32(begin_item->Data)==TYPE_OBJECT)?begin_item->Parent:begin_item;
        while (node){
			name.Insert(node->Text+AnsiString('\\'),0);
        	if (node==end_item) break;
            node=node->Parent;
        }
        if (!bOnlyFolder){
        	if (u32(begin_item->Data)==TYPE_OBJECT) name+=begin_item->Text;
            else return false;
        }
        return true;
    }else{
        return false;
    }
}
//---------------------------------------------------------------------------

bool CFolderHelper::MakeFullName(TElTreeItem* begin_item, TElTreeItem* end_item, AnsiString& name)
{
	if (begin_item){
    	TElTreeItem* node = begin_item;
        name = node->Text;
		node = node->Parent;
        while (node){
			name.Insert(node->Text+AnsiString('\\'),0);
        	if (node==end_item) break;
            node=node->Parent;
        }
        return true;
    }else{
		name = "";
        return false;
    }
}
//---------------------------------------------------------------------------

TElTreeItem* CFolderHelper::FindItemInFolder(EItemType type, TElTree* tv, TElTreeItem* start_folder, const AnsiString& name)
{
   if (start_folder){
        for (TElTreeItem* node=start_folder->GetFirstChild(); node; node=start_folder->GetNextChild(node))
            if (type==((EItemType)(node->Data))&&(node->Text==name)) return node;
    }else{
        for (TElTreeItem* node=tv->Items->GetFirstNode(); node; node=node->GetNextSibling())
            if (type==((EItemType)(node->Data))&&(node->Text==name)) return node;
    }
    return 0;
}
//---------------------------------------------------------------------------

TElTreeItem* CFolderHelper::FindItemInFolder(TElTree* tv, TElTreeItem* start_folder, const AnsiString& name)
{
    if (start_folder){
        for (TElTreeItem* node=start_folder->GetFirstChild(); node; node=start_folder->GetNextChild(node))
            if (node->Text==name) return node;
    }else{
        for (TElTreeItem* node=tv->Items->GetFirstNode(); node; node=node->GetNextSibling())
            if (node->Text==name) return node;
    }
    return 0;
}
//---------------------------------------------------------------------------

TElTreeItem* CFolderHelper::FindItem(TElTree* tv, AnsiString full_name, TElTreeItem** last_valid_node, int* last_valid_idx) 
{
    if (last_valid_node) *last_valid_node=0;
    if (last_valid_idx) *last_valid_idx=-1;
	if (!full_name.IsEmpty()){
        int cnt = _GetItemCount(full_name.c_str(),'\\');
        if (cnt<=0) return 0;

        // find folder item
        int itm = 0;
        AnsiString fld;
        TElTreeItem* node = 0;
        TElTreeItem* last_node = 0;
        do{
            _GetItem(full_name.c_str(),itm++,fld,'\\',"",false);
            last_node = node;
            node = FindItemInFolder(tv,node,fld);
        }while (node&&(itm<cnt));

        if(!node){
            if (last_valid_node) *last_valid_node=last_node;
            if (last_valid_idx) *last_valid_idx=--itm;
        }else{
            if (last_valid_node) *last_valid_node=node;
            if (last_valid_idx) *last_valid_idx=--itm;
        }
        return node;
    }else{
    	return 0;
    }
}
//---------------------------------------------------------------------------

TElTreeItem* CFolderHelper::FindFolder(TElTree* tv, AnsiString full_name, TElTreeItem** last_valid_node, int* last_valid_idx)
{
	int cnt = _GetItemCount(full_name.c_str(),'\\');
    if (cnt<=0) return 0;

    // find folder item
    int itm = 0;
	AnsiString fld;
	TElTreeItem* node = 0;
    TElTreeItem* last_node = 0;
    do{
    	_GetItem(full_name.c_str(),itm++,fld,'\\',"",false);
        last_node = node;
        node = FindItemInFolder(TYPE_FOLDER,tv,node,fld);
    }while (node&&(itm<cnt));

    if(!node){
		if (last_valid_node) *last_valid_node=last_node;
        if (last_valid_idx) *last_valid_idx=--itm;
    }
    return node;
}
//---------------------------------------------------------------------------

TElTreeItem* CFolderHelper::FindObject(TElTree* tv, AnsiString full_name, TElTreeItem** last_valid_node, int* last_valid_idx)
{
	int cnt = _GetItemCount(full_name.c_str(),'\\'); cnt--;
    if (cnt<0) return 0;

    // find folder item
    int itm = 0;
    AnsiString fld;
    TElTreeItem* node = 0;
    TElTreeItem* last_node = 0;
    if (cnt){
        do{
            _GetItem(full_name.c_str(),itm++,fld,'\\',"",false);
            last_node = node;
            node = FindItemInFolder(TYPE_FOLDER,tv,node,fld);
        }while (node&&(itm<cnt));
    }

    if(cnt&&!node){
		if (last_valid_node) *last_valid_node=last_node;
        if (last_valid_idx) *last_valid_idx=--itm;
    }else{
    	// find object item if needed
        AnsiString obj;
        _GetItem(full_name.c_str(),cnt,obj,'\\',"",false);
        last_node = node;
        node = FindItemInFolder(TYPE_OBJECT,tv,node,obj);
        if (!node){
            if (last_valid_node) *last_valid_node=last_node;
            if (last_valid_idx) *last_valid_idx=itm;
        }
    }

    return node;
}
//---------------------------------------------------------------------------

TElTreeItem* CFolderHelper::AppendFolder(TElTree* tv, AnsiString full_name, bool force_icon)
{
    int idx=0;
	TElTreeItem* last_node=0;
    TElTreeItem* node = FindFolder(tv,full_name,&last_node,&idx);

    if (node) return node;

	AnsiString fld;
	int cnt = _GetItemCount(full_name.c_str(),'\\');
	node = last_node;
    for (int itm=idx; itm<cnt; itm++){
    	_GetItem(full_name.c_str(),itm,fld,'\\',"",false);
        node	= LL_CreateFolder(tv,node,fld,force_icon);
    }
	return node;
}
//---------------------------------------------------------------------------

TElTreeItem* CFolderHelper::AppendObject(TElTree* tv, AnsiString full_name, bool allow_duplicate, bool force_icon)
{
    int idx=0;
	TElTreeItem* last_node=0;
	AnsiString fld;
	int fld_cnt = _GetItemCount(full_name.c_str(),'\\')-1;
    if (full_name[full_name.Length()]=='\\') fld_cnt++;
    _GetItems(full_name.c_str(),0,fld_cnt,fld,'\\');
//.
    TElTreeItem* fld_node = !fld.IsEmpty()?FindItem/*FindFolder*/(tv,fld,&last_node,&idx):0;
//.
    if (!fld_node){
	    fld_node = last_node;
    	for (int itm=idx; itm<fld_cnt; itm++){
    		_GetItem(full_name.c_str(),itm,fld,'\\',"",false);
	        fld_node	= LL_CreateFolder(tv,fld_node,fld,force_icon);
    	}
    }
	AnsiString obj;
	_GetItem(full_name.c_str(),fld_cnt,obj,'\\',"",false);

    if (!allow_duplicate&&FindItemInFolder(TYPE_OBJECT,tv,fld_node,obj)) 
    	return 0;

	return LL_CreateObject(tv,fld_node,obj);
}
//---------------------------------------------------------------------------

void CFolderHelper::GenerateFolderName(TElTree* tv, TElTreeItem* node, AnsiString& name,AnsiString pref, bool num_first)
{
    int cnt = 0;
    if (num_first) name.sprintf("%s_%02d",pref,cnt++); else name = pref;
    while (FindItemInFolder(TYPE_FOLDER,tv,node,name))
    	name.sprintf("%s_%02d",pref,cnt++);
}
//---------------------------------------------------------------------------

void CFolderHelper::GenerateObjectName(TElTree* tv, TElTreeItem* node, AnsiString& name,AnsiString pref, bool num_first)
{
    int cnt = 0;
    if (num_first) name.sprintf("%s_%02d",pref,cnt++); else name = pref;
    while (FindItemInFolder(TYPE_OBJECT,tv,node,name))
    	name.sprintf("%s_%02d",pref,cnt++);
}
//---------------------------------------------------------------------------

AnsiString CFolderHelper::ReplacePart(AnsiString old_name, AnsiString ren_part, int level, LPSTR dest)
{
    VERIFY(level<_GetItemCount(old_name.c_str(),'\\'));
    _ReplaceItem(old_name.c_str(),level,ren_part.c_str(),dest,'\\');
    return dest;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Drag'n'Drop
//---------------------------------------------------------------------------
void CFolderHelper::DragDrop(TObject *Sender, TObject* Source, int X, int Y, TOnItemRename after_drag)
{
	R_ASSERT(after_drag);

	TElTree* tv = dynamic_cast<TElTree*>(Sender); VERIFY(tv);

    tv->IsUpdating = true;
    
    TSTItemPart IP=(TSTItemPart)0;
    int 		hc=0;
	TElTreeItem* tgt_folder = tv->GetItemAt(X, Y, IP, hc);
    if (tgt_folder&&(IsObject(tgt_folder))) tgt_folder=tgt_folder->Parent;

    AnsiString base_name;
    MakeName(tgt_folder,0,base_name,true);
    AnsiString cur_fld_name=base_name;
    TElTreeItem* cur_folder=tgt_folder;

//..FS.lock_rescan();
    for (ELVecIt it=drag_items.begin(); it!=drag_items.end(); it++){
        TElTreeItem* item 	= *it;
        int drg_level		= item->Level;

        bool bFolderMove	= IsFolder(item);

        do{
            // проверяем есть ли в таргете такой элемент
            EItemType type 	= EItemType(item->Data);
            TElTreeItem* pNode = FindItemInFolder(type,tv,cur_folder,item->Text);
            if (pNode&&IsObject(item)){
                Msg			("#!Item '%s' already exist in folder '%s'.",AnsiString(item->Text).c_str(),AnsiString(cur_folder->Text).c_str());
                item		= item->GetNext();
                continue;
            }
            // если нет добавляем
            if (!pNode){ 
                pNode 				= (type==TYPE_FOLDER)?LL_CreateFolder(tv,cur_folder,item->Text,item->ForceButtons):LL_CreateObject(tv,cur_folder,item->Text);
                if (type==TYPE_OBJECT) pNode->Assign(item);
            }
            if (IsFolder(item)){
                cur_folder 			= pNode;
                MakeName			(cur_folder,0,cur_fld_name,true);
                item				= item->GetNext();
            }else{
                // rename
                AnsiString old_name, new_name;
                MakeName			(item,0,old_name,false);
                MakeName			(pNode,0,new_name,false);

                after_drag			(old_name.c_str(),new_name.c_str(),TYPE_OBJECT);

                TElTreeItem* parent	= item->Parent;
                // get next item && delete existence
                TElTreeItem* next	= item->GetNext();
                item->Delete		();

                if (parent&&((parent->GetLastChild()==item)||(0==parent->ChildrenCount))){
    //	            if (0==parent->ChildrenCount) parent->Delete();
                    cur_folder = cur_folder?cur_folder->Parent:0;
                }

                item=next;
            }
        }while(item&&(item->Level>drg_level));
        // delete folders
        if (bFolderMove){
            AnsiString 		old_name;
            MakeName		(*it,0,old_name,false);
            after_drag		(old_name.c_str(),0,TYPE_FOLDER);
            (*it)->Delete	();
        }
    }
//..FS.unlock_rescan();

    tv->IsUpdating = false;
 }
//---------------------------------------------------------------------------

void CFolderHelper::DragOver(TObject *Sender, TObject *Source, int X, int Y, TDragState State, bool &Accept)
{
	TElTree* tv = dynamic_cast<TElTree*>(Sender); VERIFY(Sender);
	TElTreeItem* tgt;

    for (ELVecIt it=drag_items.begin(); it!=drag_items.end(); it++){
        TElTreeItem* src=*it;
        TSTItemPart IP;
        int HCol;
        if (!src) Accept = false;
        else{
            tgt = tv->GetItemAt(X, Y, IP, HCol);
            if (tgt){
                if (IsFolder(src)){
                    bool b = true;
                    for (TElTreeItem* itm=tgt->Parent; itm; itm=itm->Parent) if (itm==src){b=false; break;}
                    if (IsFolder(tgt)){
                        Accept = b&&(tgt!=src)&&(src->Parent!=tgt);
                    }else if (IsObject(tgt)){
                        Accept = b&&(src!=tgt->Parent)&&(tgt!=src)&&(tgt->Parent!=src->Parent);
                    }
                }else if (IsObject(src)){
                    if (IsFolder(tgt)){
                        Accept = (tgt!=src)&&(src->Parent!=tgt);
                    }else if (IsObject(tgt)){
                        Accept = (tgt!=src)&&(src->Parent!=tgt->Parent);
                    }
                }
            }else Accept = !!src->Parent;
        }
        if (false==Accept) return;
    }
}
//---------------------------------------------------------------------------

void CFolderHelper::StartDrag(TObject *Sender, TDragObject *&DragObject)
{
	TElTree* tv = dynamic_cast<TElTree*>(Sender); VERIFY(Sender);
	drag_items.clear		();
    for (TElTreeItem* item=tv->GetNextSelected(0); item; item = tv->GetNextSelected(item))
    	drag_items.push_back(item);
}
//---------------------------------------------------------------------------
/*
void CFolderHelper::StartDragNoFolder(TObject *Sender, TDragObject *&DragObject)
{
	TElTree* tv = dynamic_cast<TElTree*>(Sender); VERIFY(Sender);
	if (tv->ItemFocused&&IsObject(tv->ItemFocused)) DragItem = tv->ItemFocused;
  	else											DragItem = 0;
}
*/
//---------------------------------------------------------------------------

bool CFolderHelper::RenameItem(TElTree* tv, TElTreeItem* node, AnsiString& new_text, TOnItemRename OnRename)
{
    R_ASSERT(OnRename);
    if (new_text.IsEmpty()) return false;
    new_text = new_text.LowerCase();

    // find item with some name
    for (TElTreeItem* item=node->GetFirstSibling(); item; item=item->GetNextSibling()){
        if ((item->Text==new_text)&&(item!=node))
            return false;
    }
    AnsiString full_name;
    if (IsFolder(node)){
    	// is folder - rename all folder items
        for (TElTreeItem* item=node->GetFirstChild(); item&&(item->Level>node->Level); item=item->GetNext()){
            if (IsObject(item)){
                MakeName(item,0,full_name,false);
                VERIFY(node->Level<_GetItemCount(full_name.c_str(),'\\'));
                AnsiString new_full_name;
                _ReplaceItem(full_name.c_str(),node->Level,new_text.c_str(),new_full_name,'\\');
		        if (full_name!=new_full_name)
        	        OnRename(full_name.c_str(),new_full_name.c_str(),TYPE_OBJECT);
            }
        }
        AnsiString new_full_name;
        MakeName(node,0,full_name,true);
        _ReplaceItem(full_name.c_str(),node->Level,new_text.c_str(),new_full_name,'\\');
        if (full_name!=new_full_name)
			OnRename(full_name.c_str(),new_full_name.c_str(),TYPE_FOLDER);
    }else if (IsObject(node)){
    	// is object - rename only this item
        MakeName(node,0,full_name,false);
        VERIFY(node->Level<_GetItemCount(full_name.c_str(),'\\'));
        AnsiString new_full_name;
        _ReplaceItem(full_name.c_str(),node->Level,new_text.c_str(),new_full_name,'\\');
        if (full_name!=new_full_name)
			OnRename(full_name.c_str(),new_full_name.c_str(),TYPE_OBJECT);
    }
    tv->Selected=node;
    return true;
}
//------------------------------------------------------------------------------

void CFolderHelper::CreateNewFolder(TElTree* tv, bool bEditAfterCreate)
{
	AnsiString folder;
    AnsiString start_folder;
    MakeName(tv->Selected,0,start_folder,true);
    TElTreeItem* parent = tv->Selected?(IsFolder(tv->Selected)?tv->Selected:tv->Selected->Parent):0;
    GenerateFolderName(tv,parent,folder);
    folder = start_folder+folder;
	TElTreeItem* node = AppendFolder(tv,folder.c_str(),true);
    if (tv->Selected) tv->Selected->Expand(false);
    if (bEditAfterCreate) tv->EditItem(node,-1);
}
//------------------------------------------------------------------------------

BOOL CFolderHelper::RemoveItem(TElTree* tv, TElTreeItem* pNode, TOnItemRemove OnRemoveItem, TOnItemAfterRemove OnAfterRemoveItem)
{
	bool bRes = false;
    R_ASSERT(OnRemoveItem);
    if (pNode){
		tv->IsUpdating = true;
	    TElTreeItem* pSelNode = pNode->GetPrevSibling();
	    if (!pSelNode) pSelNode = pNode->GetNextSibling();
		AnsiString full_name;
    	if (IsFolder(pNode)){
//			if (mrYes==MessageDlg("Delete selected folder?", mtConfirmation, TMsgDlgButtons() << mbYes << mbNo, 0))
            {
                bRes = true;
		        for (TElTreeItem* item=pNode->GetFirstChild(); item&&(item->Level>pNode->Level); item=item->GetNext()){
                    MakeName(item,0,full_name,false);
                	if (IsObject(item)){
                    	bool res		= true;
                    	OnRemoveItem(full_name.c_str(),TYPE_OBJECT,res);
                    	if (!res) bRes	= FALSE;
                    }
                }
                if (bRes){
                    MakeName(pNode,0,full_name,true);
                    bool res			= true;
                	OnRemoveItem(full_name.c_str(),TYPE_FOLDER,res);
                	pNode->Delete();
                    if (!OnAfterRemoveItem.empty()) OnAfterRemoveItem();
                }
        	}
        }
    	if (IsObject(pNode)){
//			if (mrYes==MessageDlg("Delete selected item?", mtConfirmation, TMsgDlgButtons() << mbYes << mbNo, 0))
            {
				MakeName	(pNode,0,full_name,false);
                OnRemoveItem(full_name.c_str(),TYPE_OBJECT,bRes);
	            if (bRes){
                	pNode->Delete();
                    if (!OnAfterRemoveItem.empty()) OnAfterRemoveItem();
                }
        	}
        }
        if (bRes) tv->Selected = pSelNode;
        tv->IsUpdating 	= false;
        tv->SetFocus();
    }else{
		Msg				("#At first select item.");
    }
    return bRes;
}
TElTreeItem* CFolderHelper::ExpandItem(TElTree* tv, TElTreeItem* node)
{
	if (node){
	    tv->IsUpdating 	= true;
        TElTreeItem* folder	= node->Parent;
        while(folder){
			if (folder) folder->Expand(false);
        	if (folder->Parent){
            	folder = folder->Parent;
            }else				break;
        }
	    tv->IsUpdating 	= false;
    }
    return node;
}
TElTreeItem* CFolderHelper::ExpandItem(TElTree* tv, AnsiString full_name)
{
	TElTreeItem* last_valid=0;
    FindItem(tv,full_name,&last_valid);
	return ExpandItem(tv,last_valid);
}
TElTreeItem* CFolderHelper::RestoreSelection(TElTree* tv, TElTreeItem* node, bool bLeaveSel)
{
	if (tv->MultiSelect){
        if (bLeaveSel){
            if (node) node->Selected = true;
        }else{
            tv->DeselectAll();
            if (node) node->Selected = true;
        }
		if (tv->OnAfterSelectionChange) tv->OnAfterSelectionChange(tv);
    }else{
		tv->Selected 		= node;
		if (tv->OnAfterSelectionChange) tv->OnAfterSelectionChange(tv);
    }
	if (node){
		tv->EnsureVisible	(node);
    }
    return node;
}
TElTreeItem* CFolderHelper::RestoreSelection(TElTree* tv, AnsiString full_name, bool bLeaveSel)
{
	TElTreeItem* last_valid=0;
    FindItem(tv,full_name,&last_valid);
	return RestoreSelection(tv,last_valid,bLeaveSel);
}
//------------------------------------------------------------------------------

bool CFolderHelper::NameAfterEdit(TElTreeItem* node, AnsiString value, AnsiString& N)
{
	VERIFY(node);
	N=N.LowerCase();
    if (N.IsEmpty()){ N=value; return false; }
	int cnt=_GetItemCount(N.c_str(),'\\');
    if (cnt>1)		{ N=value; return false; }
    VERIFY(node);

    for (TElTreeItem* itm=node->GetFirstSibling(); itm; itm=itm->GetNextSibling()){
        if ((itm->Text==N)&&(itm!=node)){
	        N=value;
            return false;
        }
    }
    // all right
    node->Text=N;
	cnt=_GetItemCount(value.c_str(),'\\');
    AnsiString new_name;
	_ReplaceItem(value.c_str(),cnt-1,N.c_str(),new_name,'\\');
    N=new_name;
    return true;
}

void DrawBitmap(HDC hdc, const Irect& r, u32* data, u32 w, u32 h)
{
    BITMAPINFO  	bmi;
    bmi.bmiHeader.biSize 		= sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth 		= w;
    bmi.bmiHeader.biHeight 		= h;
    bmi.bmiHeader.biPlanes 		= 1;
    bmi.bmiHeader.biBitCount 		= 32;
    bmi.bmiHeader.biCompression 	= BI_RGB;
    bmi.bmiHeader.biSizeImage 		= 0;
    bmi.bmiHeader.biXPelsPerMeter 	= 0;
    bmi.bmiHeader.biYPelsPerMeter 	= 0;
    bmi.bmiHeader.biClrUsed 		= 0;
    bmi.bmiHeader.biClrImportant 	= 0;

    SetMapMode		(hdc,	MM_ANISOTROPIC	);
    SetStretchBltMode(hdc,	HALFTONE		);
    int err 		= StretchDIBits	(       hdc,
                                                r.x1, r.y1, (r.x2-r.x1), (r.y2-r.y1),
    					        0, 0, w, h, data, &bmi,
                    				DIB_RGB_COLORS, SRCCOPY);
    if (err==GDI_ERROR){
    	Log("!StretchDIBits - Draw failed.");
    }
}

void CFolderHelper::FillRect(HDC hdc, const Irect& r, u32 color)
{
    HBRUSH hbr		= CreateSolidBrush(color);
    ::FillRect		(hdc,(const RECT*)&r,hbr);
    DeleteObject	(hbr);
}

bool CFolderHelper::DrawThumbnail(HDC hdc, const Irect &r, u32* data, u32 w, u32 h)
{
	Irect R			= r;
//	int dw 			= R.width()-R.height();
//	if (dw>=0) R.x2	-= dw;
    bool bRes 		= !!(w*h*4);
    if (bRes){
    	DrawBitmap	(hdc,R,data,w,h); 
    }else{	
    	FillRect	(hdc,R,0x00000000);	
    }
    return bRes;
}
//---------------------------------------------------------------------------

AnsiString CFolderHelper::GenerateName(LPCSTR _pref, int dgt_cnt, TFindObjectByName cb, bool allow_pref_name, bool allow_)
{
	VERIFY		(!cb.empty());
	AnsiString result;
    int counter 		= 0;
   // test exist name
   	xr_string pref	= _pref;
    xr_strlwr			(pref);
    if (allow_pref_name&&pref.size()){
        result	= pref.c_str();
        bool 	res;
        cb		(result.c_str(),res);
        if (!res)return result;
    }
    // generate new name
    string512 	prefix;
   	strcpy		(prefix, pref.c_str());
    string32	mask;
    sprintf		(mask,"%%s%s%%0%dd",allow_?"_":"",dgt_cnt);

    int pref_dgt_cnt	= dgt_cnt+(allow_?1:0);
    int pref_size		= pref.size();
    if (pref_size>pref_dgt_cnt)
    {
        bool del_suff	= false;   
        if (allow_ && (prefix[pref_size-pref_dgt_cnt]=='_'))
        {
        	del_suff	= true;
        	for (int i=pref_size-pref_dgt_cnt+1; i<pref_size; ++i)
            	if (!isdigit(prefix[i]))
                { 
                	del_suff=false; 
                    break; 
                }
        }
        if (del_suff)	
        	prefix[pref_size-pref_dgt_cnt] = 0;
    }
    

    bool 	res;
    do{	
    	result.sprintf	(mask, prefix, counter++);
        res				= false;
        cb				(result.c_str(),res);
    }while(res);
    
    return result;
}
//---------------------------------------------------------------------------

