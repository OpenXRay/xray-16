#include "pch.hpp"
#include "ELog.h"
#include "Windows/WindowLog.h"

gcroot<EditorLog^> ELog;

using namespace XRay::Editor::Windows;

XRECORE_API void ELogCallback(void* context, pcstr message)
{
    if (0 == message[0])
        return;

    bool isDialog = false;

    MessageType type;
    switch (message[0])
    {
    case '*':
        type = MessageType::Information;
        message++;
        break;

    case '~':
        type = MessageType::Warning;
        message++;
        break;

    case '!':
        type = MessageType::Error;
        message++;
        break;

    case '#':
        isDialog = true;
        type = MessageType::Confirmation;
        message++;
        break;

    case '@':
        type = MessageType::UserInput;
        message++;
        break;

    case '-':
        type = MessageType::Confirmation;
        message++;
        break;

    default:
        type = MessageType::Custom;
        break;
    }

    auto windowLog = safe_cast<WindowLog^>(Form::FromHandle(IntPtr(context)));

    if (windowLog)
    {

        if (isDialog)
        {
            auto d = gcnew WindowLog::AddMessageDelegate(windowLog, &WindowLog::AddDialogMessage);
            if (windowLog->InvokeRequired)
                windowLog->BeginInvoke(d, gcnew array<Object^> { (int)type, gcnew String(message) });
            else
                d(type, gcnew String(message));;
        }
        else
        {
            auto d = gcnew WindowLog::AddMessageDelegate(windowLog, &WindowLog::AddMessage);
            if (windowLog->InvokeRequired)
                windowLog->BeginInvoke(d, gcnew array<Object^> { (int)type, gcnew String(message) });
            else
                d(type, gcnew String(message));
        }
    }
}