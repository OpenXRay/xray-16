////////////////////////////////////////////////////////////////////////////
//	Module 		: property_editor_file_name.hpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property editor file name class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_EDITOR_FILE_NAME_HPP_INCLUDED
#define PROPERTY_EDITOR_FILE_NAME_HPP_INCLUDED

#include "property_holder_include.hpp"

namespace CustomControls {
namespace Controls {
	ref class OpenFileDialogEx;
} // namespace Controls
} // namespace CustomControls

public ref class property_editor_file_name : public System::Drawing::Design::UITypeEditor {
public:
	typedef editor::property_holder::string_getter_type		string_getter_type;
	typedef editor::property_holder::string_setter_type		string_setter_type;

private:
	typedef System::Drawing::Design::UITypeEditor			inherited;
	typedef System::Drawing::Design::UITypeEditorEditStyle	UITypeEditorEditStyle;
	typedef System::Windows::Forms::OpenFileDialog			OpenFileDialog;
	typedef Flobbster::Windows::Forms::PropertyBag			PropertyBag;
	typedef PropertyBag::PropertySpecDescriptor				PropertySpecDescriptor;
	typedef System::ComponentModel::ITypeDescriptorContext	ITypeDescriptorContext;
	typedef System::IServiceProvider						IServiceProvider;
	typedef System::Object									Object;
	typedef CustomControls::Controls::OpenFileDialogEx		OpenFileDialogEx;

public:
									property_editor_file_name	();
	virtual UITypeEditorEditStyle	GetEditStyle				(ITypeDescriptorContext^ context) override;
	virtual	Object^					EditValue					(
										ITypeDescriptorContext^ context,
										IServiceProvider^ provider,
										Object ^value
									) override;

private:
	OpenFileDialog^					m_dialog;
}; // ref class property_editor_file_name

#endif // ifndef PROPERTY_EDITOR_FILE_NAME_HPP_INCLUDED