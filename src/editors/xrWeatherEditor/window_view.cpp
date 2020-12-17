#include "pch.hpp"
#include "engine_include.hpp"
#include "window_ide.h"
#include "window_view.h"
#include "ide_impl.hpp"
#include "property_container.hpp"
#include "property_color_base.hpp"

#include "resource.h"

#pragma comment(lib, "gdi32.lib")

using editor::window_ide;
using editor::window_view;
using XRay::SdkControls::IIncrementable;

using System::Windows::Forms::GridItem;
using Flobbster::Windows::Forms::PropertyBag;
typedef PropertyBag::PropertySpecDescriptor PropertySpecDescriptor;

void window_view::custom_init(window_ide % ide)
{
    SuspendLayout();

    m_ide = % ide;
    m_engine = &ide.engine();
    m_loaded = false;

    ResumeLayout();
}

void window_view::on_load_finished()
{
    if (m_loaded)
        return;

    m_loaded = true;
    EditButton_Click(this, nullptr);
    PauseButton_Click(this, nullptr);
}

void window_view::pause()
{
    if (EditButton->Checked)
        return;

    EditButton_Click(this, nullptr);
}

System::IntPtr window_view::draw_handle() { return (ViewPanel->Handle); }
void window_view::reclip_cursor()
{
    if (EditButton->Checked)
        return;

    //if (!m_loaded)
    //    return;

    while (ShowCursor(FALSE) >= 0)
        ;

    using System::Drawing::Rectangle;
    Rectangle view = RectangleToScreen(ClientRectangle);
    RECT rectangle;
    rectangle.left = view.Left;
    rectangle.top = view.Top + MainToolBar->Height;
    rectangle.right = view.Right;
    rectangle.bottom = view.Bottom;
    ClipCursor(&rectangle);
}

System::Void window_view::EditButton_Click(System::Object ^ sender, System::EventArgs ^ e)
{
    EditButton->Checked = !EditButton->Checked;
    PauseButton->Enabled = EditButton->Checked;

    if (EditButton->Checked)
    {
        m_engine->pause(PauseButton->Checked);
        m_engine->capture_input(true);
        ClipCursor(nullptr);
        while (ShowCursor(TRUE) <= 0)
            ;
    }
    else
    {
        m_engine->pause(false);
        m_engine->capture_input(false);
        reclip_cursor();
    }
}

System::Void window_view::PauseButton_Click(System::Object ^ sender, System::EventArgs ^ e)
{
    if (!PauseButton->Enabled)
        return;
    PauseButton->Checked = !PauseButton->Checked;
    m_engine->pause(PauseButton->Checked);
}

System::Void window_view::window_view_DoubleClick(System::Object ^ sender, System::EventArgs ^ e)
{
    if (!EditButton->Checked)
        return;

    EditButton_Click(this, nullptr);
}

System::Void window_view::window_view_SizeChanged(System::Object ^ sender, System::EventArgs ^ e)
{
    if (!components)
        return;

    m_engine->on_resize();
    reclip_cursor();
}

System::Void window_view::window_view_LocationChanged(System::Object ^ sender, System::EventArgs ^ e)
{
    if (!Parent)
        return;

    if (!m_loaded)
        return;

    reclip_cursor();
}

System::Void window_view::window_view_Activated(System::Object ^ sender, System::EventArgs ^ e)
{
    if (!Parent)
        return;

    while (ShowCursor(TRUE) <= 0)
        ;

    //if (!m_loaded)
    //    return;

    reclip_cursor();
}

System::Void window_view::window_view_Deactivate(System::Object ^ sender, System::EventArgs ^ e)
{
    if (!Parent)
        return;

    //if (!m_loaded)
    //    return;

    ClipCursor(nullptr);
    while (ShowCursor(TRUE) <= 0)
        ;

    if (!EditButton->Checked)
        EditButton_Click(this, nullptr);
}

System::Void window_view::window_view_KeyUp(System::Object ^ sender, System::Windows::Forms::KeyEventArgs ^ e)
{
    if (!e->Alt)
        return;

    if (e->KeyCode == System::Windows::Forms::Keys::Return)
    {
        if (!EditButton->Checked)
            EditButton_Click(this, nullptr);

        return;
    }
}

System::Void window_view::window_view_Paint(System::Object ^ sender, System::Windows::Forms::PaintEventArgs ^ e)
{
    if (dynamic_cast<XRay::Editor::ide_impl&>(m_ide->ide()).idle())
        return;

    m_engine->on_idle();
}

void window_view::on_idle() { check_cursor(); }
void window_view::property_grid(PropertyGrid ^ property_grid) { m_property_grid = property_grid; }
System::Void window_view::ViewPanel_MouseDown(System::Object ^ sender, System::Windows::Forms::MouseEventArgs ^ e)
{
    if (e->Button != System::Windows::Forms::MouseButtons::Middle)
        return;

    m_previous_location = e->Location;
}

System::Void window_view::ViewPanel_MouseMove(System::Object ^ sender, System::Windows::Forms::MouseEventArgs ^ e)
{
    if (e->Location.X == m_previous_location.X)
        return;

    if (e->Button != System::Windows::Forms::MouseButtons::Middle)
        return;

    if (!m_property_grid)
        return;

    m_property_grid->Refresh();

    Object ^ selected_object = m_property_grid->SelectedObject;
    if (!selected_object)
        return;

    GridItem ^ item = m_property_grid->SelectedGridItem;
    if (!item)
        return;

    PropertyDescriptor ^ descriptor_raw = item->PropertyDescriptor;
    if (!descriptor_raw)
        return;

    PropertySpecDescriptor ^ descriptor = safe_cast<PropertySpecDescriptor ^>(descriptor_raw);
    property_container ^ container = safe_cast<property_container ^>(descriptor->bag);
    ::IProperty ^ raw_value = container->GetProperty(descriptor->item);
    VERIFY(raw_value);

    IIncrementable ^ incrementable = dynamic_cast<IIncrementable ^>(raw_value);
    if (!incrementable)
        return;

    incrementable->Increment(float(e->Location.X - m_previous_location.X));
    m_previous_location = e->Location;
}

System::Void window_view::ViewPanel_MouseLeave(System::Object ^ sender, System::EventArgs ^ e)
{
    //	m_property_grid				= nullptr;
}

System::Void window_view::ViewPanel_MouseClick(Object ^ sender, MouseEventArgs ^ e)
{
    if ((Control::ModifierKeys & Keys::Alt) != Keys::Alt)
        return;

    if (e->Button != System::Windows::Forms::MouseButtons::Left)
        return;

    if (!m_property_grid)
        return;

    Object ^ selected_object = m_property_grid->SelectedObject;
    if (!selected_object)
        return;

    GridItem ^ item = m_property_grid->SelectedGridItem;
    if (!item)
        return;

    PropertyDescriptor ^ descriptor_raw = item->PropertyDescriptor;
    VERIFY(descriptor_raw);
    PropertySpecDescriptor ^ descriptor = safe_cast<PropertySpecDescriptor ^>(descriptor_raw);
    property_container ^ container = safe_cast<property_container ^>(descriptor->bag);
    ::IProperty ^ raw_value = container->GetProperty(descriptor->item);
    VERIFY(raw_value);

    property_color_base ^ color = dynamic_cast<property_color_base ^>(raw_value);
    if (!color)
        return;

#ifdef XR_ARCHITECTURE_X64
    HDC dc = GetWindowDC((HWND)ViewPanel->Handle.ToInt64());
#else
    HDC dc = GetWindowDC((HWND)ViewPanel->Handle.ToInt32());
#endif
    u32 pixel_color = GetPixel(dc, e->Location.X, e->Location.Y);
    XRay::Editor::color value;
    value.r = float((pixel_color & 0x000000ff) >> 0) / 255.f;
    value.g = float((pixel_color & 0x0000ff00) >> 8) / 255.f;
    value.b = float((pixel_color & 0x00ff0000) >> 16) / 255.f;
    color->set_value_raw(value);

    m_property_grid->Refresh();
}

System::Void window_view::window_view_KeyDown(Object ^ sender, KeyEventArgs ^ e) { check_cursor(); }
void window_view::pick_color_cursor(bool value)
{
    if (!value)
    {
        ViewPanel->Cursor = System::Windows::Forms::Cursors::Default;
        return;
    }

    ViewPanel->Cursor = gcnew System::Windows::Forms::Cursor(
        (System::IntPtr)LoadCursor((HINSTANCE)System::Runtime::InteropServices::Marshal::GetHINSTANCE(
                               System::Reflection::Assembly::GetExecutingAssembly()->GetModules()[0])
#ifdef XR_ARCHITECTURE_X64
                               .ToInt64(),
#else
                               .ToInt32(),
#endif
            MAKEINTRESOURCE(IDC_CURSOR1)));
}

bool window_view::pick_color_cursor()
{
    if ((Control::ModifierKeys & Keys::Alt) != Keys::Alt)
        return (false);

    if (!m_property_grid)
        return (false);

    Object ^ selected_object = m_property_grid->SelectedObject;
    if (!selected_object)
        return (false);

    GridItem ^ item = m_property_grid->SelectedGridItem;
    if (!item)
        return (false);

    PropertyDescriptor ^ descriptor_raw = item->PropertyDescriptor;
    VERIFY(descriptor_raw);
    PropertySpecDescriptor ^ descriptor = safe_cast<PropertySpecDescriptor ^>(descriptor_raw);
    property_container ^ container = safe_cast<property_container ^>(descriptor->bag);
    ::IProperty ^ raw_value = container->GetProperty(descriptor->item);
    VERIFY(raw_value);

    property_color_base ^ color = dynamic_cast<property_color_base ^>(raw_value);
    if (!color)
        return (false);

    return (true);
}

void window_view::check_cursor() { pick_color_cursor(pick_color_cursor()); }
