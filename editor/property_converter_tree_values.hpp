////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_tree_values.hpp
//	Created 	: 21.12.2007
//  Modified 	: 21.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property converter tree values class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_CONVERTER_TREE_VALUES_HPP_INCLUDED
#define PROPERTY_CONVERTER_TREE_VALUES_HPP_INCLUDED

public ref class property_converter_tree_values : public System::ComponentModel::TypeConverter {
private:
	typedef System::ComponentModel::TypeConverter					inherited;
	typedef System::ComponentModel::PropertyDescriptorCollection	PropertyDescriptorCollection;
	typedef System::ComponentModel::ITypeDescriptorContext			ITypeDescriptorContext;
	typedef System::Object											Object;
	typedef System::Attribute										Attribute;
	typedef System::Globalization::CultureInfo						CultureInfo;
	typedef System::Type											Type;

public:
	typedef System::ComponentModel::TypeConverter::StandardValuesCollection	StandardValuesCollection;

public:
	virtual	bool	CanConvertFrom	(
						ITypeDescriptorContext^ context,
						Type^ source_type
					) override;
}; // ref class property_converter_tree_values

#endif // ifndef PROPERTY_CONVERTER_TREE_VALUES_HPP_INCLUDED