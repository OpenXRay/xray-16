#include "pch.hpp"
#include "TextEdit.h"

namespace XRay::Editor::Controls
{
System::Void TextEdit::buttonClear_Click(System::Object^ sender, System::EventArgs^ e)
{
    textBox1->Clear();
}

System::Void TextEdit::textBox1_SelectionChanged(System::Object^ sender, System::EventArgs^ e)
{
    buttonOk->Enabled = textBox1->Modified;

    auto line = textBox1->GetLineFromCharIndex(textBox1->SelectionStart);
    auto currline = textBox1->GetFirstCharIndexOfCurrentLine();
    auto charpos = textBox1->SelectionStart - currline;
    toolStripStatusLabel1->Text = (line + 1) + " : " + (charpos + 1);
}

System::Void TextEdit::buttonApply_Click(System::Object^ sender, System::EventArgs^ e)
{
    if (!onApplyClick->empty())
    {
        msclr::interop::marshal_context ctx;
        if ((*onApplyClick)(ctx.marshal_as<pcstr>(textBox1->Text)))
        {
            *m_text = ctx.marshal_as<pcstr>(textBox1->Text);
        }
    }
    else
    {
        msclr::interop::marshal_context ctx;
        *m_text = ctx.marshal_as<pcstr>(textBox1->Text);
    }
    textBox1->Modified = false;
    buttonOk->Enabled = false;
    toolStripStatusLabel3->Text = "Successful save";
}

System::Void TextEdit::textBox1_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e)
{
    if (!onCodeInsight->empty() && e->KeyChar == (Char)Keys::Space
        && textBox1->ModifierKeys == (Keys::Control & Keys::Shift))
    {
        xr_string src_line, hint;
        src_line = textBox1->GetLineFromCharIndex(textBox1->SelectionStart);
        bool result = true;
        (*onCodeInsight)(src_line, hint, result);
        if (result)
        {
            toolStripStatusLabel3->Text = gcnew String((xr_string("Hint: ") + hint).c_str());
            toolStripStatusLabel3->ToolTipText = gcnew String(hint.c_str());
        }
        else
        {
            toolStripStatusLabel3->Text = gcnew String((xr_string("Error: ") + hint).c_str());
            toolStripStatusLabel3->ToolTipText = gcnew String(hint.c_str());
        }
    }
    else if (e->KeyChar == (Char)Keys::Enter && textBox1->ModifierKeys == Keys::Control)
    {
        buttonOk->PerformClick();
    }
}

TextEdit::TextEdit(xr_string& text, pcstr caption, bool read_only, int lim, pcstr apply_name, TOnApplyClick on_apply, TOnCloseClick on_close, TOnCodeInsight on_insight)
{
    Text = gcnew String(caption);
    m_text = &text;
    textBox1->ReadOnly = read_only;
    textBox1->Text = gcnew String(text.c_str());
    textBox1->MaxLength = lim;

    buttonApply->Text = gcnew String(apply_name);

    onApplyClick = &on_apply;
    onCloseClick = &on_close;
    onCodeInsight = &on_insight;

    textBox1_SelectionChanged(nullptr, nullptr);
    Show();
}

bool TextEdit::Run(xr_string& text, pcstr caption, bool read_only, int lim,
         pcstr apply_name, TOnApplyClick on_apply, TOnCloseClick on_close,
         TOnCodeInsight on_insight)
{
    Text = gcnew String(caption);
    m_text = &text;
    textBox1->ReadOnly = read_only;
    textBox1->Text = gcnew String(text.c_str());
    textBox1->MaxLength = lim;
    
    buttonApply->Text = gcnew String(apply_name);

    onApplyClick = &on_apply;
    onCloseClick = &on_close;
    onCodeInsight = &on_insight;

    textBox1_SelectionChanged(nullptr, nullptr);
    return ShowDialog() == Windows::Forms::DialogResult::OK;
}

System::Void TextEdit::buttonOk_Click(System::Object^ sender, System::EventArgs^ e)
{
    msclr::interop::marshal_context ctx;
    *m_text = ctx.marshal_as<pcstr>(textBox1->Text);
}

System::Void TextEdit::buttonLoad_Click(System::Object^ sender, System::EventArgs^ e)
{
    xr_string fn;
    if (EFS.GetOpenName(_import_, fn, false, NULL, 2))
    {
        xr_string buf;
        IReader* F = FS.r_open(fn.c_str());
        F->r_stringZ(buf);
        textBox1->Text = gcnew String(buf.c_str());
        FS.r_close(F);
        buttonOk->Enabled = true;
    }
    else
        toolStripStatusLabel3->Text = "Error: can't load file";
}

System::Void TextEdit::buttonSave_Click(System::Object^ sender, System::EventArgs^ e)
{
    xr_string fn;
    if (EFS.GetSaveName(_import_, fn, NULL, 2))
    {
        CMemoryWriter F;
        msclr::interop::marshal_context ctx;
        F.w_stringZ(ctx.marshal_as<pcstr>(textBox1->Text));
        if (!F.save_to(fn.c_str()))
            Log("!Can't save text file:", fn.c_str());
    }
    else
        toolStripStatusLabel3->Text = "Error: can't save file";

}
} // namespace XRay::Editor::Controls
