#pragma once

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

public ref class SelectItem : public System::Windows::Forms::Form
{
public:
	SelectItem(void)
	{
		InitializeComponent();
	}

protected:
	~SelectItem()
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
        // 
        // SelectItem
        // 
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->ClientSize = System::Drawing::Size(284, 264);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::SizableToolWindow;
        this->Name = L"SelectItem";
        this->Text = L"Select Item";
        this->ResumeLayout(false);

    }
#pragma endregion
};
} // namespace Props
} // namespace ECore
} // namespace XRay
