#include "pch.hpp"
#include "DockPanelSerializer.h"

#define COMPANY_NAME "GSC Game World"
#define PRODUCT_NAME "OpenXRay"

using Microsoft::Win32::Registry;
using Microsoft::Win32::RegistryKey;
using Microsoft::Win32::RegistryValueKind;

namespace XRay::Editor::Controls
{
template <typename T>
inline static T RegistryValue(RegistryKey^ key, System::String^ valueId, const T& defaultValue)
{
    auto names = key->GetValueNames();
    if (names->IndexOf(names, valueId) >= 0)
        return ((T)key->GetValue(valueId));

    return defaultValue;
}

static RegistryKey^ BaseRegistryKey()
{
    auto software = Registry::CurrentUser->OpenSubKey("Software", true);
    VERIFY(software);

    auto company = software->OpenSubKey(COMPANY_NAME, true);
    if (!company)
        company = software->CreateSubKey(COMPANY_NAME);
    VERIFY(company);
    software->Close();

    auto product = company->OpenSubKey(PRODUCT_NAME, true);
    if (!product)
        product = company->CreateSubKey(PRODUCT_NAME);

    VERIFY(product);
    company->Close();

    return product;
}

void Serializer::SerializeDockPanelRoot(Form^ root, DockPanel^ panel, String^ windowName)
{
    using System::IO::MemoryStream;

    auto product = BaseRegistryKey();
    R_ASSERT(product);

    auto windows = product->CreateSubKey("Windows");

    auto dock_panel = windows->CreateSubKey(windowName);
    auto stream = gcnew MemoryStream();

    panel->SaveAsXml(stream, System::Text::Encoding::Unicode, true);
    stream->Seek(0, System::IO::SeekOrigin::Begin);
    dock_panel->SetValue("Panels", stream->ToArray());
    delete stream;

    dock_panel->SetValue("WindowState", 2);
    switch (root->WindowState)
    {
    case FormWindowState::Maximized:
        dock_panel->SetValue("WindowState", 1);

    case FormWindowState::Minimized:
    {
        auto position = dock_panel->CreateSubKey("Position");
        position->SetValue("Left", root->RestoreBounds.X);
        position->SetValue("Top", root->RestoreBounds.Y);
        position->SetValue("Width", root->RestoreBounds.Width);
        position->SetValue("Height", root->RestoreBounds.Height);
        position->Close();

        break;
    }

    default:
    {
        dock_panel->SetValue("WindowState", 2);
        {
            auto position = dock_panel->CreateSubKey("Position");
            position->SetValue("Left", root->Location.X);
            position->SetValue("Top", root->Location.Y);
            position->SetValue("Width", root->Size.Width);
            position->SetValue("Height", root->Size.Height);
            position->Close();
        }
        break;
    }
    }

    dock_panel->Close();

    windows->Close();
    product->Close();
}

bool Serializer::DeserializeDockPanelRoot(Form^ root, DockPanel^ panel, String^ windowName, DeserializeDockContent^ getPanelForSettingCallback)
{
    bool isLoadSuccess = false;
    root->SuspendLayout();

    root->Width = 800;
    root->Height = 600;

    auto product = BaseRegistryKey();
    R_ASSERT(product);

    auto windows = product->OpenSubKey("Windows");
    if (windows)
    {
        auto dockPanel = windows->OpenSubKey(windowName);
        if (dockPanel)
        {
            auto temp = dockPanel->GetValue("Panels");

            if (temp)
            {
                auto object = safe_cast<System::Array^>(dockPanel->GetValue("Panels"));

                windows->Close();
                delete windows;

                product->Close();
                delete product;

                auto stream = gcnew System::IO::MemoryStream();
                stream->Write(safe_cast<array<unsigned char, 1>^>(object), 0, object->Length);
                stream->Seek(0, System::IO::SeekOrigin::Begin);
                panel->LoadFromXml(stream, getPanelForSettingCallback);
                root->ResumeLayout();
                isLoadSuccess = true;
            }

            auto position = dockPanel->OpenSubKey("Position");
            if (position)
            {
                root->Left = (int)RegistryValue(position, "Left", root->Left);
                root->Top = (int)RegistryValue(position, "Top", root->Top);
                root->Width = (int)RegistryValue(position, "Width", root->Width);
                root->Height = (int)RegistryValue(position, "Height", root->Height);

                root->Location = Point(root->Left, root->Top);

                position->Close();
            }

            switch ((int)RegistryValue(dockPanel, "WindowState", 2))
            {
            case 1:
            {
                root->WindowState = FormWindowState::Maximized;
                break;
            }

            case 2:
            {
                root->WindowState = FormWindowState::Normal;
                break;
            }

            default: NODEFAULT;
            }

            dockPanel->Close();
        }
        windows->Close();
        delete windows;
    }

    product->Close();
    delete product;

    root->ResumeLayout();

    return isLoadSuccess;
}

} // namespace XRay::Editor::Controls::Serializer
