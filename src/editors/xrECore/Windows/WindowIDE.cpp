#include "pch.hpp"
#include "WindowIDE.h"
#include "WindowView.h"
#include "WindowLog.h"
#include "Controls/DockPanelSerializer.h"

namespace XRay::Editor::Windows
{
void WindowIDE::Initialize()
{
    windowView = gcnew WindowView();
    windowLog = gcnew WindowLog();

    if (!Editor::Controls::Serializer::DeserializeDockPanelRoot(
        this, editorDock, this->Text,
        gcnew WeifenLuo::WinFormsUI::Docking::DeserializeDockContent(this, &WindowIDE::reloadContent)))
    {
        windowView->Show(editorDock, WeifenLuo::WinFormsUI::Docking::DockState::Document);
        windowLog->Show(editorDock, WeifenLuo::WinFormsUI::Docking::DockState::DockBottomAutoHide);
    }

    auto cb = LogCallback(ELogCallback, windowLog->Handle.ToPointer());
    SetLogCB(cb);
}

System::Void WindowIDE::WindowIDE_FormClosing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e)
{
    Editor::Controls::Serializer::SerializeDockPanelRoot(this, editorDock, this->Text);
}

WeifenLuo::WinFormsUI::Docking::IDockContent^ WindowIDE::reloadContent(System::String^ persistString)
{
    WeifenLuo::WinFormsUI::Docking::IDockContent^ result = nullptr;

    if (persistString == windowView->GetType()->ToString())
        result = windowView;

    if (persistString == windowLog->GetType()->ToString())
        result = windowLog;

    return result;
}
} // namespace XRay::Editor::Windows
