#pragma once

using namespace System;
using namespace System::Windows::Forms;

using TFindObjectByName = fastdelegate::FastDelegate2<pcstr, bool&>;

class CFolderHelper
{
    inline TreeNode^ LL_CreateFolder(TreeNode^ parent, const xr_string& name, bool force_icon)
    {
        auto node = gcnew TreeNode();
        node->Tag = (u32)TYPE_FOLDER;
        node->Name = gcnew String(name.c_str());
        parent->Nodes->Add(node);
        UNUSED(force_icon);
        return node;
    }

    inline TreeNode^ LL_CreateFolder(TreeNode^ parent, String^ name, bool force_icon)
    {
        auto node = gcnew TreeNode();
        node->Tag = (u32)TYPE_FOLDER;
        node->Name = name;
        parent->Nodes->Add(node);
        UNUSED(force_icon);
        return node;
    }

    inline TreeNode^ LL_CreateObject(TreeNode^ parent, const xr_string& name)
    {
        auto node = gcnew TreeNode();
        node->Tag = (u32)TYPE_OBJECT;
        node->Name = gcnew String(name.c_str());
        parent->Nodes->Add(node);
        return node;
    }

    inline TreeNode^ LL_CreateObject(TreeNode^ parent, String^ name)
    {
        auto node = gcnew TreeNode();
        node->Tag = (u32)TYPE_OBJECT;
        node->Name = name;
        parent->Nodes->Add(node);
        return node;
    }

public:
    inline bool IsFolder(TreeNode^ node) { return node ? (TYPE_FOLDER == (u32)node->Tag) : TYPE_INVALID; }
    inline bool IsObject(TreeNode^ node) { return node ? (TYPE_OBJECT == (u32)node->Tag) : TYPE_INVALID; }
    bool MakeFullName(TreeNode^ begin_item, TreeNode^ end_item, xr_string& folder);
    bool MakeName(TreeNode^ begin_item, TreeNode^ end_item, xr_string& folder, bool bOnlyFolder);
    TreeNode^ FindItemInFolder(TreeView^ tv, TreeNode^ start_folder, const xr_string& name);
    TreeNode^ FindItemInFolder(EItemType type, TreeView^ tv, TreeNode^ start_folder, const xr_string& name);
    TreeNode^ AppendFolder(TreeView^ tv, xr_string full_name, bool force_icon);
    TreeNode^ AppendObject(TreeView^ tv, xr_string full_name, bool allow_duplicate, bool force_icon);
    TreeNode^ FindObject(
        TreeView^ tv, xr_string full_name, TreeNode^* last_valid_node = 0, int* last_valid_idx = 0);
    TreeNode^ FindFolder(
        TreeView^ tv, xr_string full_name, TreeNode^* last_valid_node = 0, int* last_valid_idx = 0);
    TreeNode^ FindItem(
        TreeView^ tv, xr_string full_name, TreeNode^* last_valid_node = 0, int* last_valid_idx = 0);
    void GenerateFolderName(
        TreeView^ tv, TreeNode^ node, xr_string& name, xr_string pref = "folder", bool num_first = false);
    void GenerateObjectName(
        TreeView^ tv, TreeNode^ node, xr_string& name, xr_string pref = "object", bool num_first = false);
    xr_string GetFolderName(const xr_string& full_name, xr_string& dest);
    xr_string GetObjectName(const xr_string& full_name, xr_string& dest);
    xr_string ReplacePart(xr_string old_name, xr_string ren_part, int level, pstr dest);
    bool RenameItem(TreeView^ tv, TreeNode^ node, xr_string& new_text, TOnItemRename OnRenameItem);
    void CreateNewFolder(TreeView^ tv, bool bEditAfterCreate);
    bool RemoveItem(
        TreeView^ tv, TreeNode^ pNode, TOnItemRemove OnRemoveItem, TOnItemAfterRemove OnAfterRemoveItem = 0);
    // drag'n'drop
    /*void __fastcall DragDrop(Object^ Sender, Object^ Source, int X, int Y, TOnItemRename after_drag);
    void __fastcall DragOver(Object^ Sender, Object^ Source, int X, int Y, TDragState State, bool& Accept);
    void __fastcall StartDrag(Object^ Sender, TDragObject*& DragObject);*/
    //	void __fastcall		StartDragNoFolder	(TObject *Sender, TDragObject *&DragObject);
    // name edit
    bool NameAfterEdit(TreeNode^ node, xr_string value, xr_string& N);
    // last selection
    TreeNode^ RestoreSelection(TreeView^ tv, TreeNode^ node, bool bLeaveSel);
    TreeNode^ RestoreSelection(TreeView^ tv, xr_string full_name, bool bLeaveSel);
    TreeNode^ ExpandItem(TreeView^ tv, TreeNode^ node);
    TreeNode^ ExpandItem(TreeView^ tv, xr_string full_name);

    bool DrawThumbnail(HDC hdc, const Irect& R, u32* data, u32 w, u32 h);
    void FillRect(HDC hdc, const Irect& r, u32 color);

    xr_string GenerateName(pcstr pref, int dgt_cnt, TFindObjectByName cb, bool allow_pref_name, bool allow_);
    //------------------------------------------------------------------------------
};

extern XR_EPROPS_API CFolderHelper FHelper;
