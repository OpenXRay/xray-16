////////////////////////////////////////////////////////////////////////////
//	Module 		: property_editor_tree_values.cpp
//	Created 	: 20.12.2007
//  Modified 	: 20.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property editor tree values class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_editor_tree_values.hpp"
#include "property_container.hpp"
#include "window_tree_values.h"

using System::Drawing::Design::UITypeEditorEditStyle;
using System::ComponentModel::ITypeDescriptorContext;
using System::Object;
using System::Windows::Forms::Design::IWindowsFormsEditorService;
using System::String;
using editor::window_tree_values;

property_editor_tree_values::property_editor_tree_values			() :
	m_dialog								(gcnew window_tree_values())
{
}

UITypeEditorEditStyle property_editor_tree_values::GetEditStyle	(ITypeDescriptorContext^ context)
{
	if (context)
		return								(UITypeEditorEditStyle::Modal);

	return									(inherited::GetEditStyle(context));
}

Object^	property_editor_tree_values::EditValue					(
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
	property_string_values_value_base^		real_value = safe_cast<property_string_values_value_base^>(raw_value);
	m_dialog->values						(real_value->values(), safe_cast<String^>(raw_value->get_value()));
	switch (m_dialog->ShowDialog()) {
		case System::Windows::Forms::DialogResult::OK : {
			raw_value->set_value			(m_dialog->Result);
			break;
		}
	}
	
	return									(inherited::EditValue(context, provider, value));
}
