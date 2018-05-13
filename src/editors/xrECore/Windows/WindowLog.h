#pragma once

namespace XRay
{
namespace Editor
{
namespace Windows
{
ref class WindowLog;
}
}
}

namespace WeifenLuo
{
namespace WinFormsUI
{
namespace Docking
{
ref class DockContent;
}
}
}

#include "Core/ELog.h"

namespace XRay
{
namespace Editor
{
namespace Windows
{
using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Drawing;

public ref class WindowLog : public WeifenLuo::WinFormsUI::Docking::DockContent
{
public:
    WindowLog(void)
    {
        InitializeComponent();
    }

protected:
    ~WindowLog()
    {
        if (components)
        {
            delete components;
        }
    }

public:
    void AddMessage(MessageType type, System::String^ message);
    inline void AddMessage(System::String^ message) { AddMessage(MessageType::Custom, message); }

    void AddDialogMessage(MessageType type, System::String^ message);
    inline void AddDialogMessage(System::String^ message) { AddDialogMessage(MessageType::Custom, message); }

    delegate void AddMessageDelegate(MessageType type, System::String^ text);

private: System::Void buttonClose_Click(System::Object^ sender, System::EventArgs^ e);
private: System::Void buttonFlush_Click(System::Object^ sender, System::EventArgs^ e);
private: System::Void buttonClear_Click(System::Object^ sender, System::EventArgs^ e);
private: System::Void buttonClearSelected_Click(System::Object^ sender, System::EventArgs^ e);

private: System::Void logList_Resize(System::Object^ sender, System::EventArgs^ e);
private: System::Void LogForm_Closing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e);
private: System::Void LogForm_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e);

private: System::Windows::Forms::ListView^ logList;
private: System::Windows::Forms::ColumnHeader^ columnHeader1;

private: System::Windows::Forms::Panel^ panel1;
private: System::Windows::Forms::Button^ buttonClose;
private: System::Windows::Forms::Button^ buttonClearSelected;
private: System::Windows::Forms::Button^ buttonClear;
private: System::Windows::Forms::Button^ buttonFlush;

private:
    System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
    void InitializeComponent(void)
    {
        this->logList = (gcnew System::Windows::Forms::ListView());
        this->columnHeader1 = (gcnew System::Windows::Forms::ColumnHeader());
        this->buttonClose = (gcnew System::Windows::Forms::Button());
        this->panel1 = (gcnew System::Windows::Forms::Panel());
        this->buttonClearSelected = (gcnew System::Windows::Forms::Button());
        this->buttonClear = (gcnew System::Windows::Forms::Button());
        this->buttonFlush = (gcnew System::Windows::Forms::Button());
        this->panel1->SuspendLayout();
        this->SuspendLayout();
        this->logList->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
            | System::Windows::Forms::AnchorStyles::Left)
            | System::Windows::Forms::AnchorStyles::Right));
        this->logList->AutoArrange = false;
        this->logList->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(1) { this->columnHeader1 });
        this->logList->FullRowSelect = true;
        this->logList->GridLines = true;
        this->logList->HeaderStyle = System::Windows::Forms::ColumnHeaderStyle::None;
        this->logList->Location = System::Drawing::Point(0, 0);
        this->logList->Name = L"logList";
        this->logList->Size = System::Drawing::Size(337, 205);
        this->logList->TabIndex = 0;
        this->logList->UseCompatibleStateImageBehavior = false;
        this->logList->View = System::Windows::Forms::View::Details;
        this->logList->Resize += gcnew System::EventHandler(this, &WindowLog::logList_Resize);
        this->columnHeader1->Width = 333;
        this->buttonClose->Location = System::Drawing::Point(0, 0);
        this->buttonClose->Name = L"buttonClose";
        this->buttonClose->Size = System::Drawing::Size(85, 23);
        this->buttonClose->TabIndex = 2;
        this->buttonClose->Text = L"Close";
        this->buttonClose->UseVisualStyleBackColor = true;
        this->buttonClose->Click += gcnew System::EventHandler(this, &WindowLog::buttonClose_Click);
        this->panel1->Controls->Add(this->buttonClearSelected);
        this->panel1->Controls->Add(this->buttonClear);
        this->panel1->Controls->Add(this->buttonFlush);
        this->panel1->Controls->Add(this->buttonClose);
        this->panel1->Dock = System::Windows::Forms::DockStyle::Bottom;
        this->panel1->Location = System::Drawing::Point(0, 205);
        this->panel1->Name = L"panel1";
        this->panel1->Size = System::Drawing::Size(337, 23);
        this->panel1->TabIndex = 3;
        this->buttonClearSelected->Anchor = System::Windows::Forms::AnchorStyles::Top;
        this->buttonClearSelected->Location = System::Drawing::Point(168, 0);
        this->buttonClearSelected->Name = L"buttonClearSelected";
        this->buttonClearSelected->Size = System::Drawing::Size(85, 23);
        this->buttonClearSelected->TabIndex = 5;
        this->buttonClearSelected->Text = L"Clear Selected";
        this->buttonClearSelected->UseVisualStyleBackColor = true;
        this->buttonClearSelected->Click += gcnew System::EventHandler(this, &WindowLog::buttonClearSelected_Click);
        this->buttonClear->Anchor = System::Windows::Forms::AnchorStyles::Top;
        this->buttonClear->Location = System::Drawing::Point(84, 0);
        this->buttonClear->Name = L"buttonClear";
        this->buttonClear->Size = System::Drawing::Size(85, 23);
        this->buttonClear->TabIndex = 4;
        this->buttonClear->Text = L"Clear";
        this->buttonClear->UseVisualStyleBackColor = true;
        this->buttonClear->Click += gcnew System::EventHandler(this, &WindowLog::buttonClear_Click);
        this->buttonFlush->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
        this->buttonFlush->Location = System::Drawing::Point(252, 0);
        this->buttonFlush->Name = L"buttonFlush";
        this->buttonFlush->Size = System::Drawing::Size(85, 23);
        this->buttonFlush->TabIndex = 3;
        this->buttonFlush->Text = L"Flush";
        this->buttonFlush->UseVisualStyleBackColor = true;
        this->buttonFlush->Click += gcnew System::EventHandler(this, &WindowLog::buttonFlush_Click);
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->ClientSize = System::Drawing::Size(337, 228);
        this->Controls->Add(this->panel1);
        this->Controls->Add(this->logList);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::SizableToolWindow;
        this->MinimumSize = System::Drawing::Size(353, 264);
        this->Name = L"WindowLog";
        this->Text = L"Log";
        this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &WindowLog::LogForm_Closing);
        this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &WindowLog::LogForm_KeyDown);
        this->panel1->ResumeLayout(false);
        this->ResumeLayout(false);

    }
#pragma endregion
};
} // namespace Windows
} // namespace Editor
} // namespace XRay
