#pragma once

namespace XRay
{
namespace Editor
{
namespace Controls
{
ref class ItemList;
}
}
}

class ListItem;
using ListItemsVec = xr_vector<ListItem*>;

namespace XRay
{
namespace Editor
{
namespace Controls
{
using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

enum
{
    // set
    ilEditMenu = (1 << 0),
    ilMultiSelect = (1 << 1),
    ilDragAllowed = (1 << 2),
    ilDragCustom = (1 << 3),
    ilFolderStore = (1 << 4),
    ilSuppressIcon = (1 << 5),
    ilSuppressStatus = (1 << 6),

    // internal
    ilRT_FullExpand = (1 << 30),
    //        ilRT_UpdateLocked=(1<<31),
};

public ref class ItemList : public System::Windows::Forms::Form
{
public:
    ItemList(void)
    {
        InitializeComponent();
    }

protected:
    ~ItemList()
    {
        if (components)
        {
            delete components;
        }
    }

public:
    void AssignItems(ListItemsVec& newItems, bool fullExpand, bool fullSort);

private:
    Flags32* flags;
    ListItemsVec* items;

private: XRay::SdkControls::TreeView^ viewItems;
private: System::Windows::Forms::StatusStrip^ statusStrip1;
private: System::Windows::Forms::ToolStripStatusLabel^ toolStripStatusLabel1;
private: System::Windows::Forms::ToolStripStatusLabel^ toolStripStatusLabel2;
private: System::ComponentModel::IContainer^ components;

private:
#pragma region Windows Form Designer generated code
    void InitializeComponent(void)
    {
        this->components = (gcnew System::ComponentModel::Container());
        this->statusStrip1 = (gcnew System::Windows::Forms::StatusStrip());
        this->toolStripStatusLabel1 = (gcnew System::Windows::Forms::ToolStripStatusLabel());
        this->toolStripStatusLabel2 = (gcnew System::Windows::Forms::ToolStripStatusLabel());
        this->viewItems = (gcnew XRay::SdkControls::TreeView());
        this->statusStrip1->SuspendLayout();
        this->SuspendLayout();
        this->statusStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {
            this->toolStripStatusLabel1,
                this->toolStripStatusLabel2
        });
        this->statusStrip1->Location = System::Drawing::Point(0, 242);
        this->statusStrip1->Name = L"statusStrip1";
        this->statusStrip1->Size = System::Drawing::Size(284, 22);
        this->statusStrip1->TabIndex = 0;
        this->statusStrip1->Text = L"statusStrip1";
        this->toolStripStatusLabel1->Name = L"toolStripStatusLabel1";
        this->toolStripStatusLabel1->Size = System::Drawing::Size(78, 17);
        this->toolStripStatusLabel1->Text = L"Items count:";
        this->toolStripStatusLabel2->Name = L"toolStripStatusLabel2";
        this->toolStripStatusLabel2->Size = System::Drawing::Size(0, 17);
        this->viewItems->AutoExpandOnFilter = false;
        this->viewItems->Dock = System::Windows::Forms::DockStyle::Fill;
        this->viewItems->FilterVisible = false;
        this->viewItems->Location = System::Drawing::Point(0, 0);
        this->viewItems->MultiSelect = true;
        this->viewItems->SelectableGroups = false;
        this->viewItems->Name = L"viewItems";
        this->viewItems->PathSeparator = L"/";
        this->viewItems->Size = System::Drawing::Size(284, 242);
        this->viewItems->Source = nullptr;
        this->viewItems->TabIndex = 1;
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->ClientSize = System::Drawing::Size(284, 264);
        this->Controls->Add(this->viewItems);
        this->Controls->Add(this->statusStrip1);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::SizableToolWindow;
        this->Name = L"ItemList";
        this->Text = L"Item List";
        this->statusStrip1->ResumeLayout(false);
        this->statusStrip1->PerformLayout();
        this->ResumeLayout(false);
        this->PerformLayout();

    }
#pragma endregion
};
} // namespace Controls
} // namespace Editor
} // namespace XRay
