#pragma once

namespace XRay
{
namespace ECore
{
namespace Props
{
ref class ShaderFunction;
}
}
}

struct WaveForm;
struct xr_token;

namespace XRay
{
ref class Token;
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

public ref class ShaderFunction : public System::Windows::Forms::Form
{
public:
	ShaderFunction(void);

protected:
	~ShaderFunction()
	{
		if (components)
		{
			delete components;
		}
	}

private:
    bool bLoadMode;

    WaveForm* currentFunc;
    WaveForm* saveFunc;

    void GetFuncData();
    void UpdateFuncData();
    void DrawGraph();

public:
    bool Run(WaveForm* func);
    void FillFunctionsFromToken(const xr_token* tokens);
    XRay::Token^ GetTokenFromValue(int val);

private: System::Void ShaderFunction_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e);
private: System::Void numArgX_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e);
private: System::Void buttonOk_Click(System::Object^  sender, System::EventArgs^  e);
private: System::Void buttonCancel_Click(System::Object^ sender, System::EventArgs^ e);

private: System::Windows::Forms::Label^ label1;
private: System::Windows::Forms::Label^ label2;
private: System::Windows::Forms::Label^ label3;
private: System::Windows::Forms::Label^ label4;
private: System::Windows::Forms::Label^ label5;
private: System::Windows::Forms::Label^ label6;

private: System::Windows::Forms::Label^ labelStart;
private: System::Windows::Forms::Label^ labelMin;
private: System::Windows::Forms::Label^ labelCenter;
private: System::Windows::Forms::Label^ labelMax;
private: System::Windows::Forms::Label^ labelEnd;

private: XRay::SdkControls::NumericSpinner^ numArg1;
private: XRay::SdkControls::NumericSpinner^ numArg2;
private: XRay::SdkControls::NumericSpinner^ numArg3;
private: XRay::SdkControls::NumericSpinner^ numArg4;
private: XRay::SdkControls::NumericSpinner^ numScale;

private: System::Windows::Forms::Panel^ panelLeft;
private: System::Windows::Forms::Panel^ panelRight;

private: System::Windows::Forms::ComboBox^ comboFunctions;
private: System::Windows::Forms::PictureBox^ pbDraw;

private: System::Windows::Forms::Button^ buttonOk;
private: System::Windows::Forms::Button^ buttonCancel;

private:
	System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
	void InitializeComponent(void)
	{
        this->label1 = (gcnew System::Windows::Forms::Label());
        this->label2 = (gcnew System::Windows::Forms::Label());
        this->label3 = (gcnew System::Windows::Forms::Label());
        this->label4 = (gcnew System::Windows::Forms::Label());
        this->label5 = (gcnew System::Windows::Forms::Label());
        this->numArg1 = (gcnew XRay::SdkControls::NumericSpinner());
        this->numArg2 = (gcnew XRay::SdkControls::NumericSpinner());
        this->numArg3 = (gcnew XRay::SdkControls::NumericSpinner());
        this->numArg4 = (gcnew XRay::SdkControls::NumericSpinner());
        this->label6 = (gcnew System::Windows::Forms::Label());
        this->panelLeft = (gcnew System::Windows::Forms::Panel());
        this->comboFunctions = (gcnew System::Windows::Forms::ComboBox());
        this->panelRight = (gcnew System::Windows::Forms::Panel());
        this->numScale = (gcnew XRay::SdkControls::NumericSpinner());
        this->pbDraw = (gcnew System::Windows::Forms::PictureBox());
        this->labelEnd = (gcnew System::Windows::Forms::Label());
        this->labelStart = (gcnew System::Windows::Forms::Label());
        this->labelMin = (gcnew System::Windows::Forms::Label());
        this->labelCenter = (gcnew System::Windows::Forms::Label());
        this->labelMax = (gcnew System::Windows::Forms::Label());
        this->buttonOk = (gcnew System::Windows::Forms::Button());
        this->buttonCancel = (gcnew System::Windows::Forms::Button());
        this->panelLeft->SuspendLayout();
        this->panelRight->SuspendLayout();
        (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbDraw))->BeginInit();
        this->SuspendLayout();
        this->label1->AutoSize = true;
        this->label1->Location = System::Drawing::Point(3, 5);
        this->label1->Name = L"label1";
        this->label1->Size = System::Drawing::Size(51, 13);
        this->label1->TabIndex = 0;
        this->label1->Text = L"Function:";
        this->label2->AutoSize = true;
        this->label2->Location = System::Drawing::Point(3, 33);
        this->label2->Name = L"label2";
        this->label2->Size = System::Drawing::Size(68, 13);
        this->label2->TabIndex = 1;
        this->label2->Text = L"Offset (arg1):";
        this->label3->AutoSize = true;
        this->label3->Location = System::Drawing::Point(3, 60);
        this->label3->Name = L"label3";
        this->label3->Size = System::Drawing::Size(86, 13);
        this->label3->TabIndex = 2;
        this->label3->Text = L"Amplitude (arg2):";
        this->label4->AutoSize = true;
        this->label4->Location = System::Drawing::Point(3, 87);
        this->label4->Name = L"label4";
        this->label4->Size = System::Drawing::Size(70, 13);
        this->label4->TabIndex = 3;
        this->label4->Text = L"Phase (arg3):";
        this->label5->AutoSize = true;
        this->label5->Location = System::Drawing::Point(3, 114);
        this->label5->Name = L"label5";
        this->label5->Size = System::Drawing::Size(63, 13);
        this->label5->TabIndex = 4;
        this->label5->Text = L"Rate (arg4):";
        this->numArg1->DecimalPlaces = 5;
        this->numArg1->Hexadecimal = false;
        this->numArg1->Location = System::Drawing::Point(89, 30);
        this->numArg1->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 100, 0, 0, 0 });
        this->numArg1->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg1->MinimumSize = System::Drawing::Size(70, 22);
        this->numArg1->Name = L"numArg1";
        this->numArg1->Precision = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg1->Size = System::Drawing::Size(115, 22);
        this->numArg1->TabIndex = 5;
        this->numArg1->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
        this->numArg1->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg1->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &ShaderFunction::numArgX_KeyPress);
        this->numArg2->DecimalPlaces = 5;
        this->numArg2->Hexadecimal = false;
        this->numArg2->Location = System::Drawing::Point(89, 57);
        this->numArg2->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 100, 0, 0, 0 });
        this->numArg2->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg2->MinimumSize = System::Drawing::Size(70, 22);
        this->numArg2->Name = L"numArg2";
        this->numArg2->Precision = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg2->Size = System::Drawing::Size(115, 22);
        this->numArg2->TabIndex = 6;
        this->numArg2->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
        this->numArg2->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg2->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &ShaderFunction::numArgX_KeyPress);
        this->numArg3->DecimalPlaces = 5;
        this->numArg3->Hexadecimal = false;
        this->numArg3->Location = System::Drawing::Point(89, 84);
        this->numArg3->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 100, 0, 0, 0 });
        this->numArg3->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg3->MinimumSize = System::Drawing::Size(70, 22);
        this->numArg3->Name = L"numArg3";
        this->numArg3->Precision = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg3->Size = System::Drawing::Size(115, 22);
        this->numArg3->TabIndex = 7;
        this->numArg3->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
        this->numArg3->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg3->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &ShaderFunction::numArgX_KeyPress);
        this->numArg4->DecimalPlaces = 5;
        this->numArg4->Hexadecimal = false;
        this->numArg4->Location = System::Drawing::Point(89, 111);
        this->numArg4->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 100, 0, 0, 0 });
        this->numArg4->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg4->MinimumSize = System::Drawing::Size(70, 22);
        this->numArg4->Name = L"numArg4";
        this->numArg4->Precision = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg4->Size = System::Drawing::Size(115, 22);
        this->numArg4->TabIndex = 8;
        this->numArg4->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
        this->numArg4->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numArg4->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &ShaderFunction::numArgX_KeyPress);
        this->label6->AutoSize = true;
        this->label6->Location = System::Drawing::Point(44, 5);
        this->label6->Name = L"label6";
        this->label6->Size = System::Drawing::Size(192, 13);
        this->label6->TabIndex = 10;
        this->label6->Text = L"y = arg1 + arg2*func((time + arg3)*arg4)";
        this->panelLeft->Controls->Add(this->comboFunctions);
        this->panelLeft->Controls->Add(this->label1);
        this->panelLeft->Controls->Add(this->label2);
        this->panelLeft->Controls->Add(this->numArg4);
        this->panelLeft->Controls->Add(this->label3);
        this->panelLeft->Controls->Add(this->numArg3);
        this->panelLeft->Controls->Add(this->label4);
        this->panelLeft->Controls->Add(this->numArg2);
        this->panelLeft->Controls->Add(this->label5);
        this->panelLeft->Controls->Add(this->numArg1);
        this->panelLeft->Location = System::Drawing::Point(0, 0);
        this->panelLeft->Name = L"panelLeft";
        this->panelLeft->Size = System::Drawing::Size(205, 140);
        this->panelLeft->TabIndex = 11;
        this->comboFunctions->FormattingEnabled = true;
        this->comboFunctions->Location = System::Drawing::Point(89, 1);
        this->comboFunctions->Name = L"comboFunctions";
        this->comboFunctions->Size = System::Drawing::Size(115, 21);
        this->comboFunctions->TabIndex = 9;
        this->panelRight->Controls->Add(this->numScale);
        this->panelRight->Controls->Add(this->pbDraw);
        this->panelRight->Controls->Add(this->labelEnd);
        this->panelRight->Controls->Add(this->labelStart);
        this->panelRight->Controls->Add(this->labelMin);
        this->panelRight->Controls->Add(this->labelCenter);
        this->panelRight->Controls->Add(this->labelMax);
        this->panelRight->Controls->Add(this->label6);
        this->panelRight->Location = System::Drawing::Point(204, 0);
        this->panelRight->Name = L"panelRight";
        this->panelRight->Size = System::Drawing::Size(237, 162);
        this->panelRight->TabIndex = 12;
        this->numScale->DecimalPlaces = 3;
        this->numScale->Hexadecimal = false;
        this->numScale->Location = System::Drawing::Point(103, 141);
        this->numScale->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 100, 0, 0, 0 });
        this->numScale->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numScale->MinimumSize = System::Drawing::Size(70, 22);
        this->numScale->Name = L"numScale";
        this->numScale->Precision = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numScale->Size = System::Drawing::Size(70, 22);
        this->numScale->TabIndex = 18;
        this->numScale->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
        this->numScale->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->pbDraw->Location = System::Drawing::Point(47, 30);
        this->pbDraw->Name = L"pbDraw";
        this->pbDraw->Size = System::Drawing::Size(186, 110);
        this->pbDraw->TabIndex = 17;
        this->pbDraw->TabStop = false;
        this->labelEnd->AutoSize = true;
        this->labelEnd->Location = System::Drawing::Point(189, 143);
        this->labelEnd->Name = L"labelEnd";
        this->labelEnd->Size = System::Drawing::Size(47, 13);
        this->labelEnd->TabIndex = 16;
        this->labelEnd->Text = L"1000 ms";
        this->labelEnd->TextAlign = System::Drawing::ContentAlignment::TopRight;
        this->labelStart->AutoSize = true;
        this->labelStart->Location = System::Drawing::Point(44, 143);
        this->labelStart->Name = L"labelStart";
        this->labelStart->Size = System::Drawing::Size(29, 13);
        this->labelStart->TabIndex = 14;
        this->labelStart->Text = L"0 ms";
        this->labelMin->AutoSize = true;
        this->labelMin->Location = System::Drawing::Point(17, 127);
        this->labelMin->Name = L"labelMin";
        this->labelMin->Size = System::Drawing::Size(24, 13);
        this->labelMin->TabIndex = 13;
        this->labelMin->Text = L"Min";
        this->labelMin->TextAlign = System::Drawing::ContentAlignment::TopRight;
        this->labelCenter->AutoSize = true;
        this->labelCenter->Location = System::Drawing::Point(3, 75);
        this->labelCenter->Name = L"labelCenter";
        this->labelCenter->Size = System::Drawing::Size(38, 13);
        this->labelCenter->TabIndex = 12;
        this->labelCenter->Text = L"Center";
        this->labelCenter->TextAlign = System::Drawing::ContentAlignment::TopRight;
        this->labelMax->AutoSize = true;
        this->labelMax->Location = System::Drawing::Point(17, 30);
        this->labelMax->Name = L"labelMax";
        this->labelMax->Size = System::Drawing::Size(27, 13);
        this->labelMax->TabIndex = 11;
        this->labelMax->Text = L"Max";
        this->labelMax->TextAlign = System::Drawing::ContentAlignment::TopRight;
        this->buttonOk->DialogResult = System::Windows::Forms::DialogResult::OK;
        this->buttonOk->Location = System::Drawing::Point(0, 140);
        this->buttonOk->Name = L"buttonOk";
        this->buttonOk->Size = System::Drawing::Size(89, 22);
        this->buttonOk->TabIndex = 14;
        this->buttonOk->Text = L"Ok";
        this->buttonOk->UseVisualStyleBackColor = true;
        this->buttonOk->Click += gcnew System::EventHandler(this, &ShaderFunction::buttonOk_Click);
        this->buttonCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
        this->buttonCancel->Location = System::Drawing::Point(88, 140);
        this->buttonCancel->Name = L"buttonCancel";
        this->buttonCancel->Size = System::Drawing::Size(116, 22);
        this->buttonCancel->TabIndex = 13;
        this->buttonCancel->Text = L"Cancel";
        this->buttonCancel->UseVisualStyleBackColor = true;
        this->buttonCancel->Click += gcnew System::EventHandler(this, &ShaderFunction::buttonCancel_Click);
        this->AcceptButton = this->buttonOk;
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->CancelButton = this->buttonCancel;
        this->ClientSize = System::Drawing::Size(441, 162);
        this->Controls->Add(this->buttonOk);
        this->Controls->Add(this->buttonCancel);
        this->Controls->Add(this->panelRight);
        this->Controls->Add(this->panelLeft);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedToolWindow;
        this->Name = L"ShaderFunction";
        this->Text = L"ShaderFunction";
        this->panelLeft->ResumeLayout(false);
        this->panelLeft->PerformLayout();
        this->panelRight->ResumeLayout(false);
        this->panelRight->PerformLayout();
        (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbDraw))->EndInit();
        this->ResumeLayout(false);

    }
#pragma endregion
};
} // namespace Props
} // namespace ECore
} // namespace XRay
