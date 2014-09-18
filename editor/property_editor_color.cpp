////////////////////////////////////////////////////////////////////////////
//	Module 		: property_editor_color.cpp
//	Created 	: 12.12.2007
//  Modified 	: 12.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property editor color class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_editor_color.hpp"
#include "property_container.hpp"
#include "property_color_base.hpp"

using System::Drawing::SolidBrush;
using System::Drawing::Graphics;
using System::Drawing::Rectangle;
using System::Math;

using System::Drawing::Design::UITypeEditorEditStyle;
using System::Object;
using System::Windows::Forms::ColorDialog;

typedef Flobbster::Windows::Forms::PropertyBag		PropertyBag;
typedef PropertyBag::PropertySpecDescriptor			PropertySpecDescriptor;

static int convert_color							(float color)
{
	return					((int)(Math::Min(color, 1.f)*255.f + .5f));
}

bool property_editor_color::GetPaintValueSupported	(ITypeDescriptorContext^ context)
{
	return					(true);
}

void property_editor_color::PaintValue				(PaintValueEventArgs^ arguments)
{
	if (!arguments->Value)
		return;

	property_container^		container = safe_cast<property_container^>(arguments->Value);
	editor::color			color = safe_cast<property_color_base%>(container->container_holder()).get_value_raw();
	Graphics^				graphics = arguments->Graphics;
	
	SolidBrush^				brush =	
		gcnew SolidBrush(
			System::Drawing::Color::FromArgb(
				255,
				convert_color(color.r),
				convert_color(color.g),
				convert_color(color.b)
			)
		);

	graphics->FillRectangle	(brush, arguments->Bounds);

	delete					(brush);
}

UITypeEditorEditStyle property_editor_color::GetEditStyle	(ITypeDescriptorContext^ context)
{
	if (context)
		return				(UITypeEditorEditStyle::Modal);

	return					(inherited::GetEditStyle(context));
}

Object^ property_editor_color::EditValue					(
		ITypeDescriptorContext^ context,
		IServiceProvider^ provider,
		Object ^value
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
	property_color_base^					real_value = safe_cast<property_color_base^>(raw_value);

	ColorDialog								^dialog = gcnew ColorDialog();
	dialog->FullOpen						= true;
	editor::color							color = real_value->get_value_raw();
	dialog->Color							= System::Drawing::Color::FromArgb(255, int(255.f*color.r), int(255.f*color.g), int(255.f*color.b));
	if (dialog->ShowDialog() != System::Windows::Forms::DialogResult::Cancel)
		real_value->set_value				(::Color(dialog->Color.R/255.f, dialog->Color.G/255.f, dialog->Color.B/255.f));

	return									(inherited::EditValue(context, provider, value));
}