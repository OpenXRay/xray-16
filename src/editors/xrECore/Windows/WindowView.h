#pragma once

namespace XRay
{
namespace Editor
{
namespace Windows
{
public ref class WindowView : public WeifenLuo::WinFormsUI::Docking::DockContent
{
public:
    WindowView(void)
    {
        InitializeComponent();
    }

    auto GetView() { return view; }
    auto GetViewHandle() { return view->Handle; }

protected:
    ~WindowView()
    {
        if (components)
        {
            delete components;
        }
    }

private: System::Windows::Forms::Panel^ view;
private: System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
    void InitializeComponent(void)
    {
        this->view = (gcnew System::Windows::Forms::Panel());
        this->SuspendLayout();
        this->view->Dock = System::Windows::Forms::DockStyle::Fill;
        this->view->Location = System::Drawing::Point(0, 0);
        this->view->Name = L"view";
        this->view->Size = System::Drawing::Size(284, 264);
        this->view->TabIndex = 0;
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->ClientSize = System::Drawing::Size(284, 264);
        this->Controls->Add(this->view);
        this->Name = L"RenderView";
        this->Text = L"RenderView";
        this->ResumeLayout(false);

    }
#pragma endregion
};
} // namespace Windows
} // namespace Editor
} // namespace XRay
