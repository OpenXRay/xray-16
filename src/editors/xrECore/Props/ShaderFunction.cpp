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
    return nullptr;
}

System::Void ShaderFunction::ShaderFunction_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e)
{
    switch (e->KeyChar)
    {
    case (Char)Keys::Enter:
        buttonOk->PerformClick();
        break;
    case (Char)Keys::Escape:
        buttonCancel->PerformClick();
        break;
    }
}

System::Void ShaderFunction::buttonCancel_Click(System::Object^ sender, System::EventArgs^ e)
{
    CopyMemory(currentFunc, saveFunc, sizeof(WaveForm));
}

System::Void ShaderFunction::onAnyValueChanged(System::Object^ sender, System::EventArgs^ e)
{
    UpdateFuncData();
}

void ShaderFunction::GetFuncData()
{
    bLoadMode = true;
    comboFunctions->SelectedItem = GetTokenFromValue(currentFunc->F);
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
    currentFunc->F = (WaveForm::EFunction)((XRay::Token^)comboFunctions->SelectedItem)->ToInt32();
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
    auto w = pbDraw->Width;
    auto h = pbDraw->Height;

    pbDraw->Image = gcnew Bitmap(w, h);
    auto graphics = Graphics::FromImage(pbDraw->Image);
    auto pen = gcnew Pen(Brushes::DarkGreen);

    // Fill with black
    graphics->FillRectangle(Brushes::Black, 0, 0, w, h);

    w -= 4;
    h -= 4;

    // Draw center line
    {
        pen->DashStyle = Drawing::Drawing2D::DashStyle::Dot;
        graphics->DrawLine(pen, Point(2, h / 2 + 2), Point(w + 1, h / 2 + 2));
    }

    // Draw borders
    {
        pen->DashStyle = Drawing::Drawing2D::DashStyle::Solid;
        graphics->DrawRectangle(pen, 0, 0, w + 3, h + 3);
    }

    // Draw graph
    {
        pen->Brush = Brushes::Yellow;

        float t_cost = 1.f / w;
        float tm = 0;
        float y = currentFunc->Calculate(tm) - currentFunc->arg[0];
        float delta = currentFunc->arg[1] * 2;
        delta = delta ? (h / delta) : 0;
        float yy = h - (delta * y + h / 2);


        Drawing::Drawing2D::GraphicsPath path;
        auto pointFrom = Point(2, yy + 2);
        auto pointTo = Point(0, 0);

        for (int t = 1; t < w; ++t)
        {
            tm = (float)numScale->Value * t * t_cost / (fis_zero(currentFunc->arg[3]) ? 1.f : currentFunc->arg[3]);
            y = currentFunc->Calculate(tm) - currentFunc->arg[0];
            yy = h - (delta * y + h / 2);
            pointTo = Point(t + 2, yy + 2);
            path.AddLine(pointFrom, pointTo);
            pointFrom = pointTo;
        }
        graphics->DrawPath(pen, %path);

        // Draw X-axis
        pen->Brush = Brushes::Green;
        float AxisX = h - (delta * (-currentFunc->arg[0]) + h / 2);
        graphics->DrawLine(pen, Point(2, AxisX + 2), Point(w + 1, AxisX + 2));
    }

    //pbDraw->Invalidate(); // Redraw everything just in case
}
} // namespace Props
} // namespace ECore
} // namespace XRay
