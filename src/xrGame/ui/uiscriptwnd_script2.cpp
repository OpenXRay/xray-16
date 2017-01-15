#include "pch_script.h"

// UI-controls

#include "UIButton.h"
#include "UICheckButton.h"
#include "UIEditBox.h"
#include "UIFrameLineWnd.h"
#include "UIFrameWindow.h"
#include "UIMessageBox.h"
#include "UIProgressBar.h"
#include "UIPropertiesBox.h"
#include "UIRadioButton.h"
#include "UIScriptWnd.h"
#include "UIStatic.h"
#include "UITabControl.h"

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
