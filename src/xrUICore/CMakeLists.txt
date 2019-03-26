project(xrUICore)

file(GLOB ARROW arrow/*.h arrow/*.cpp)
file(GLOB BUTTONS Buttons/*.h Buttons/*.cpp)
file(GLOB CALLBACKS Callbacks/*.h Callbacks/*.cpp)
file(GLOB COMBO_BOX ComboBox/*.h ComboBox/*.cpp)
file(GLOB CURSOR Cursor/*.h Cursor/*.cpp)
file(GLOB EDIT_BOX EditBox/*.h EditBox/*.cpp)
file(GLOB FONT_MANAGER FontManager/*.h FontManager/*.cpp)
file(GLOB HINT Hint/*.h Hint/*.cpp)
file(GLOB INTERACTIVE_BACKGROUND InteractiveBackground/*.h InteractiveBackground/*.cpp)
file(GLOB LINES Lines/*.h Lines/*.cpp)
file(GLOB LIST_BOX ListBox/*.h ListBox/*.cpp)
file(GLOB MESSAGE_BOX MessageBox/*.h MessageBox/*.cpp)
file(GLOB OPTIONS Options/*.h Options/*.cpp)
set(PCH pch.cpp pch.hpp)
file(GLOB PROGRESS_BAR ProgressBar/*.h ProgressBar/*.cpp)
file(GLOB PROPERTIES_BOX PropertiesBox/*.h PropertiesBox/*.cpp)
file(GLOB SCROLL_BAR ScrollBar/*.h ScrollBar/*.cpp)
file(GLOB SCROLL_VIEW ScrollView/*.h ScrollView/*.cpp)
file(GLOB SPIN_BOX SpinBox/*.h SpinBox/*.cpp)
file(GLOB STATIC_WINDS Static/*.h Static/*.cpp)
file(GLOB TAB_CONTROL TabControl/*.h TabControl/*.cpp)
file(GLOB TRACK_BAR TrackBar/*.h TrackBar/*.cpp)
file(GLOB WINDOWS Windows/*.h Windows/*.cpp)
file(GLOB XML XML/*.h XML/*.cpp)
set(GENERAL ui_base.cpp ui_base.h ui_defs.h uiabstract.h UIMessages.h)

######################################################

source_group("arrow" FILES ${ARROW})
source_group("Buttons" FILES ${BUTTONS})
source_group("Callbacks" FILES ${CALLBACKS})
source_group("ComboBox" FILES ${COMBO_BOX})
source_group("Cursor" FILES ${CURSOR})
source_group("EditBox" FILES ${EDIT_BOX})
source_group("FontManager" FILES ${FONT_MANAGER})
source_group("Hint" FILES ${HINT})
source_group("InteractiveBackground" FILES ${INTERACTIVE_BACKGROUND})
source_group("Lines" FILES ${LINES})
source_group("ListBox" FILES ${LIST_BOX})
source_group("MessageBox" FILES ${MESSAGE_BOX})
source_group("Options" FILES ${OPTIONS})
source_group("PCH" FILES ${PCH})
source_group("ProgressBar" FILES ${PROGRESS_BAR})
source_group("PropertiesBox" FILES ${PROPERTIES_BOX})
source_group("ScrollBar" FILES ${SCROLL_BAR})
source_group("ScrollView" FILES ${SCROLL_VIEW})
source_group("SpinBox" FILES ${SPIN_BOX})
source_group("Static" FILES ${STATIC_WINDS})
source_group("TabControl" FILES ${TAB_CONTROL})
source_group("TrackBar" FILES ${TRACK_BAR})
source_group("Windows" FILES ${WINDOWS})
source_group("XML" FILES ${XML})
source_group("" FILES ${GENERAL})

##########################################

message("path "  ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DXRUICORE_EXPORTS)

if (WIN32)
    add_compile_options(/fp:fast "/Yupch.hpp")
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/pch.cpp PROPERTIES COMPILE_FLAGS /Yc)
endif()

add_library(xrUICore SHARED ${ARROW} ${BUTTONS} ${CALLBACKS}
                    ${COMBO_BOX} ${CURSOR}
                    ${EDIT_BOX} ${FONT_MANAGER} ${HINT}
                    ${INTERACTIVE_BACKGROUND} ${LINES} ${LIST_BOX}
                    ${MESSAGE_BOX} ${OPTIONS} ${PCH}
                    ${PROGRESS_BAR} ${PROPERTIES_BOX} ${SCROLL_BAR} ${SCROLL_VIEW} ${SPIN_BOX}
                    ${STATIC_WINDS} ${TAB_CONTROL} ${TRACK_BAR} ${WINDOWS} ${XML}
                    ${GENERAL})

set_target_properties(xrUICore PROPERTIES PREFIX "")
target_include_directories(xrUICore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ../../Externals/lzo/include
    ../../Externals/luabind
    ${SDL_INCLUDE_DIRS})
target_link_libraries(xrUICore xrAPI xrScriptEngine ${LUAJIT_LIBRARIES} xrEngine xrMiscMath xrCore luabind ${SDL_LIBRARIES})
xr_install(${PROJECT_NAME})
