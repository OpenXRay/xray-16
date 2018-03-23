#include "pch.hpp"
#include "WindowIDE.h"
#include "WindowView.h"
#include "WindowLog.h"

namespace XRay::ECore::Props
{
void WindowIDE::Initialize()
{
    windowView = gcnew WindowView();
    windowLog = gcnew WindowLog();

    windowView->Show(editorDock, WeifenLuo::WinFormsUI::Docking::DockState::Document);
    windowLog->Show(editorDock, WeifenLuo::WinFormsUI::Docking::DockState::DockBottomAutoHide);

    auto cb = LogCallback(ELogCallback, windowLog->Handle.ToPointer());
    SetLogCB(cb);
}
} // namespace XRay::ECore::Props
