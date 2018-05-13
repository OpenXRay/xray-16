#pragma once

namespace XRay
{
namespace Editor
{
namespace Controls
{
using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace WeifenLuo::WinFormsUI::Docking;

public ref class Serializer
{
public:
    static void SerializeDockPanelRoot(Form^ root, DockPanel^ panel, String^ windowName);
    static bool DeserializeDockPanelRoot(Form^ root, DockPanel^ panel, String^ windowName, DeserializeDockContent^ getPanelForSettingCallback);
};
} // namespace XRay::Editor::Controls::Serializer
}
}
