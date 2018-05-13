#pragma once

namespace XRay
{
namespace Editor
{
namespace Controls
{
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
private: System::Windows::Forms::Panel^ panel1;

private:
	System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
	void InitializeComponent(void)
	{
        this->panel1 = (gcnew System::Windows::Forms::Panel());
        this->SuspendLayout();
        this->panel1->Dock = System::Windows::Forms::DockStyle::Left;
        this->panel1->Location = System::Drawing::Point(0, 0);
        this->panel1->Name = L"panel1";
        this->panel1->Size = System::Drawing::Size(200, 371);
        this->panel1->TabIndex = 0;
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->ClientSize = System::Drawing::Size(532, 371);
        this->Controls->Add(this->panel1);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::SizableToolWindow;
        this->Name = L"SelectItem";
        this->Text = L"Select Item";
        this->ResumeLayout(false);

    }
#pragma endregion
};
} // namespace Controls
} // namespace Editor
} // namespace XRay
