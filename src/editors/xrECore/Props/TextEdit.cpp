#include "pch.hpp"

#include <msclr/marshal.h>

#include "TextEdit.h"

namespace XRay
{
namespace ECore
{
namespace Props
{
System::Void TextEdit::buttonClear_Click(System::Object^ sender, System::EventArgs^ e)
{
    textBox1->Clear();
}

System::Void TextEdit::textBox1_SelectionChanged(System::Object^ sender, System::EventArgs^ e)
{
    auto line = textBox1->GetLineFromCharIndex(textBox1->SelectionStart);
    auto currline = textBox1->GetFirstCharIndexOfCurrentLine();
    auto charpos = textBox1->SelectionStart - currline;
    toolStripStatusLabel1->Text = (line + 1) + " : " + (charpos + 1);
}

bool TextEdit::Run(xr_string& text, pcstr caption, bool read_only, int lim,
         pcstr apply_name, TOnApplyClick^ on_apply, TOnCloseClick^ on_close,
         TOnCodeInsight^ on_insight)
{
    Text = gcnew String(caption);
    m_text = &text;
    textBox1->ReadOnly = read_only;
    textBox1->Text = gcnew String(text.c_str());
    textBox1->MaxLength = lim;
    
    buttonApply->Text = gcnew String(apply_name);
    //buttonApply->Click += gcnew EventHandler(on_apply);

    return ShowDialog() == Windows::Forms::DialogResult::OK;
}

System::Void TextEdit::buttonOk_Click(System::Object^ sender, System::EventArgs^ e)
{
    msclr::interop::marshal_context ctx;
    *m_text = ctx.marshal_as<pcstr>(textBox1->Text);
}
} // namespace Props
} // namespace ECore
} // namespace XRay
