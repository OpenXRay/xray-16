#pragma once

namespace XRay
{
namespace ECore
{
namespace Props
{
ref class NumericVector;
}
}
}

template <class T>
struct _vector3;
using Fvector = _vector3<float>;

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

public ref class NumericVector : public System::Windows::Forms::Form
{
public:
    NumericVector()
    {
        InitializeComponent();
    }

    bool Run(pcstr title, Fvector* data, int decimal, Fvector* reset_value, Fvector* min, Fvector* max, int* X, int* Y);

protected:
    ~NumericVector()
    {
        if (components)
        {
            delete components;
        }
    }

private: System::Void buttonApply_Click(System::Object^ sender, System::EventArgs^ e);
private: System::Void buttonOk_Click(System::Object^ sender, System::EventArgs^ e);
private: System::Void buttonReset_Click(System::Object^ sender, System::EventArgs^ e);
private: System::Void buttonImmediate_Click(System::Object^ sender, System::EventArgs^ e);
private: System::Void NumericVector_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e);

private: System::Void OnValueChanged(System::Object^ sender, System::EventArgs^ e);


private: Fvector* Value;
private: Fvector* ResetValue;
private: System::Windows::Forms::Button^ buttonOk;
private: System::Windows::Forms::Button^ buttonCancel;
private: System::Windows::Forms::Button^ buttonReset;
private: System::Windows::Forms::Button^ buttonApply;
private: System::Windows::Forms::Button^ buttonImmediate;
private: System::Windows::Forms::CheckBox^ checkImmediate;
private: XRay::SdkControls::NumericSpinner^ numX;
private: XRay::SdkControls::NumericSpinner^ numY;
private: XRay::SdkControls::NumericSpinner^ numZ;
private: System::Windows::Forms::Label^ labelX;
private: System::Windows::Forms::Label^ labelY;
private: System::Windows::Forms::Label^ labelZ;

private:
    System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
    void InitializeComponent(void)
    {
        this->buttonOk = (gcnew System::Windows::Forms::Button());
        this->buttonCancel = (gcnew System::Windows::Forms::Button());
        this->buttonReset = (gcnew System::Windows::Forms::Button());
        this->numX = (gcnew XRay::SdkControls::NumericSpinner());
        this->numY = (gcnew XRay::SdkControls::NumericSpinner());
        this->numZ = (gcnew XRay::SdkControls::NumericSpinner());
        this->labelX = (gcnew System::Windows::Forms::Label());
        this->labelY = (gcnew System::Windows::Forms::Label());
        this->labelZ = (gcnew System::Windows::Forms::Label());
        this->buttonApply = (gcnew System::Windows::Forms::Button());
        this->buttonImmediate = (gcnew System::Windows::Forms::Button());
        this->checkImmediate = (gcnew System::Windows::Forms::CheckBox());
        this->SuspendLayout();
        this->buttonOk->DialogResult = System::Windows::Forms::DialogResult::OK;
        this->buttonOk->Location = System::Drawing::Point(136, 21);
        this->buttonOk->Name = L"buttonOk";
        this->buttonOk->Size = System::Drawing::Size(50, 22);
        this->buttonOk->TabIndex = 0;
        this->buttonOk->Text = L"Ok";
        this->buttonOk->UseVisualStyleBackColor = true;
        this->buttonOk->Click += gcnew System::EventHandler(this, &NumericVector::buttonOk_Click);
        this->buttonCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
        this->buttonCancel->Location = System::Drawing::Point(136, 42);
        this->buttonCancel->Name = L"buttonCancel";
        this->buttonCancel->Size = System::Drawing::Size(50, 22);
        this->buttonCancel->TabIndex = 1;
        this->buttonCancel->Text = L"Cancel";
        this->buttonCancel->UseVisualStyleBackColor = true;
        this->buttonReset->Location = System::Drawing::Point(136, 63);
        this->buttonReset->Name = L"buttonReset";
        this->buttonReset->Size = System::Drawing::Size(50, 22);
        this->buttonReset->TabIndex = 2;
        this->buttonReset->Text = L"Reset";
        this->buttonReset->UseVisualStyleBackColor = true;
        this->numX->DecimalPlaces = 3;
        this->numX->Hexadecimal = false;
        this->numX->Location = System::Drawing::Point(22, 1);
        this->numX->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 100, 0, 0, 0 });
        this->numX->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numX->MinimumSize = System::Drawing::Size(70, 22);
        this->numX->Name = L"numX";
        this->numX->Precision = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numX->Size = System::Drawing::Size(115, 22);
        this->numX->TabIndex = 3;
        this->numX->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
        this->numX->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numY->DecimalPlaces = 3;
        this->numY->Hexadecimal = false;
        this->numY->Location = System::Drawing::Point(22, 22);
        this->numY->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 100, 0, 0, 0 });
        this->numY->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numY->MinimumSize = System::Drawing::Size(70, 22);
        this->numY->Name = L"numY";
        this->numY->Precision = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numY->Size = System::Drawing::Size(115, 22);
        this->numY->TabIndex = 4;
        this->numY->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
        this->numY->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numZ->DecimalPlaces = 3;
        this->numZ->Hexadecimal = false;
        this->numZ->Location = System::Drawing::Point(22, 43);
        this->numZ->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 100, 0, 0, 0 });
        this->numZ->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numZ->MinimumSize = System::Drawing::Size(70, 22);
        this->numZ->Name = L"numZ";
        this->numZ->Precision = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->numZ->Size = System::Drawing::Size(115, 22);
        this->numZ->TabIndex = 5;
        this->numZ->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
        this->numZ->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 0, 0, 0, 0 });
        this->labelX->AutoSize = true;
        this->labelX->Location = System::Drawing::Point(4, 4);
        this->labelX->Name = L"labelX";
        this->labelX->Size = System::Drawing::Size(14, 13);
        this->labelX->TabIndex = 6;
        this->labelX->Text = L"X";
        this->labelY->AutoSize = true;
        this->labelY->Location = System::Drawing::Point(4, 25);
        this->labelY->Name = L"labelY";
        this->labelY->Size = System::Drawing::Size(14, 13);
        this->labelY->TabIndex = 7;
        this->labelY->Text = L"Y";
        this->labelZ->AutoSize = true;
        this->labelZ->Location = System::Drawing::Point(4, 46);
        this->labelZ->Name = L"labelZ";
        this->labelZ->Size = System::Drawing::Size(14, 13);
        this->labelZ->TabIndex = 8;
        this->labelZ->Text = L"Z";
        this->buttonApply->Location = System::Drawing::Point(136, 0);
        this->buttonApply->Name = L"buttonApply";
        this->buttonApply->Size = System::Drawing::Size(50, 22);
        this->buttonApply->TabIndex = 9;
        this->buttonApply->Text = L"Apply";
        this->buttonApply->UseVisualStyleBackColor = true;
        this->buttonImmediate->Location = System::Drawing::Point(21, 63);
        this->buttonImmediate->Name = L"buttonImmediate";
        this->buttonImmediate->Size = System::Drawing::Size(116, 22);
        this->buttonImmediate->TabIndex = 10;
        this->buttonImmediate->Text = L"Immediate";
        this->buttonImmediate->UseVisualStyleBackColor = true;
        this->buttonImmediate->Click += gcnew System::EventHandler(this, &NumericVector::buttonImmediate_Click);
        this->checkImmediate->AutoSize = true;
        this->checkImmediate->Location = System::Drawing::Point(5, 67);
        this->checkImmediate->Name = L"checkImmediate";
        this->checkImmediate->Size = System::Drawing::Size(15, 14);
        this->checkImmediate->TabIndex = 11;
        this->checkImmediate->UseVisualStyleBackColor = true;
        this->AcceptButton = this->buttonOk;
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->CancelButton = this->buttonCancel;
        this->ClientSize = System::Drawing::Size(186, 85);
        this->Controls->Add(this->checkImmediate);
        this->Controls->Add(this->buttonImmediate);
        this->Controls->Add(this->buttonApply);
        this->Controls->Add(this->labelZ);
        this->Controls->Add(this->labelY);
        this->Controls->Add(this->labelX);
        this->Controls->Add(this->numZ);
        this->Controls->Add(this->numY);
        this->Controls->Add(this->numX);
        this->Controls->Add(this->buttonReset);
        this->Controls->Add(this->buttonCancel);
        this->Controls->Add(this->buttonOk);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedToolWindow;
        this->Name = L"NumericVector";
        this->ShowInTaskbar = false;
        this->Text = L"NumericVector";
        this->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &NumericVector::NumericVector_KeyPress);
        this->ResumeLayout(false);
        this->PerformLayout();

    }
#pragma endregion
};
} // namespace Props
} // namespace ECore
} // namespace XRay
