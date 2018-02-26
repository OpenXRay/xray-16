#include "pch.hpp"
#include "NumericVector.h"

namespace XRay
{
namespace ECore
{
namespace Props
{
bool NumericVector::Run(pcstr title, Fvector* data, int decimal, Fvector* resetValue, Fvector* min, Fvector* max, int* X, int* Y)
{
    System::String^ str = gcnew System::String(title);
    this->Text = str;
    Value = data;
    ResetValue = resetValue;
    buttonReset->Enabled = !!resetValue;

    numX->DecimalPlaces = decimal;
    numY->DecimalPlaces = decimal;
    numZ->DecimalPlaces = decimal;

    numX->Value = (Decimal)data->x;
    numY->Value = (Decimal)data->y;
    numZ->Value = (Decimal)data->z;

    if (min)
    {
        numX->Minimum = (Decimal)min->x;
        numY->Minimum = (Decimal)min->y;
        numZ->Minimum = (Decimal)min->z;
    }
    else
    {
        numX->Minimum = Decimal::MinValue;
        numY->Minimum = Decimal::MinValue;
        numZ->Minimum = Decimal::MinValue;

    }

    if (max)
    {
        numX->Maximum = (Decimal)max->x;
        numY->Maximum = (Decimal)max->y;
        numZ->Maximum = (Decimal)max->z;
    }
    else
    {
        numX->Maximum = Decimal::MaxValue;
        numY->Maximum = Decimal::MaxValue;
        numZ->Maximum = Decimal::MaxValue;

    }

    if (!X || !Y)
    {
        POINT pt;
        GetCursorPos(&pt);
        int w = GetSystemMetrics(SM_CXSCREEN);
        int h = GetSystemMetrics(SM_CYSCREEN);
        Left = pt.x - (Width * 0.5f);
        Top = pt.y;
        if (((Left + Width * 0.5f) > w) || ((Top + Height) > h))
        {
            Left = w * 0.5f - Width * 0.5f;
            Top = h * 0.5f;
        }
    }
    else
    {
        Left = *X - (Width * 0.5f);
        Top = *Y;
    }

    numX->ValueChanged += gcnew EventHandler(this, &NumericVector::OnValueChanged);

    return ShowDialog() == Windows::Forms::DialogResult::OK;
}

System::Void NumericVector::buttonApply_Click(System::Object^ sender, System::EventArgs^ e)
{
    Value->set((float)numX->Value, (float)numY->Value, (float)numZ->Value);
}

System::Void NumericVector::buttonOk_Click(System::Object^ sender, System::EventArgs^ e)
{
    buttonApply_Click(sender, e);
}

System::Void NumericVector::buttonReset_Click(System::Object^ sender, System::EventArgs^ e)
{
    numX->Value = (Decimal)ResetValue->x;
    numY->Value = (Decimal)ResetValue->y;
    numZ->Value = (Decimal)ResetValue->z;
}

System::Void NumericVector::buttonImmediate_Click(System::Object^ sender, System::EventArgs^ e)
{
    checkImmediate->Checked = !checkImmediate->Checked;
}

System::Void NumericVector::NumericVector_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e)
{
    switch (e->KeyChar)
    {
    case (Char)Keys::Space:
        buttonApply->PerformClick();
        break;
    case (Char)Keys::Enter:
        buttonOk->PerformClick();
        break;
    case (Char)Keys::Escape:
        buttonCancel->PerformClick();
        break;
    }
}

System::Void NumericVector::OnValueChanged(System::Object^ sender, System::EventArgs^ e)
{
    if (checkImmediate->Checked)
        buttonApply_Click(sender, e);
}
} // namespace Props
} // namespace ECore
} // namespace XRay
