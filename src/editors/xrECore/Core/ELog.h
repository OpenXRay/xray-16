#pragma once

enum class MessageType
{
    Information,  // *
    Warning,      // ~
    Error,        // !
    Confirmation, // #, -
    UserInput,    // @
    Custom
};

ref class EditorLog
{
public:
    bool in_use;

    EditorLog() : in_use(false) {}

public: System::Windows::Forms::DialogResult DlgMsg(MessageType type, System::String^ message)
{
    using namespace System::Windows::Forms;

    in_use = true;

    //ExecCommand(COMMAND_RENDER_FOCUS);

    DialogResult result;

    if (type == MessageType::Confirmation)
    {
        result = MessageBox::Show(message, "Message", MessageBoxButtons::YesNoCancel);
        switch (result)
        {
        case DialogResult::Yes: message->Concat(" - Yes."); break;
        case DialogResult::No: message->Concat(" - No."); break;
        case DialogResult::Cancel: message->Concat(" - Cancel."); break;
        default: message->Concat(" - Something.");
        }
    }
    else
        result = MessageBox::Show(message, "Message", MessageBoxButtons::OK);

    msclr::interop::marshal_context ctx;
    Log(ctx.marshal_as<pcstr>(message));

    in_use = false;

    return result;
}

public: System::Windows::Forms::DialogResult DlgMsg(MessageType type, System::String^ message, System::Windows::Forms::MessageBoxButtons buttons)
{
    using namespace System::Windows::Forms;

    in_use = true;

    //ExecCommand(COMMAND_RENDER_FOCUS);

    DialogResult result = MessageBox::Show(message, "Message", buttons);;
    if (type == MessageType::Confirmation)
    {
        switch (result)
        {
        case DialogResult::Yes: message->Concat(" - Yes."); break;
        case DialogResult::No: message->Concat(" - No."); break;
        case DialogResult::Cancel: message->Concat(" - Cancel."); break;
        default: message->Concat(" - Something.");
        }
    }

    msclr::interop::marshal_context ctx;
    Log(ctx.marshal_as<pcstr>(message));

    in_use = false;

    return result;
}
};

void XRECORE_API ELogCallback(void* context, pcstr message);

extern XRECORE_API gcroot<EditorLog^> ELog;