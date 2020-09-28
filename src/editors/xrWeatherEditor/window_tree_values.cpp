#include "pch.hpp"
#include "window_tree_values.h"

using System::String;
using System::Windows::Forms::TreeNode;
using editor::window_tree_values;
using System::Void;

ref class NodeSorter : public IComparer
{
public:
    virtual int Compare(Object ^ x, Object ^ y)
    {
        TreeNode ^ left = safe_cast<TreeNode ^>(x);
        TreeNode ^ right = safe_cast<TreeNode ^>(y);

        if (left->Nodes->Count)
        {
            if (right->Nodes->Count)
                return (left->Text->CompareTo(right->Text));

            return 0;
        }

        if (right->Nodes->Count)
            return 1;

        return (left->Text->CompareTo(right->Text));
    }
}; // ref class NodeSorter

void window_tree_values::values(property_string_values_value_base::collection_type ^ values, System::String ^ current_value)
{
    TextBox->Text = current_value;
    Result = current_value;

    TreeView->BeginUpdate();

    TreeView->Nodes->Clear();
    TreeView->SelectedNode = nullptr;

    TreeNode ^ selected = nullptr;

    for each(String ^ i in values)
    {
        TreeNode ^ current = nullptr;
        String ^ j = i;
        while (j->Length)
        {
            int index = j->IndexOf('\\');
            String ^ k = index < 0 ? j : j->Substring(0, index);
            j = index < 0 ? "" : j->Substring(index + 1);
            if (!current)
            {
                for each(TreeNode ^ i in TreeView->Nodes)
                {
                    if (i->Text != k)
                        continue;

                    current = i;
                    break;
                }

                if (current)
                    continue;

                current = TreeView->Nodes->Add(k);
                current->ImageIndex = 0;
                current->SelectedImageIndex = 0;
                continue;
            }

            bool found = false;
            for each(TreeNode ^ i in current->Nodes)
            {
                if (i->Text != k)
                    continue;

                found = true;
                current = i;
                break;
            }
            if (!found)
            {
                current->ImageIndex = 0;
                current->SelectedImageIndex = 0;
                current = current->Nodes->Add(k);
            }
        }

        if (current->FullPath == current_value)
            selected = current;

        current->ImageIndex = 2;
        current->SelectedImageIndex = 2;
    }

    TreeView->TreeViewNodeSorter = gcnew NodeSorter();
    TreeView->Sort();

    TreeView->SelectedNode = selected;

    TreeView->EndUpdate();
}

Void window_tree_values::TreeView_AfterCollapse(Object ^ sender, TreeViewEventArgs ^ e)
{
    e->Node->ImageIndex = 0;
    e->Node->SelectedImageIndex = 0;
}

Void window_tree_values::TreeView_AfterExpand(Object ^ sender, TreeViewEventArgs ^ e)
{
    e->Node->ImageIndex = 1;
    e->Node->SelectedImageIndex = 1;
}

Void window_tree_values::TreeView_MouseClick(Object ^ sender, MouseEventArgs ^ e)
{
    if (e->Button != ::MouseButtons::Left)
        return;

    TreeViewHitTestInfo ^ info = TreeView->HitTest(e->Location);
    if (!info->Node)
        return;

    if (info->Node->Nodes->Count)
    {
        TextBox->Text = "";
        Result = "";
        return;
    }

    TextBox->Text = info->Node->FullPath;
    Result = TextBox->Text;
}

Void window_tree_values::TreeView_MouseDoubleClick(Object ^ sender, MouseEventArgs ^ e)
{
    if (e->Button != ::MouseButtons::Left)
        return;

    TreeViewHitTestInfo ^ info = TreeView->HitTest(e->Location);
    if (!info->Node)
        return;

    if (info->Node->Nodes->Count)
    {
        TextBox->Text = "";
        Result = "";
        return;
    }

    TextBox->Text = info->Node->FullPath;
    Result = TextBox->Text;
    DialogResult = System::Windows::Forms::DialogResult::OK;
}
