#include "pch.hpp"
#include "ShaderFunction.h"
#include "xrEngine/WaveForm.h"
#include "xrCore/xr_token.h"
#include "Token.h"

xr_token function_token[] =
{
    { "Constant", WaveForm::fCONSTANT },
    { "Sin", WaveForm::fSIN },
    { "Triangle", WaveForm::fTRIANGLE },
    { "Square", WaveForm::fSQUARE },
    { "Saw-Tooth", WaveForm::fSAWTOOTH },
    { "Inv Saw-Tooth", WaveForm::fINVSAWTOOTH },
    { nullptr, 0 }
};

namespace XRay
{
namespace ECore
{
namespace Props
{
ShaderFunction::ShaderFunction(void)
{
    InitializeComponent();
    FillFunctionsFromToken(function_token);
}

bool ShaderFunction::Run(WaveForm* func)
{
    currentFunc = func;
    saveFunc = new WaveForm(*func);
    GetFuncData();
    UpdateFuncData();
    auto result = ShowDialog();
    delete saveFunc;
    return result == Windows::Forms::DialogResult::OK;
}

void ShaderFunction::FillFunctionsFromToken(const xr_token* tokens)
{
    comboFunctions->Items->Clear();
    for (int i = 0; tokens[i].name; i++)
    {
        comboFunctions->Items->Add(gcnew XRay::Token(tokens[i].id, tokens[i].name));
    }
}

XRay::Token^ ShaderFunction::GetTokenFromValue(int val)
{
    for each (XRay::Token^ token in comboFunctions->Items)
    {
        if (token->ToInt32() == val)
            return token;
    }

    NODEFAULT;
}

System::Void ShaderFunction::ShaderFunction_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e)
{
    switch (e->KeyChar)
    {
    case (Char)Keys::Escape:
        buttonCancel_Click(sender, e);
        break;
    }
}

System::Void ShaderFunction::buttonCancel_Click(System::Object^ sender, System::EventArgs^ e)
{
    CopyMemory(currentFunc, saveFunc, sizeof(WaveForm));
    this->Close();
}

System::Void ShaderFunction::numArgX_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e)
{
    switch (e->KeyChar)
    {
    case (Char)Keys::Enter:
        UpdateFuncData();
        break;
    }
}

System::Void ShaderFunction::buttonOk_Click(System::Object^ sender, System::EventArgs^ e)
{
    UpdateFuncData();
}

void ShaderFunction::GetFuncData()
{
    bLoadMode = true;
    comboFunctions->SelectedValue = GetTokenFromValue(currentFunc->F); // XXX: may not work
    numArg1->Value = (Decimal)currentFunc->arg[0];
    numArg2->Value = (Decimal)currentFunc->arg[1];
    numArg3->Value = (Decimal)currentFunc->arg[2];
    numArg4->Value = (Decimal)currentFunc->arg[3];
    bLoadMode = false;
}

void ShaderFunction::UpdateFuncData()
{
    if (bLoadMode)
        return;
    currentFunc->F = (WaveForm::EFunction)((XRay::Token^)comboFunctions->SelectedValue)->ToInt32();
    currentFunc->arg[0] = (float)numArg1->Value;
    currentFunc->arg[1] = (float)numArg2->Value;
    currentFunc->arg[2] = (float)numArg3->Value;
    currentFunc->arg[3] = (float)numArg4->Value;

    labelMax->Text = (currentFunc->arg[1] + currentFunc->arg[0]).ToString();
    labelMin->Text = (-currentFunc->arg[1] + currentFunc->arg[0]).ToString();
    labelCenter->Text = (currentFunc->arg[0]).ToString();

    float v = (float)numScale->Value * 1000 / currentFunc->arg[3];
    string16 buf;
    if (v <= 1000)
        xr_sprintf(buf, "%4.0f ms", v);
    else
        xr_sprintf(buf, "%.2f s", v / 1000);
    labelEnd->Text = gcnew System::String(buf);

    DrawGraph();
}

void ShaderFunction::DrawGraph()
{
    auto w = pbDraw->Width - 4;
    auto h = pbDraw->Height - 4;

    // XXX: Draw
    //System::Windows::Controls::Canvas canvas;
}
} // namespace Props
} // namespace ECore
} // namespace XRay
