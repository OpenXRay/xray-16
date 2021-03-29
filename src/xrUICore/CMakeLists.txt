project(xrUICore)

set(SRC_FILES
    "pch.cpp"
    "pch.hpp"
    "ui_base.cpp"
    "ui_base.h"
    "ui_defs.h"
    "uiabstract.h"
    "UIMessages.h"
    "arrow/ui_arrow.cpp"
    "arrow/ui_arrow.h"
    "Buttons/UI3tButton.cpp"
    "Buttons/UI3tButton.h"
    "Buttons/UIBtnHint.cpp"
    "Buttons/UIBtnHint.h"
    "Buttons/UIButton.cpp"
    "Buttons/UIButton.h"
    "Buttons/UIButton_script.cpp"
    "Buttons/UICheckButton.cpp"
    "Buttons/UICheckButton.h"
    "Buttons/UIRadioButton.cpp"
    "Buttons/UIRadioButton.h"
    "Callbacks/UIWndCallback.cpp"
    "Callbacks/UIWndCallback.h"
    "Callbacks/callback_info.h"
    "ComboBox/UIComboBox.cpp"
    "ComboBox/UIComboBox.h"
    "ComboBox/UIComboBox_script.cpp"
    "Cursor/UICursor.cpp"
    "Cursor/UICursor.h"
    "EditBox/UICustomEdit.cpp"
    "EditBox/UICustomEdit.h"
    "EditBox/UIEditBox.cpp"
    "EditBox/UIEditBox.h"
    "EditBox/UIEditBoxEx.cpp"
    "EditBox/UIEditBoxEx.h"
    "EditBox/UIEditBox_script.cpp"
    "FontManager/FontManager.cpp"
    "FontManager/FontManager.h"
    "Hint/UIHint.cpp"
    "Hint/UIHint.h"
    "InteractiveBackground/UIInteractiveBackground.h"
    "InteractiveBackground/UI_IB_Static.cpp"
    "InteractiveBackground/UI_IB_Static.h"
    "Lines/UILine.cpp"
    "Lines/UILine.h"
    "Lines/UILines.cpp"
    "Lines/UILines.h"
    "Lines/UISubLine.cpp"
    "Lines/UISubLine.h"
    "Lines/uilinestd.h"
    "ListBox/UIListBox.cpp"
    "ListBox/UIListBox.h"
    "ListBox/UIListBoxItem.cpp"
    "ListBox/UIListBoxItem.h"
    "ListBox/UIListBoxItemMsgChain.cpp"
    "ListBox/UIListBoxItemMsgChain.h"
    "ListBox/UIListBox_script.cpp"
    "ListWnd/UIListItem.cpp"
    "ListWnd/UIListItem.h"
    "ListWnd/UIListItemEx.cpp"
    "ListWnd/UIListItemEx.h"
    "ListWnd/UIListWnd.cpp"
    "ListWnd/UIListWnd.h"
    "ListWnd/UIListWnd_inline.h"
    "ListWnd/UIListWnd_script.cpp"
    "MessageBox/UIMessageBox.cpp"
    "MessageBox/UIMessageBox.h"
    "MessageBox/UIMessageBox_script.cpp"
    "Options/UIOptionsItem.cpp"
    "Options/UIOptionsItem.h"
    "Options/UIOptionsManager.cpp"
    "Options/UIOptionsManager.h"
    "Options/UIOptionsManagerScript.cpp"
    "Options/UIOptionsManagerScript.h"
    "ProgressBar/UIDoubleProgressBar.cpp"
    "ProgressBar/UIDoubleProgressBar.h"
    "ProgressBar/UIProgressBar.cpp"
    "ProgressBar/UIProgressBar.h"
    "ProgressBar/UIProgressBar_script.cpp"
    "ProgressBar/UIProgressShape.cpp"
    "ProgressBar/UIProgressShape.h"
    "PropertiesBox/UIPropertiesBox.cpp"
    "PropertiesBox/UIPropertiesBox.h"
    "PropertiesBox/UIPropertiesBox_script.cpp"
    "ScrollBar/UIFixedScrollBar.cpp"
    "ScrollBar/UIFixedScrollBar.h"
    "ScrollBar/UIScrollBar.cpp"
    "ScrollBar/UIScrollBar.h"
    "ScrollBar/UIScrollBox.cpp"
    "ScrollBar/UIScrollBox.h"
    "ScrollView/UIScrollView.cpp"
    "ScrollView/UIScrollView.h"
    "SpinBox/UICustomSpin.cpp"
    "SpinBox/UICustomSpin.h"
    "SpinBox/UISpinNum.cpp"
    "SpinBox/UISpinNum.h"
    "SpinBox/UISpinText.cpp"
    "SpinBox/UISpinText.h"
    "Static/UIAnimatedStatic.cpp"
    "Static/UIAnimatedStatic.h"
    "Static/UILanimController.cpp"
    "Static/UILanimController.h"
    "Static/UIStatic.cpp"
    "Static/UIStatic.h"
    "Static/UIStaticItem.cpp"
    "Static/UIStaticItem.h"
    "Static/UIStatic_script.cpp"
    "TabControl/UITabButton.cpp"
    "TabControl/UITabButton.h"
    "TabControl/UITabControl.cpp"
    "TabControl/UITabControl.h"
    "TabControl/UITabControl_script.cpp"
    "TrackBar/UITrackBar.cpp"
    "TrackBar/UITrackBar.h"
    "Windows/UIFrameLineWnd.cpp"
    "Windows/UIFrameLineWnd.h"
    "Windows/UIFrameWindow.cpp"
    "Windows/UIFrameWindow.h"
    "Windows/UITextFrameLineWnd.cpp"
    "Windows/UITextFrameLineWnd.h"
    "Windows/UIWindow.cpp"
    "Windows/UIWindow.h"
    "Windows/UIWindow_script.cpp"
    "XML/UITextureMaster.cpp"
    "XML/UITextureMaster.h"
    "XML/UIXmlInitBase.cpp"
    "XML/UIXmlInitBase.h"
    "XML/xrUIXmlParser.cpp"
    "XML/xrUIXmlParser.h"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Externals/lzo/include
    ${CMAKE_SOURCE_DIR}/Externals/luabind
    ${SDL_INCLUDE_DIRS}
)

target_link_libraries(xrUICore
    PRIVATE
    xrAPI
    xrScriptEngine
    xrEngine
    xrMiscMath
    xrCore
    xrLuabind
    ${SDL_LIBRARIES}
)

if (WIN32)
    add_compile_options(/fp:fast "/Yupch.hpp")
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/pch.cpp PROPERTIES COMPILE_FLAGS /Yc)
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRUICORE_EXPORTS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "pch.hpp"
)

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
