#include "pch_script.h"

// UI-controls

#include "UIScriptWnd.h"
#include "xrUICore/Buttons/UIButton.h"
#include "xrUICore/MessageBox/UIMessageBox.h"
#include "xrUICore/PropertiesBox/UIPropertiesBox.h"
#include "xrUICore/Buttons/UICheckButton.h"
#include "xrUICore/Buttons/UIRadioButton.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/EditBox/UIEditBox.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/TabControl/UITabControl.h"

#include "uiscriptwnd_script.h"

using namespace luabind;

#pragma optimize("s", on)
export_class& script_register_ui_window2(export_class& instance)
{
    instance.def("OnKeyboard", &BaseType::OnKeyboardAction, &WrapType::OnKeyboard_static)
        .def("Update", &BaseType::Update, &WrapType::Update_static)
        .def("Dispatch", &BaseType::Dispatch, &WrapType::Dispatch_static)

        ;
    return (instance);
}
