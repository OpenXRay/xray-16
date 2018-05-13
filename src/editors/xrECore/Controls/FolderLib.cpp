#include "pch.hpp"

#include "FolderLib.h"
#include "xrServerEntities/PropertiesListHelper.h"

CFolderHelper FHelper;
//DEFINE_VECTOR(TreeNode^, ELVec, ELVecIt);
//static ELVec drag_items;

xr_string CFolderHelper::GetFolderName(const xr_string& full_name, xr_string& dest)
{
    for (int i = full_name.size(); i >= 1; i--)
    {
        if (full_name[i] == '\\')
        {
            dest = full_name.substr(1, i);
            break;
        }
    }
    return dest.c_str();
}

xr_string CFolderHelper::GetObjectName(const xr_string& full_name, xr_string& dest)
{
    for (int i = full_name.size(); i >= 1; i--)
    {
        if (full_name[i] == '\\')
        {
            dest = full_name.substr(i + 1, full_name.size());
            break;
        }
    }
    return dest.c_str();
}

// собирает имя от стартового итема до конечного
// может включать либо не включать имя объекта
bool CFolderHelper::MakeName(TreeNode^ begin_item, TreeNode^ end_item, xr_string& name, bool bOnlyFolder)
{
    name = "";
    if (begin_item)
    {
        TreeNode^ node = (begin_item->Tag == TYPE_OBJECT) ? begin_item->Parent : begin_item;
        msclr::interop::marshal_context ctx;
        while (node)
        {
            name.insert(0, "\\");
            name.insert(0, ctx.marshal_as<pcstr>(node->Text));
            if (node == end_item)
                break;
            node = node->Parent;
        }
        if (!bOnlyFolder)
        {
            if ((u32)begin_item->Tag == TYPE_OBJECT)
                name += ctx.marshal_as<pcstr>(begin_item->Text);
            else
                return false;
        }
        return true;
    }
    else
    {
        return false;
    }
}
//---------------------------------------------------------------------------

bool CFolderHelper::MakeFullName(TreeNode^ begin_item, TreeNode^ end_item, xr_string& name)
{
    if (begin_item)
    {
        TreeNode^ node = begin_item;
        msclr::interop::marshal_context ctx;
        name = ctx.marshal_as<pcstr>(node->Text);
        node = node->Parent;
        while (node)
        {
            name.insert(0, "\\");
            name.insert(0, ctx.marshal_as<pcstr>(node->Text));
            if (node == end_item)
                break;
            node = node->Parent;
        }
        return true;
    }
    else
    {
        name = "";
        return false;
    }
}
//---------------------------------------------------------------------------

TreeNode^ CFolderHelper::FindItemInFolder(
    EItemType type, TreeView^ tv, TreeNode^ start_folder, const xr_string& name)
{
    if (start_folder)
    {
        for each (TreeNode^ node in start_folder->Nodes)
            if (type == (Int32)node->Tag && node->Text == (gcnew String(name.c_str())))
                return node;
    }
    else
    {
        for each (TreeNode^ node in tv->Nodes)
            if (type == (Int32)node->Tag && node->Text == (gcnew String(name.c_str())))
                return node;
    }
    return nullptr;
}
//---------------------------------------------------------------------------

TreeNode^ CFolderHelper::FindItemInFolder(TreeView^ tv, TreeNode^ start_folder, const xr_string& name)
{
    if (start_folder)
    {
        for each (TreeNode^ node in start_folder->Nodes)
            if (node->Text == (gcnew String(name.c_str())))
                return node;
    }
    else
    {
        for each (TreeNode^ node in tv->Nodes)
            if (node->Text == (gcnew String(name.c_str())))
                return node;
    }
    return nullptr;
}
//---------------------------------------------------------------------------

TreeNode^ CFolderHelper::FindItem(
    TreeView^ tv, xr_string full_name, TreeNode^* last_valid_node, int* last_valid_idx)
{
    if (last_valid_node)
        *last_valid_node = nullptr;
    if (last_valid_idx)
        *last_valid_idx = -1;
    if (!full_name.empty())
    {
        int cnt = _GetItemCount(full_name.c_str(), '\\');
        if (cnt <= 0)
            return nullptr;

        // find folder item
        int itm = 0;
        xr_string fld;
        TreeNode^ node = nullptr;
        TreeNode^ last_node = nullptr;
        do
        {
            _GetItem(full_name.c_str(), itm++, fld, '\\', "", false);
            last_node = node;
            node = FindItemInFolder(tv, node, fld);
        } while (node && (itm < cnt));

        if (!node)
        {
            if (last_valid_node)
                *last_valid_node = last_node;
            if (last_valid_idx)
                *last_valid_idx = --itm;
        }
        else
        {
            if (last_valid_node)
                *last_valid_node = node;
            if (last_valid_idx)
                *last_valid_idx = --itm;
        }
        return node;
    }
    else
    {
        return nullptr;
    }
}
//---------------------------------------------------------------------------

TreeNode^ CFolderHelper::FindFolder(
    TreeView^ tv, xr_string full_name, TreeNode^* last_valid_node, int* last_valid_idx)
{
    int cnt = _GetItemCount(full_name.c_str(), '\\');
    if (cnt <= 0)
        return nullptr;

    // find folder item
    int itm = 0;
    xr_string fld;
    TreeNode^ node = nullptr;
    TreeNode^ last_node = nullptr;
    do
    {
        _GetItem(full_name.c_str(), itm++, fld, '\\', "", false);
        last_node = node;
        node = FindItemInFolder(TYPE_FOLDER, tv, node, fld);
    } while (node && (itm < cnt));

    if (!node)
    {
        if (last_valid_node)
            *last_valid_node = last_node;
        if (last_valid_idx)
            *last_valid_idx = --itm;
    }
    return node;
}
//---------------------------------------------------------------------------

TreeNode^ CFolderHelper::FindObject(
    TreeView^ tv, xr_string full_name, TreeNode^* last_valid_node, int* last_valid_idx)
{
    int cnt = _GetItemCount(full_name.c_str(), '\\');
    cnt--;
    if (cnt < 0)
        return nullptr;

    // find folder item
    int itm = 0;
    xr_string fld;
    TreeNode^ node = nullptr;
    TreeNode^ last_node = nullptr;
    if (cnt)
    {
        do
        {
            _GetItem(full_name.c_str(), itm++, fld, '\\', "", false);
            last_node = node;
            node = FindItemInFolder(TYPE_FOLDER, tv, node, fld);
        } while (node && (itm < cnt));
    }

    if (cnt && !node)
    {
        if (last_valid_node)
            *last_valid_node = last_node;
        if (last_valid_idx)
            *last_valid_idx = --itm;
    }
    else
    {
        // find object item if needed
        xr_string obj;
        _GetItem(full_name.c_str(), cnt, obj, '\\', "", false);
        last_node = node;
        node = FindItemInFolder(TYPE_OBJECT, tv, node, obj);
        if (!node)
        {
            if (last_valid_node)
                *last_valid_node = last_node;
            if (last_valid_idx)
                *last_valid_idx = itm;
        }
    }

    return node;
}
//---------------------------------------------------------------------------

TreeNode^ CFolderHelper::AppendFolder(TreeView^ tv, xr_string full_name, bool force_icon)
{
    int idx = 0;
    TreeNode^ last_node = nullptr;
    TreeNode^ node = FindFolder(tv, full_name, &last_node, &idx);

    if (node)
        return node;

    xr_string fld;
    int cnt = _GetItemCount(full_name.c_str(), '\\');
    node = last_node;
    for (int itm = idx; itm < cnt; itm++)
    {
        _GetItem(full_name.c_str(), itm, fld, '\\', "", false);
        node = LL_CreateFolder(node, fld, force_icon);
    }
    return node;
}
//---------------------------------------------------------------------------

TreeNode^ CFolderHelper::AppendObject(TreeView^ tv, xr_string full_name, bool allow_duplicate, bool force_icon)
{
    int idx = 0;
    TreeNode^ last_node = nullptr;
    xr_string fld;
    int fld_cnt = _GetItemCount(full_name.c_str(), '\\') - 1;
    if (full_name[full_name.size()] == '\\')
        fld_cnt++;
    _GetItems(full_name.c_str(), 0, fld_cnt, fld, '\\');
    //.
    TreeNode^ fld_node = !fld.empty() ? /*FindFolder*/FindItem(tv, fld, &last_node, &idx) : nullptr;
    //.
    if (!fld_node)
    {
        fld_node = last_node;
        for (int itm = idx; itm < fld_cnt; itm++)
        {
            _GetItem(full_name.c_str(), itm, fld, '\\', "", false);
            fld_node = LL_CreateFolder(fld_node, fld, force_icon);
        }
    }
    xr_string obj;
    _GetItem(full_name.c_str(), fld_cnt, obj, '\\', "", false);

    if (!allow_duplicate && FindItemInFolder(TYPE_OBJECT, tv, fld_node, obj))
        return nullptr;

    return LL_CreateObject(fld_node, obj);
}
//---------------------------------------------------------------------------

void CFolderHelper::GenerateFolderName(
    TreeView^ tv, TreeNode^ node, xr_string& name, xr_string pref, bool num_first)
{
    int cnt = 0;
    if (num_first)
        xr_sprintf(name.data(), name.size(), "%s_%02d", pref, cnt++);
    else
        name = pref;
    while (FindItemInFolder(TYPE_FOLDER, tv, node, name))
        xr_sprintf(name.data(), name.size(), "%s_%02d", pref, cnt++);
}
//---------------------------------------------------------------------------

void CFolderHelper::GenerateObjectName(
    TreeView^ tv, TreeNode^ node, xr_string& name, xr_string pref, bool num_first)
{
    int cnt = 0;
    if (num_first)
        xr_sprintf(name.data(), name.size(), "%s_%02d", pref, cnt++);
    else
        name = pref;
    while (FindItemInFolder(TYPE_OBJECT, tv, node, name))
        xr_sprintf(name.data(), name.size(), "%s_%02d", pref, cnt++);
}
//---------------------------------------------------------------------------

xr_string CFolderHelper::ReplacePart(xr_string old_name, xr_string ren_part, int level, pstr dest)
{
    VERIFY(level < _GetItemCount(old_name.c_str(), '\\'));
    _ReplaceItem(old_name.c_str(), level, ren_part.c_str(), dest, '\\');
    return dest;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Drag'n'Drop
//---------------------------------------------------------------------------
/*
void CFolderHelper::DragDrop(Object^ Sender, Object^ Source, int X, int Y, TOnItemRename after_drag)
{
    R_ASSERT(after_drag);

    TreeView^ tv = dynamic_cast<TreeView^>(Sender);
    VERIFY(tv);

    tv->BeginUpdate();

    TSTItemPart IP = (TSTItemPart)0;
    int hc = 0;
    TreeNode^ tgt_folder = tv->GetItemAt(X, Y, IP, hc);
    if (tgt_folder && (IsObject(tgt_folder)))
        tgt_folder = tgt_folder->Parent;

    xr_string base_name;
    MakeName(tgt_folder, nullptr, base_name, true);
    xr_string cur_fld_name = base_name;
    TreeNode^ cur_folder = tgt_folder;

    //..FS.lock_rescan();
    for (ELVecIt it = drag_items.begin(); it != drag_items.end(); it++)
    {
        TreeNode^ item = *it;
        int drg_level = item->Level;

        bool bFolderMove = IsFolder(item);

        do
        {
            // проверяем есть ли в таргете такой элемент
            EItemType type = EItemType(item->Tag);
            TreeNode^ pNode = FindItemInFolder(type, tv, cur_folder, msclr::interop::marshal_as<pcstr>(item->Text));
            if (pNode && IsObject(item))
            {
                Msg("#!Item '%s' already exist in folder '%s'.", xr_string(msclr::interop::marshal_as<pcstr>(item->Text)).c_str(),
                    xr_string(msclr::interop::marshal_as<pcstr>(cur_folder->Text)).c_str());
                item = item->GetNext();
                continue;
            }
            // если нет добавляем
            if (!pNode)
            {
                pNode = (type == TYPE_FOLDER) ? LL_CreateFolder(cur_folder, item->Text, item->ForceButtons) :
                                                LL_CreateObject(cur_folder, item->Text);
                if (type == TYPE_OBJECT)
                    pNode->Assign(item);
            }
            if (IsFolder(item))
            {
                cur_folder = pNode;
                MakeName(cur_folder, nullptr, cur_fld_name, true);
                item = item->GetNext();
            }
            else
            {
                // rename
                xr_string old_name, new_name;
                MakeName(item, nullptr, old_name, false);
                MakeName(pNode, nullptr, new_name, false);

                after_drag(old_name.c_str(), new_name.c_str(), TYPE_OBJECT);

                TreeNode^ parent = item->Parent;
                // get next item && delete existence
                TreeNode^ next = item->GetNext();
                item->Remove();

                if (parent && ((parent->GetLastChild() == item) || (0 == parent->ChildrenCount)))
                {
                    //	            if (0==parent->ChildrenCount) parent->Delete();
                    cur_folder = cur_folder ? cur_folder->Parent : 0;
                }

                item = next;
            }
        } while (item && (item->Level > drg_level));
        // delete folders
        if (bFolderMove)
        {
            xr_string old_name;
            MakeName(*it, nullptr, old_name, false);
            after_drag(old_name.c_str(), 0, TYPE_FOLDER);
            (*it)->Remove();
        }
    }
    //..FS.unlock_rescan();

    tv->EndUpdate();
}
//---------------------------------------------------------------------------

void CFolderHelper::DragOver(Object^ Sender, Object^ Source, int X, int Y, TDragState State, bool& Accept)
{
    TreeView^ tv = dynamic_cast<TreeView^>(Sender);
    VERIFY(Sender);
    TreeNode^ tgt;

    for (ELVecIt it = drag_items.begin(); it != drag_items.end(); it++)
    {
        TreeNode^ src = *it;
        TSTItemPart IP;
        int HCol;
        if (!src)
            Accept = false;
        else
        {
            tgt = tv->GetItemAt(X, Y, IP, HCol);
            if (tgt)
            {
                if (IsFolder(src))
                {
                    bool b = true;
                    for (TreeNode^ itm = tgt->Parent; itm; itm = itm->Parent)
                        if (itm == src)
                        {
                            b = false;
                            break;
                        }
                    if (IsFolder(tgt))
                    {
                        Accept = b && (tgt != src) && (src->Parent != tgt);
                    }
                    else if (IsObject(tgt))
                    {
                        Accept = b && (src != tgt->Parent) && (tgt != src) && (tgt->Parent != src->Parent);
                    }
                }
                else if (IsObject(src))
                {
                    if (IsFolder(tgt))
                    {
                        Accept = (tgt != src) && (src->Parent != tgt);
                    }
                    else if (IsObject(tgt))
                    {
                        Accept = (tgt != src) && (src->Parent != tgt->Parent);
                    }
                }
            }
            else
                Accept = !!src->Parent;
        }
        if (false == Accept)
            return;
    }
}
//---------------------------------------------------------------------------

void CFolderHelper::StartDrag(Object^ Sender, TDragObject*& DragObject)
{
    TreeView^ tv = dynamic_cast<TreeView^>(Sender);
    VERIFY(Sender);
    drag_items.clear();
    for (TreeNode^ item = tv->GetNextSelected(0); item; item = tv->GetNextSelected(item))
        drag_items.push_back(item);
}*/
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

bool CFolderHelper::RenameItem(TreeView^ tv, TreeNode^ node, xr_string& new_text, TOnItemRename OnRename)
{
    R_ASSERT(OnRename);
    if (new_text.empty())
        return false;
    xr_strlwr(new_text.data());

    // find item with same name
    for each (TreeNode^ item in node->Nodes)
    {
        if (item->Text == (gcnew String(new_text.c_str())) && item != node)
            return false;
    }
    xr_string full_name;
    if (IsFolder(node))
    {
        // is folder - rename all folder items
        for (TreeNode^ item = node->FirstNode; item && (item->Level > node->Level); item = item->NextNode)
        {
            if (IsObject(item))
            {
                MakeName(item, nullptr, full_name, false);
                VERIFY(node->Level < _GetItemCount(full_name.c_str(), '\\'));
                xr_string new_full_name;
                _ReplaceItem(full_name.c_str(), node->Level, new_text.c_str(), new_full_name, '\\');
                if (full_name != new_full_name)
                    OnRename(full_name.c_str(), new_full_name.c_str(), TYPE_OBJECT);
            }
        }
        xr_string new_full_name;
        MakeName(node, nullptr, full_name, true);
        _ReplaceItem(full_name.c_str(), node->Level, new_text.c_str(), new_full_name, '\\');
        if (full_name != new_full_name)
            OnRename(full_name.c_str(), new_full_name.c_str(), TYPE_FOLDER);
    }
    else if (IsObject(node))
    {
        // is object - rename only this item
        MakeName(node, nullptr, full_name, false);
        VERIFY(node->Level < _GetItemCount(full_name.c_str(), '\\'));
        xr_string new_full_name;
        _ReplaceItem(full_name.c_str(), node->Level, new_text.c_str(), new_full_name, '\\');
        if (full_name != new_full_name)
            OnRename(full_name.c_str(), new_full_name.c_str(), TYPE_OBJECT);
    }
    tv->SelectedNode = node;
    return true;
}
//------------------------------------------------------------------------------

void CFolderHelper::CreateNewFolder(TreeView^ tv, bool bEditAfterCreate)
{
    xr_string folder;
    xr_string start_folder;
    MakeName(tv->SelectedNode, nullptr, start_folder, true);
    TreeNode^ parent = tv->SelectedNode ? (IsFolder(tv->SelectedNode) ? tv->SelectedNode : tv->SelectedNode->Parent) : nullptr;
    GenerateFolderName(tv, parent, folder);
    folder = start_folder + folder;
    TreeNode^ node = AppendFolder(tv, folder.c_str(), true);
    if (tv->SelectedNode)
        tv->SelectedNode->Expand();
    if (bEditAfterCreate)
        node->BeginEdit();
}
//------------------------------------------------------------------------------

bool CFolderHelper::RemoveItem(
    TreeView^ tv, TreeNode^ pNode, TOnItemRemove OnRemoveItem, TOnItemAfterRemove OnAfterRemoveItem)
{
    bool bRes = false;
    R_ASSERT(OnRemoveItem);
    if (pNode)
    {
        tv->BeginUpdate();
        TreeNode^ pSelNode = pNode->PrevNode;
        if (!pSelNode)
            pSelNode = pNode->NextNode;
        xr_string full_name;
        if (IsFolder(pNode))
        {
            //			if (mrYes==MessageDlg("Delete selected folder?", mtConfirmation, TMsgDlgButtons() << mbYes <<
            //mbNo,
            // 0))
            {
                bRes = true;
                for (TreeNode^ item = pNode->FirstNode; item && (item->Level > pNode->Level);
                     item = item->FirstNode)
                {
                    MakeName(item, nullptr, full_name, false);
                    if (IsObject(item))
                    {
                        bool res = true;
                        OnRemoveItem(full_name.c_str(), TYPE_OBJECT, res);
                        if (!res)
                            bRes = false;
                    }
                }
                if (bRes)
                {
                    MakeName(pNode, nullptr, full_name, true);
                    bool res = true;
                    OnRemoveItem(full_name.c_str(), TYPE_FOLDER, res);
                    pNode->Remove();
                    if (!OnAfterRemoveItem.empty())
                        OnAfterRemoveItem();
                }
            }
        }
        if (IsObject(pNode))
        {
            //			if (mrYes==MessageDlg("Delete selected item?", mtConfirmation, TMsgDlgButtons() << mbYes <<
            //mbNo,
            // 0))
            {
                MakeName(pNode, nullptr, full_name, false);
                OnRemoveItem(full_name.c_str(), TYPE_OBJECT, bRes);
                if (bRes)
                {
                    pNode->Remove();
                    if (!OnAfterRemoveItem.empty())
                        OnAfterRemoveItem();
                }
            }
        }
        if (bRes)
            tv->SelectedNode = pSelNode;
        tv->EndUpdate();
        tv->Focus();
    }
    else
    {
        Msg("#At first select item.");
    }
    return bRes;
}
TreeNode^ CFolderHelper::ExpandItem(TreeView^ tv, TreeNode^ node)
{
    if (node)
    {
        tv->BeginUpdate();
        TreeNode^ folder = node->Parent;
        while (folder)
        {
            if (folder)
                folder->Expand();
            if (folder->Parent)
                folder = folder->Parent;
            else
                break;
        }
        tv->EndUpdate();
    }
    return node;
}
TreeNode^ CFolderHelper::ExpandItem(TreeView^ tv, xr_string full_name)
{
    TreeNode^ last_valid = nullptr;
    FindItem(tv, full_name, &last_valid);
    return ExpandItem(tv, last_valid);
}
TreeNode^ CFolderHelper::RestoreSelection(TreeView^ tv, TreeNode^ node, bool bLeaveSel)
{
    if (false/*tv->MultiSelect*/)
    {
        if (bLeaveSel)
        {
            if (node)
                tv->SelectedNode = node;
        }
        else
        {
            if (node)
                tv->SelectedNode = node;
        }
        //if (tv->OnAfterSelectionChange)
        //    tv->OnAfterSelectionChange(tv);
    }
    else
    {
        tv->SelectedNode = node;
        //if (tv->OnAfterSelectionChange)
        //    tv->OnAfterSelectionChange(tv);
    }
    if (node)
    {
        node->EnsureVisible();
    }
    return node;
}
TreeNode^ CFolderHelper::RestoreSelection(TreeView^ tv, xr_string full_name, bool bLeaveSel)
{
    TreeNode^ last_valid = nullptr;
    FindItem(tv, full_name, &last_valid);
    return RestoreSelection(tv, last_valid, bLeaveSel);
}
//------------------------------------------------------------------------------

bool CFolderHelper::NameAfterEdit(TreeNode^ node, xr_string value, xr_string& N)
{
    VERIFY(node);
    xr_strlwr(N.data());
    if (N.empty())
    {
        N = value;
        return false;
    }
    int cnt = _GetItemCount(N.c_str(), '\\');
    if (cnt > 1)
    {
        N = value;
        return false;
    }
    VERIFY(node);

    for each (TreeNode^ itm in node->Nodes)
    {
        if ((itm->Text == gcnew String(N.c_str()) && itm != node))
        {
            N = value;
            return false;
        }
    }
    // all right
    node->Text = gcnew String(N.c_str());
    cnt = _GetItemCount(value.c_str(), '\\');
    xr_string new_name;
    _ReplaceItem(value.c_str(), cnt - 1, N.c_str(), new_name, '\\');
    N = new_name;
    return true;
}

void DrawBitmap(HDC hdc, const Irect& r, u32* data, u32 w, u32 h)
{
    BITMAPINFO bmi;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    SetMapMode(hdc, MM_ANISOTROPIC);
    SetStretchBltMode(hdc, HALFTONE);
    int err =
        StretchDIBits(hdc, r.x1, r.y1, (r.x2 - r.x1), (r.y2 - r.y1), 0, 0, w, h, data, &bmi, DIB_RGB_COLORS, SRCCOPY);
    if (err == GDI_ERROR)
    {
        Log("!StretchDIBits - Draw failed.");
    }
}

void CFolderHelper::FillRect(HDC hdc, const Irect& r, u32 color)
{
    HBRUSH hbr = CreateSolidBrush(color);
    ::FillRect(hdc, (const RECT*)&r, hbr);
    DeleteObject(hbr);
}

bool CFolderHelper::DrawThumbnail(HDC hdc, const Irect& r, u32* data, u32 w, u32 h)
{
    Irect R = r;
    //	int dw 			= R.width()-R.height();
    //	if (dw>=0) R.x2	-= dw;
    bool bRes = !!(w * h * 4);
    if (bRes)
    {
        DrawBitmap(hdc, R, data, w, h);
    }
    else
    {
        FillRect(hdc, R, 0x00000000);
    }
    return bRes;
}
//---------------------------------------------------------------------------

xr_string CFolderHelper::GenerateName(
    pcstr _pref, int dgt_cnt, TFindObjectByName cb, bool allow_pref_name, bool allow_)
{
    VERIFY(!cb.empty());
    xr_string result;
    int counter = 0;
    // test exist name
    xr_string pref = _pref;
    xr_strlwr(pref);
    if (allow_pref_name && pref.size())
    {
        result = pref.c_str();
        bool res;
        cb(result.c_str(), res);
        if (!res)
            return result;
    }
    // generate new name
    string512 prefix;
    xr_strcpy(prefix, pref.c_str());
    string32 mask;
    xr_sprintf(mask, "%%s%s%%0%dd", allow_ ? "_" : "", dgt_cnt);

    int pref_dgt_cnt = dgt_cnt + (allow_ ? 1 : 0);
    int pref_size = pref.size();
    if (pref_size > pref_dgt_cnt)
    {
        bool del_suff = false;
        if (allow_ && (prefix[pref_size - pref_dgt_cnt] == '_'))
        {
            del_suff = true;
            for (int i = pref_size - pref_dgt_cnt + 1; i < pref_size; ++i)
                if (!isdigit(prefix[i]))
                {
                    del_suff = false;
                    break;
                }
        }
        if (del_suff)
            prefix[pref_size - pref_dgt_cnt] = 0;
    }

    bool res;
    do
    {
        xr_sprintf(result.data(), result.size(), mask, prefix, counter++);
        res = false;
        cb(result.c_str(), res);
    } while (res);

    return result;
}
