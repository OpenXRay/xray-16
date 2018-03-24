#include "pch.hpp"
#include "WindowLog.h"
#include "Core/ELog.h"

namespace XRay::Editor::Windows
{
void WindowLog::AddDialogMessage(MessageType type, System::String^ message)
{
    //ExecCommand(COMMAND_RENDER_FOCUS);
    MessageBox::Show(message, "Message", MessageBoxButtons::OK);
    AddMessage(type, message);
}

void WindowLog::AddMessage(MessageType type, System::String^ message)
{
    auto newMessage = gcnew ListViewItem(message);
    newMessage->ToolTipText = message;

    switch (type)
    {
    case MessageType::Information:
        newMessage->BackColor = Color::LightGray;
        break;

    case MessageType::Warning:
        newMessage->BackColor = Color::LightGoldenrodYellow;
        break;

    case MessageType::Error:
        newMessage->BackColor = Color::Crimson;
        break;

    case MessageType::Confirmation:
        newMessage->BackColor = Color::LightGreen;
        break;

    case MessageType::UserInput:
        newMessage->BackColor = Color::LightBlue;
        break;
    }

    logList->Items->Add(newMessage);
    newMessage->EnsureVisible();

    if (type == MessageType::Error)
        Focus();
}

System::Void WindowLog::buttonClose_Click(System::Object^ sender, System::EventArgs^ e)
{
    Hide();
}

System::Void WindowLog::buttonFlush_Click(System::Object^ sender, System::EventArgs^ e)
{
    FlushLog();
}

System::Void WindowLog::buttonClear_Click(System::Object^ sender, System::EventArgs^ e)
{
    logList->Items->Clear();
}

System::Void WindowLog::buttonClearSelected_Click(System::Object^ sender, System::EventArgs^ e)
{
    for each (ListViewItem^ item in logList->SelectedItems)
        item->Remove();
}

System::Void WindowLog::logList_Resize(System::Object^ sender, System::EventArgs^ e)
{
    columnHeader1->AutoResize(ColumnHeaderAutoResizeStyle::HeaderSize);
}

System::Void WindowLog::LogForm_Closing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e)
{
    e->Cancel = true;
    buttonClose->PerformClick();
}

System::Void WindowLog::LogForm_KeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e)
{
    switch (e->KeyCode)
    {
    case Keys::Escape:
        Close();
        break;

    default:
        // XXX: enable in future
        // UI->ApplyGlobalShortCut(Key, Shift);
        break;
    }
}
} // namespace XRay::Editor::Windows
