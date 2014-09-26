////////////////////////////////////////////////////////////////////////////
//	Module 		: property_editor_file_name.cpp
//	Created 	: 07.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property editor file name class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"

//#define USE_CUSTOM_DIALOG

#ifdef USE_CUSTOM_DIALOG
#	pragma unmanaged
#	include <windows.h>
#	pragma comment(lib, "comdlg32.lib")
#	pragma managed
#endif // #ifdef USE_CUSTOM_DIALOG

#include "property_editor_file_name.hpp"
#include "property_file_name_value.hpp"
#include "property_container.hpp"

using System::Drawing::Design::UITypeEditorEditStyle;
using System::ComponentModel::ITypeDescriptorContext;
using System::Object;
using System::Windows::Forms::Design::IWindowsFormsEditorService;
using System::String;

property_editor_file_name::property_editor_file_name			() :
	m_dialog								(gcnew OpenFileDialog())
{
	m_dialog->AddExtension					= false;
	m_dialog->CheckFileExists				= true;
	m_dialog->CheckPathExists				= true;
	m_dialog->DereferenceLinks				= true;
	m_dialog->Multiselect					= false;
	m_dialog->ReadOnlyChecked				= true;
	m_dialog->RestoreDirectory				= true;
	m_dialog->ShowReadOnly					= false;
	m_dialog->ValidateNames					= true;
}

UITypeEditorEditStyle property_editor_file_name::GetEditStyle	(ITypeDescriptorContext^ context)
{
	if (context)
		return								(UITypeEditorEditStyle::Modal);

	return									(inherited::GetEditStyle(context));
}

Object^	property_editor_file_name::EditValue					(
		ITypeDescriptorContext^ context,
		IServiceProvider^ provider,
		Object^ value
	)
{
	if (!context || !provider)
		return								(inherited::EditValue(context, provider, value));

	typedef System::Windows::Forms::Design::IWindowsFormsEditorService	IWindowsFormsEditorService;
	IWindowsFormsEditorService^				service = 
		dynamic_cast<IWindowsFormsEditorService^>(
			provider->GetService(
				IWindowsFormsEditorService::typeid
			)
		);

	if (!service)
		return								(inherited::EditValue(context, provider, value));

	property_container^						container = safe_cast<property_container^>(context->Instance);
	PropertySpecDescriptor^					descriptor = safe_cast<PropertySpecDescriptor^>(context->PropertyDescriptor);
	property_value^							raw_value = container->value(descriptor->item);
	property_file_name_value_base^			real_value = safe_cast<property_file_name_value_base^>(raw_value);

#ifndef USE_CUSTOM_DIALOG
	m_dialog->FileName						= safe_cast<System::String^>(value);
	String^									default_extension = real_value->default_extension()->ToLower();
	m_dialog->DefaultExt					= default_extension;
	m_dialog->Filter						= real_value->filter();
//	m_dialog->InitialDirectory				= real_value->initial_directory();
	m_dialog->Title							= real_value->title();
	String^									initial_directory = System::IO::Path::GetFullPath(real_value->initial_directory()->ToLower());
	String^									string_value = safe_cast<System::String^>(value);
	if (string_value->Length)
		m_dialog->FileName					= initial_directory + string_value;
	else {
		m_dialog->InitialDirectory			= initial_directory;
		m_dialog->FileName					= "";
	}
	
	switch (m_dialog->ShowDialog()) {
		case System::Windows::Forms::DialogResult::OK : {
			String^							file_name = System::IO::Path::GetFullPath(m_dialog->FileName->ToLower());
			if (file_name->StartsWith(initial_directory))
				file_name					= file_name->Substring(initial_directory->Length);

			if (real_value->remove_extension() && (file_name->EndsWith(default_extension)))
				file_name					= file_name->Substring(0, file_name->Length - default_extension->Length);

			real_value->set_value			(file_name);
			break;
		}
	}
#else // #ifndef USE_CUSTOM_DIALOG
	CustomControls::FormOpenFileDialog		^dialog = gcnew CustomControls::FormOpenFileDialog();
	dialog->StartLocation					= CustomControls::Controls::AddonWindowLocation::None;
	dialog->DefaultViewMode					= CustomControls::OS::FolderViewMode::Thumbnails;

	dialog->OpenDialog->RestoreDirectory	= true;
	dialog->OpenDialog->DefaultExt			= real_value->default_extension();
	dialog->OpenDialog->Filter				= real_value->filter();
	dialog->OpenDialog->InitialDirectory	= System::IO::Path::GetFullPath(real_value->initial_directory());
	dialog->OpenDialog->Title				= real_value->title();
	dialog->OpenDialog->FileName			= dialog->OpenDialog->InitialDirectory + safe_cast<System::String^>(value);
	dialog->OpenDialog->InitialDirectory	= "";
    dialog->ShowDialog();
	delete(dialog);
#endif // #ifndef USE_CUSTOM_DIALOG
	
	return									(inherited::EditValue(context, provider, value));
}
