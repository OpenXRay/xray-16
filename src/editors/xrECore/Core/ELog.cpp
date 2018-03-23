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
    MessageType type = MessageType::Information;

    switch (message[0])
    {
    case '!':
        type = MessageType::Error;
        message++;
        break;

    case '#':
        isDialog = true;
        type = MessageType::Confirmation;
        message++;
        break;
    }

    auto windowLog = safe_cast<WindowLog^>(Form::FromHandle(IntPtr(context)));

    if (windowLog)
    {
        if (isDialog)
        {
            auto d = gcnew WindowLog::AddMessageDelegate(windowLog, &WindowLog::AddDialogMessage);
            windowLog->Invoke(d, gcnew array<Object^> { (int)type, gcnew String(message) });
        }
        else
        {
            auto d = gcnew WindowLog::AddMessageDelegate(windowLog, &WindowLog::AddMessage);
            windowLog->Invoke(d, gcnew array<Object^> { (int)type, gcnew String(message) });
        }
    }
}