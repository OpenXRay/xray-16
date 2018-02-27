#include "pch.hpp"
#include "TextEdit.h"

System::Void XRay::ECore::Props::TextEdit::buttonClear_Click(System::Object^ sender, System::EventArgs^ e)
{
    textBox1->Clear();
}

System::Void XRay::ECore::Props::TextEdit::buttonOk_Click(System::Object^ sender, System::EventArgs^ e)
{
    //marshal this
    //*m_text = textBox1->Text;
}
