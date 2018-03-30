#pragma once

namespace XRay
{
namespace Editor
{
namespace Windows
{
ref class WindowIDE;
ref class WindowView;
ref class WindowLog;
}
}
}

namespace XRay
{
namespace Editor
{
namespace Windows
{
using namespace System;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

public ref class WindowIDE : public System::Windows::Forms::Form
{
public:
    WindowIDE(void)
    {
        InitializeComponent();
        Initialize();
    }

    WindowView% View() { return *windowView; }
    WindowLog% Log() { return *windowLog; }

protected:
    ~WindowIDE()
    {
        if (components)
        {
            delete components;
        }
    }

private:
    void Initialize();
    WeifenLuo::WinFormsUI::Docking::IDockContent^ reloadContent(System::String^ persistString);

private: System::Void WindowIDE_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e);

private: WeifenLuo::WinFormsUI::Docking::DockPanel^ editorDock;
private: WeifenLuo::WinFormsUI::Docking::VS2015LightTheme^ editorTheme;
private: WindowView^ windowView;
private: WindowLog^ windowLog;


private: System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
    void InitializeComponent(void)
    {
        System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(WindowIDE::typeid));
        this->editorTheme = (gcnew WeifenLuo::WinFormsUI::Docking::VS2015LightTheme());
        this->editorDock = (gcnew WeifenLuo::WinFormsUI::Docking::DockPanel());
        this->SuspendLayout();
        this->editorDock->Dock = System::Windows::Forms::DockStyle::Fill;
        this->editorDock->DockBackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(238)), static_cast<System::Int32>(static_cast<System::Byte>(238)),
            static_cast<System::Int32>(static_cast<System::Byte>(242)));
        this->editorDock->DocumentStyle = WeifenLuo::WinFormsUI::Docking::DocumentStyle::DockingSdi;
        this->editorDock->Location = System::Drawing::Point(0, 0);
        this->editorDock->Name = L"editorDock";
        this->editorDock->Padding = System::Windows::Forms::Padding(6);
        this->editorDock->ShowAutoHideContentOnHover = false;
        this->editorDock->Size = System::Drawing::Size(284, 264);
        this->editorDock->TabIndex = 0;
        this->editorDock->Theme = this->editorTheme;
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->ClientSize = System::Drawing::Size(284, 264);
        this->Controls->Add(this->editorDock);
        this->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
        this->Name = L"WindowIDE";
        this->Text = L"OpenXRay Editor";
        this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &WindowIDE::WindowIDE_FormClosing);
        this->ResumeLayout(false);

    }
#pragma endregion
};
} // namespace Windows
} // namespace Editor
} // namespace XRay
