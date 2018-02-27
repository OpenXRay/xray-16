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

public ref class TextEdit : public System::Windows::Forms::Form
{
public:
    TextEdit(void)
    {
        InitializeComponent();
    }

protected:
    ~TextEdit()
    {
        if (components)
        {
            delete components;
        }
    }

public:
    bool Run();

private:
    xr_string* m_text;

private: System::Void buttonOk_Click(System::Object^ sender, System::EventArgs^ e);
private: System::Void buttonClear_Click(System::Object^ sender, System::EventArgs^ e);

private: System::Windows::Forms::Button^ buttonOk;
private: System::Windows::Forms::Button^ buttonCancel;
private: System::Windows::Forms::Button^ buttonApply;
private: System::Windows::Forms::Button^ buttonLoad;
private: System::Windows::Forms::Button^ buttonSave;
private: System::Windows::Forms::Button^ buttonClear;

private: System::Windows::Forms::Panel^ panel1;
private: System::Windows::Forms::TextBox^ textBox1;

private: System::Windows::Forms::StatusStrip^ statusStrip1;
private: System::Windows::Forms::ToolStripStatusLabel^ toolStripStatusLabel1;
private: System::Windows::Forms::ToolStripStatusLabel^ toolStripStatusLabel2;
private: System::Windows::Forms::ToolStripStatusLabel^ toolStripStatusLabel3;


private:
    System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
    void InitializeComponent(void)
    {
        this->textBox1 = (gcnew System::Windows::Forms::TextBox());
        this->buttonOk = (gcnew System::Windows::Forms::Button());
        this->buttonCancel = (gcnew System::Windows::Forms::Button());
        this->buttonApply = (gcnew System::Windows::Forms::Button());
        this->buttonLoad = (gcnew System::Windows::Forms::Button());
        this->buttonSave = (gcnew System::Windows::Forms::Button());
        this->buttonClear = (gcnew System::Windows::Forms::Button());
        this->panel1 = (gcnew System::Windows::Forms::Panel());
        this->statusStrip1 = (gcnew System::Windows::Forms::StatusStrip());
        this->toolStripStatusLabel1 = (gcnew System::Windows::Forms::ToolStripStatusLabel());
        this->toolStripStatusLabel2 = (gcnew System::Windows::Forms::ToolStripStatusLabel());
        this->toolStripStatusLabel3 = (gcnew System::Windows::Forms::ToolStripStatusLabel());
        this->panel1->SuspendLayout();
        this->statusStrip1->SuspendLayout();
        this->SuspendLayout();
        // 
        // textBox1
        // 
        this->textBox1->AcceptsReturn = true;
        this->textBox1->AcceptsTab = true;
        this->textBox1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
            | System::Windows::Forms::AnchorStyles::Left)
            | System::Windows::Forms::AnchorStyles::Right));
        this->textBox1->Location = System::Drawing::Point(0, 25);
        this->textBox1->Multiline = true;
        this->textBox1->Name = L"textBox1";
        this->textBox1->ScrollBars = System::Windows::Forms::ScrollBars::Both;
        this->textBox1->Size = System::Drawing::Size(445, 217);
        this->textBox1->TabIndex = 0;
        // 
        // buttonOk
        // 
        this->buttonOk->Anchor = System::Windows::Forms::AnchorStyles::Top;
        this->buttonOk->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
        this->buttonOk->DialogResult = System::Windows::Forms::DialogResult::OK;
        this->buttonOk->Location = System::Drawing::Point(0, 0);
        this->buttonOk->Name = L"buttonOk";
        this->buttonOk->Size = System::Drawing::Size(75, 23);
        this->buttonOk->TabIndex = 1;
        this->buttonOk->Text = L"Ok";
        this->buttonOk->UseVisualStyleBackColor = true;
        this->buttonOk->Click += gcnew System::EventHandler(this, &TextEdit::buttonOk_Click);
        // 
        // buttonCancel
        // 
        this->buttonCancel->Anchor = System::Windows::Forms::AnchorStyles::Top;
        this->buttonCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
        this->buttonCancel->Location = System::Drawing::Point(74, 0);
        this->buttonCancel->Name = L"buttonCancel";
        this->buttonCancel->Size = System::Drawing::Size(75, 23);
        this->buttonCancel->TabIndex = 2;
        this->buttonCancel->Text = L"Cancel";
        this->buttonCancel->UseVisualStyleBackColor = true;
        // 
        // buttonApply
        // 
        this->buttonApply->Anchor = System::Windows::Forms::AnchorStyles::Top;
        this->buttonApply->Location = System::Drawing::Point(148, 0);
        this->buttonApply->Name = L"buttonApply";
        this->buttonApply->Size = System::Drawing::Size(75, 23);
        this->buttonApply->TabIndex = 3;
        this->buttonApply->Text = L"Apply";
        this->buttonApply->UseVisualStyleBackColor = true;
        // 
        // buttonLoad
        // 
        this->buttonLoad->Anchor = System::Windows::Forms::AnchorStyles::Top;
        this->buttonLoad->Location = System::Drawing::Point(222, 0);
        this->buttonLoad->Name = L"buttonLoad";
        this->buttonLoad->Size = System::Drawing::Size(75, 23);
        this->buttonLoad->TabIndex = 4;
        this->buttonLoad->Text = L"Load";
        this->buttonLoad->UseVisualStyleBackColor = true;
        // 
        // buttonSave
        // 
        this->buttonSave->Anchor = System::Windows::Forms::AnchorStyles::Top;
        this->buttonSave->Location = System::Drawing::Point(296, 0);
        this->buttonSave->Name = L"buttonSave";
        this->buttonSave->Size = System::Drawing::Size(75, 23);
        this->buttonSave->TabIndex = 5;
        this->buttonSave->Text = L"Save";
        this->buttonSave->UseVisualStyleBackColor = true;
        // 
        // buttonClear
        // 
        this->buttonClear->Anchor = System::Windows::Forms::AnchorStyles::Top;
        this->buttonClear->Location = System::Drawing::Point(370, 0);
        this->buttonClear->Name = L"buttonClear";
        this->buttonClear->Size = System::Drawing::Size(75, 23);
        this->buttonClear->TabIndex = 6;
        this->buttonClear->Text = L"Clear";
        this->buttonClear->UseVisualStyleBackColor = true;
        this->buttonClear->Click += gcnew System::EventHandler(this, &TextEdit::buttonClear_Click);
        // 
        // panel1
        // 
        this->panel1->Controls->Add(this->buttonOk);
        this->panel1->Controls->Add(this->buttonClear);
        this->panel1->Controls->Add(this->buttonCancel);
        this->panel1->Controls->Add(this->buttonSave);
        this->panel1->Controls->Add(this->buttonApply);
        this->panel1->Controls->Add(this->buttonLoad);
        this->panel1->Dock = System::Windows::Forms::DockStyle::Top;
        this->panel1->Location = System::Drawing::Point(0, 0);
        this->panel1->Name = L"panel1";
        this->panel1->Size = System::Drawing::Size(445, 24);
        this->panel1->TabIndex = 7;
        // 
        // statusStrip1
        // 
        this->statusStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {
            this->toolStripStatusLabel1,
                this->toolStripStatusLabel2, this->toolStripStatusLabel3
        });
        this->statusStrip1->Location = System::Drawing::Point(0, 242);
        this->statusStrip1->Name = L"statusStrip1";
        this->statusStrip1->Size = System::Drawing::Size(445, 22);
        this->statusStrip1->TabIndex = 8;
        this->statusStrip1->Text = L"statusStrip1";
        // 
        // toolStripStatusLabel1
        // 
        this->toolStripStatusLabel1->BorderStyle = System::Windows::Forms::Border3DStyle::RaisedInner;
        this->toolStripStatusLabel1->Name = L"toolStripStatusLabel1";
        this->toolStripStatusLabel1->Size = System::Drawing::Size(54, 17);
        this->toolStripStatusLabel1->Text = L"Position";
        // 
        // toolStripStatusLabel2
        // 
        this->toolStripStatusLabel2->Enabled = false;
        this->toolStripStatusLabel2->Name = L"toolStripStatusLabel2";
        this->toolStripStatusLabel2->Size = System::Drawing::Size(0, 17);
        // 
        // toolStripStatusLabel3
        // 
        this->toolStripStatusLabel3->BorderStyle = System::Windows::Forms::Border3DStyle::RaisedInner;
        this->toolStripStatusLabel3->Name = L"toolStripStatusLabel3";
        this->toolStripStatusLabel3->Size = System::Drawing::Size(376, 17);
        this->toolStripStatusLabel3->Spring = true;
        this->toolStripStatusLabel3->Text = L"Description";
        // 
        // TextEdit
        // 
        this->AcceptButton = this->buttonOk;
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->CancelButton = this->buttonCancel;
        this->ClientSize = System::Drawing::Size(445, 264);
        this->Controls->Add(this->statusStrip1);
        this->Controls->Add(this->panel1);
        this->Controls->Add(this->textBox1);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::SizableToolWindow;
        this->Name = L"TextEdit";
        this->Text = L"TextForm";
        this->panel1->ResumeLayout(false);
        this->statusStrip1->ResumeLayout(false);
        this->statusStrip1->PerformLayout();
        this->ResumeLayout(false);
        this->PerformLayout();

    }
#pragma endregion
};
}
}
}
