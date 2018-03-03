#pragma once

namespace XRay
{
namespace ECore
{
namespace Props
{
ref class ItemList;
}
}
}

namespace XRay
{
namespace ECore
{
namespace Props
{
using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

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

private:
    System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
    void InitializeComponent(void)
    {
        this->SuspendLayout();
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->ClientSize = System::Drawing::Size(284, 264);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::SizableToolWindow;
        this->Name = L"ItemList";
        this->Text = L"Item List";
        this->ResumeLayout(false);

    }
#pragma endregion
};
} // namespace Props
} // namespace ECore
} // namespace XRay
